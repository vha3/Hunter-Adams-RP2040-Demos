/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "pt_cornell_rp2040_v1_3_client.h"

// Show/conceal debugging information
#if 0
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

// UUID of custom service that we'll access
#define CUSTOM_SERVICE 0xFF10
static const uint8_t service_name[16] = {0x00, 0x00, 0xFF, 0x10, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB} ;

// UUID for the characteristic descriptors of interest
#define CHARACTERISTIC_USER_DESCRIPTION 0x2901
#define CHARACTERISTIC_CONFIGURATION    0x2902

// The number of letters in the alphabet. Could be larger, but would
// just require a little refactoring below.
#define MAX_CHARACTERISTICS 26

// Plaintext for various access permisions
char broadcast[] = "Broadcast";
char read[] = "Read";
char write_no_resp[] = "Write without response";
char write[] = "Write";
char notify[] = "Notify";
char indicate[] = "Indicate" ;
char authen[] = "Signed write";
char extended[] = "Extended";
char * access_permissions[8] = {broadcast, read, write_no_resp, write, notify, indicate, authen, extended} ;


// Some human-readable names for states in our ATT state machine
typedef enum {
    TC_OFF=0,
    TC_IDLE,
    TC_W4_SCAN_RESULT,
    TC_W4_CONNECT,
    TC_W4_SERVICE_RESULT,
    TC_W4_CHARACTERISTIC_RESULT,
    TC_W4_CHARACTERISTIC_DESCRIPTOR,
    TC_W4_CHARACTERISTIC_DESCRIPTOR_PRINT,
    TC_W4_CHARACTERISTIC_VALUE_PRINT,
    TC_W4_CHARACTERISTIC_NOTIFICATIONS,
    TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
    TC_W4_READY
} gc_state_t;


// Bluetooth control structures
static btstack_packet_callback_registration_t hci_event_callback_registration;
static gc_state_t state = TC_OFF;
static bd_addr_t server_addr;
static bd_addr_type_t server_addr_type;
static hci_con_handle_t connection_handle;
static gatt_client_service_t server_service;

//  Server characteristic object, and associated array of server descriptor objects
static gatt_client_characteristic_t             server_characteristic[MAX_CHARACTERISTICS];
static gatt_client_characteristic_descriptor_t  server_characteristic_descriptor[MAX_CHARACTERISTICS][2] ;
static uint8_t                                  server_characteristic_user_description[MAX_CHARACTERISTICS][50] ;
// Arrays in which to hold characteristic data. We send data as formatted strings (just like a serial console)
static char server_characteristic_values[MAX_CHARACTERISTICS][100] ;
// Array for holding characteristic configuration
static int server_characteristic_configurations[MAX_CHARACTERISTICS] ;

// For keeping track of notification status for each characteristic
char notifications_enabled[MAX_CHARACTERISTICS] = {0} ;

// For characteristic notifications
static bool listener_registered;
static gatt_client_notification_t notification_listener;

// Protothreads semaphore
struct pt_sem characteristics_discovered ;

// Counts characteristics in ATT state machine
int k = 0 ;
// Counts characteristic descriptors in ATT state machine
int k2 = 0 ;
// Holds the number of discovered characteristics
int num_characteristics = 0 ;
// Holds the length of received packets in ATT state machine
uint32_t descriptor_length ;

// Start the client. Initialize GATT state machine, and start GAP scan
static void client_start(void){
    DEBUG_LOG("Start scanning!\n");
    state = TC_W4_SCAN_RESULT;
    gap_set_scan_parameters(0,0x0030, 0x0030);
    gap_start_scan();
}

static bool advertisement_report_contains_service(uint16_t service, uint8_t *advertisement_report){
    // get advertisement from report event
    const uint8_t * adv_data = gap_event_advertising_report_get_data(advertisement_report);
    uint8_t adv_len  = gap_event_advertising_report_get_data_length(advertisement_report);

    // iterate over advertisement data
    ad_context_t context;
    for (ad_iterator_init(&context, adv_len, adv_data) ; ad_iterator_has_more(&context) ; ad_iterator_next(&context)){
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t data_size = ad_iterator_get_data_len(&context);
        const uint8_t * data = ad_iterator_get_data(&context);
        switch (data_type){
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
                for (int i = 0; i < data_size; i += 2) {
                    uint16_t type = little_endian_read_16(data, i);
                    if (type == service) return true;
                }
            default:
                break;
        }
    }
    return false;
}

// Parses and prints access permissions
void parsePermissions(uint16_t value) {
    int i ;
    for (i=0; i<9; i++) {
        if ((1u<<i) & value) {
            printf("| %s ", access_permissions[i]) ;
        }
    }
    printf(" |\n") ;
}

// Callback function which manages GATT events. Implements a state machine.
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    // Local variables for storing ATT status and type of packet
    uint8_t att_status;
    uint8_t type_of_packet ;

    // What type of packet did we just receive?
    type_of_packet = hci_event_packet_get_type(packet) ;

    // Did we just receive a notification? If so, update the associated characteristic value
    // and signal the protothread which refreshes values in the terminal UI.
    if (type_of_packet == GATT_EVENT_NOTIFICATION) {
        // How many bytes in the payload?
        uint32_t value_length = gatt_event_notification_get_value_length(packet);
        // What is the value handle for the notification we've just received
        uint16_t notification_handle = gatt_event_notification_get_value_handle(packet) ;
        // What is the address of the start of the data for this packet (a byte array)
        const uint8_t *value = gatt_event_notification_get_value(packet);
        // Initialize our characteristic identifier to -1 (invalid)
        int which_characteristic = -1 ;
        // Loop thru each characteristic, compare that characteristic's value handle to the value
        // handle of the notification we've received. A match indicates that this is a notification
        // for the associated characteristic.
        for (int i=0; i<num_characteristics; i++) {
            if (notification_handle==server_characteristic[i].value_handle) {
                which_characteristic = i ;
            }
        }
        // Did we find a value handle match?
        if (which_characteristic>=0) {
            // If so, copy the received data to the value buffer of the associated characteristic
            memcpy(server_characteristic_values[which_characteristic], value, value_length) ;
            // Null-terminate the character array
            server_characteristic_values[which_characteristic][value_length] = 0 ;
            // Make sure notifications are labeled as on for this characteristic
            // notifications_enabled[which_characteristic] = 1 ;
            // Semaphore-signal the protothread which will refresh values in the UI
            PT_SEM_SAFE_SIGNAL(pt, &characteristics_discovered) ;
        }
        // Exit the callback
        return;
    }

    // Packet was not a notification, enter state machine
    switch(state){

        // We've been placed into this state by the HCI callback machine, which
        // has connected to a GATT server and has queried for our custom service UUID
        case TC_W4_SERVICE_RESULT:
            switch(type_of_packet) {
                // This packet contains the result of the service query. Store the service.
                case GATT_EVENT_SERVICE_QUERY_RESULT:
                    // store service (we expect only one)
                    DEBUG_LOG("Storing service\n");
                    gatt_event_service_query_result_get_service(packet, &server_service);
                    break;

                // Finished with query, look for our custom characteristic
                case GATT_EVENT_QUERY_COMPLETE:
                    // Check ATT status to make sure we don't have any errors
                    att_status = gatt_event_query_complete_get_att_status(packet);
                    if (att_status != ATT_ERROR_SUCCESS){
                        printf("SERVICE_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                        gap_disconnect(connection_handle);
                        break;  
                    } 
                    // Turn on the LED to indicate connection to server
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    // Clear all notification information
                    memset(notifications_enabled, -1, MAX_CHARACTERISTICS) ;
                    // Transition to next state
                    state = TC_W4_CHARACTERISTIC_RESULT;
                    DEBUG_LOG("Search for counting characteristic.\n");
                    // Discover all characteristics contained within custom service.
                    gatt_client_discover_characteristics_for_service(handle_gatt_client_event, connection_handle, &server_service);
                    break;

                default:
                    break;
            }
            break;

        // We've found the custom service, and we've just sent a query for all characteristics.
        // We don't know how many characteristics this service contains, we need to discover them.
        case TC_W4_CHARACTERISTIC_RESULT:
            switch(type_of_packet) {
                // This packet contains information about a characteristic in the custom service. We'll
                // keep receiving these packets until we've gotten one for each service. Then we'll get
                // a GATT_EVENT_QUERY_COMPLETE packet.
                case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
                    // Store the characteristic in a gatt_client_characteristic_t object, and increment to the next
                    // element in an array of such objects.
                    DEBUG_LOG("Storing characteristic\n");
                    gatt_event_characteristic_query_result_get_characteristic(packet, &server_characteristic[k++]);
                    break;

                // We've received all characteristics!
                case GATT_EVENT_QUERY_COMPLETE:
                    // How many characteristics did we just discover?
                    num_characteristics = k ;

                    // Reset our two incrementers
                    k  = 0 ;
                    k2 = 0 ;
  
                    // Check the ATT status for errors
                    att_status = gatt_event_query_complete_get_att_status(packet);
                    if (att_status != ATT_ERROR_SUCCESS){
                        printf("CHARACTERISTIC_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                        gap_disconnect(connection_handle);
                        break;  
                    } 

                    // Clear the terminal screen
                    printf("\033[2J") ;

                    // Discover all characteristic descriptors for all characteristics. starting with number "k" (which
                    // has been reset to 0 above).
                    gatt_client_discover_characteristic_descriptors(handle_gatt_client_event, connection_handle, &server_characteristic[k]) ;

                    // State transition
                    state = TC_W4_CHARACTERISTIC_DESCRIPTOR ;
                    break;

                default:
                    break;
            }
            break;            

        // We're querying each characteristic for its descriptors.
        case TC_W4_CHARACTERISTIC_DESCRIPTOR:
            switch(type_of_packet) {
                // We've just received another descriptor for characteristic k. Store it in an array of gatt_characteristic_descriptor objects, and then
                // increment to the next element in an array of such objects.
                case GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT:
                    DEBUG_LOG("Storing characteristic descriptor\n") ;
                    gatt_event_all_characteristic_descriptors_query_result_get_characteristic_descriptor(packet, &server_characteristic_descriptor[k][k2++]) ;
                    break ;

                // We've received all descriptors for characteristic k.
                case GATT_EVENT_QUERY_COMPLETE:
                    DEBUG_LOG("Transitioning to Descriptor Print\n") ;
                    // Increment k to the next characteristic
                    k++ ;
                    // Reset k2 (used to count characteristic descriptors) back to 0.
                    k2 = 0 ;
                    // If we haven't yet queried all of the characteristics that we discovered, query the next characteristic for all its descriptors.
                    // We stay in this state to receive these next descriptors.
                    if (k < num_characteristics) {
                        gatt_client_discover_characteristic_descriptors(handle_gatt_client_event, connection_handle, &server_characteristic[k]) ;
                    }
                    // If we have received all of the descriptors for all of our characteristics . . .
                    else {
                        // Reset our characteristic counter to 0.
                        k = 0 ;
                        // For characteristic k (reset to 0), query the value of the descriptor with UUID CHARACTERISTIC_USER_DESCRIPTION.
                        // This descriptor contains the plaintext user description of this characteristic. We're not sure if this is the first or the second
                        // descriptor for the characteristic, so check the UUID of each and read the correct one.
                        if (server_characteristic_descriptor[k][0].uuid16 == CHARACTERISTIC_USER_DESCRIPTION) {
                            gatt_client_read_characteristic_descriptor(handle_gatt_client_event, connection_handle, &server_characteristic_descriptor[k][0]) ;
                        }
                        else {
                            gatt_client_read_characteristic_descriptor(handle_gatt_client_event, connection_handle, &server_characteristic_descriptor[k][1]) ;
                        }
                        // Move to next state.
                        state = TC_W4_CHARACTERISTIC_DESCRIPTOR_PRINT ;
                    }
                    break ;

                default:
                    break ;
            }
            break ;

        // We'd like to locally store the user descriptions for each characteristic. We'll do so in this state.
        case TC_W4_CHARACTERISTIC_DESCRIPTOR_PRINT:
            switch(type_of_packet) {
                // The result of the characteristic descriptor query for the CHARACTERISTIC_USER_DESCRIPTION. Parse this packet,
                // and store the contents in a global array of descriptors.
                case GATT_EVENT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT:
                    DEBUG_LOG("Storing characteristic descriptor\n") ;
                    // The user description is a character array. How long is it?
                    descriptor_length = gatt_event_characteristic_descriptor_query_result_get_descriptor_length(packet);
                    // What is the address of the start of this array?
                    const uint8_t *descriptor = gatt_event_characteristic_descriptor_query_result_get_descriptor(packet);

                    // Store the user description in a global array of characteristic descriptions.
                    memcpy(server_characteristic_user_description[k], descriptor, descriptor_length) ;
                    // Null-terminate the character array.
                    server_characteristic_user_description[k][descriptor_length] = 0 ;

                    break ;

                // Finished with read for this particular characteristic, do we need to read more?
                case GATT_EVENT_QUERY_COMPLETE:
                    // Increment k to the next characteristic
                    k++ ;
                    // Are there still characteristics remaining? If so . . .
                    if (k < num_characteristics) {
                        // Read the user description for the next characteristic, just like before.
                        if (server_characteristic_descriptor[k][0].uuid16 == CHARACTERISTIC_USER_DESCRIPTION) {
                            gatt_client_read_characteristic_descriptor(handle_gatt_client_event, connection_handle, &server_characteristic_descriptor[k][0]) ;
                        }
                        else {
                            gatt_client_read_characteristic_descriptor(handle_gatt_client_event, connection_handle, &server_characteristic_descriptor[k][1]) ;
                        }
                    }
                    // If not . . .
                    else {
                        // Reset both the characteristic and characteristic descriptor iterators
                        k   = 0 ;
                        k2  = 0 ;
                        // Read the value of characteristic k (reset to 0)
                        gatt_client_read_value_of_characteristic(handle_gatt_client_event, connection_handle, &server_characteristic[k]) ;
                        // Transition state
                        state = TC_W4_CHARACTERISTIC_VALUE_PRINT ;
                    }
                    break ;

                default:
                    break ;
            }
            break ;

        // We'd like to read the value of each characteristic. We'll do so in this state.
        case TC_W4_CHARACTERISTIC_VALUE_PRINT:
            switch(type_of_packet) {

                // We've just received the result of a characteristic value query. This packet contains that value
                case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
                    DEBUG_LOG("Storing characteristic value\n") ;
                    // The characteristic value is a character array. How long is it?
                    descriptor_length = gatt_event_characteristic_value_query_result_get_value_length(packet);
                    // What is the address to the start of this array?
                    const uint8_t *descriptor = gatt_event_characteristic_value_query_result_get_value(packet);

                    // Store the characteristic value in a global array of characteristic values
                    memcpy(server_characteristic_values[k], descriptor, descriptor_length) ;
                    // Null-terminate the character array.
                    server_characteristic_values[k][descriptor_length] = 0 ;
                    break ;

                // We've finished receiving the value for a particular characteristic
                case GATT_EVENT_QUERY_COMPLETE:
                    // Move to the next characteristic
                    k++ ;
                    // Do we still have more characteristics to query for value? If so . . .
                    if (k < num_characteristics) {
                        // Read the next one!
                        gatt_client_read_value_of_characteristic(handle_gatt_client_event, connection_handle, &server_characteristic[k]) ;
                    }
                    // If not . . .
                    else {
                        k = 0 ;
                        k2 = 0 ;

                        // Find the next characteristic with a descriptor.
                        while((k<num_characteristics) && (server_characteristic_descriptor[k][0].uuid16!=CHARACTERISTIC_CONFIGURATION)) {
                            k++ ;
                        }

                        // Have we gone thru all characteristics? If not, read the next configuration
                        if (k<num_characteristics) {
                            DEBUG_LOG("QUERYING FOR CONFIGURATION: %d\n", k) ;
                            gatt_client_read_characteristic_descriptor(handle_gatt_client_event, connection_handle, &server_characteristic_descriptor[k][0]) ;
                            // Transition state
                            state = TC_W4_CHARACTERISTIC_NOTIFICATIONS ;
                        }
                        // If so, move to next state
                        else {
                            // Signal thread that all characteristics have been acquired
                            PT_SEM_SAFE_SIGNAL(pt, &characteristics_discovered) ;

                            // Transition to an idle state
                            state = TC_W4_READY ;

                            // Turn on notifications
                            // Register handler for notifications
                            listener_registered = true;
                            gatt_client_listen_for_characteristic_value_updates(&notification_listener, handle_gatt_client_event, connection_handle, NULL);
                        }
                    }
                    break ;

                default:
                    break ;

            }
            break ;

        // Obtain the notification status of each characteristic
        case TC_W4_CHARACTERISTIC_NOTIFICATIONS:
            switch(type_of_packet) {
                // Received the result of a notification query
                case GATT_EVENT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT:
                    DEBUG_LOG("Storing characteristic notification status\n") ;
                    // The characteristic value is a character array. How long is it?
                    descriptor_length = gatt_event_characteristic_value_query_result_get_value_length(packet);
                    // What is the address to the start of this array?
                    const uint8_t *descriptor = gatt_event_characteristic_value_query_result_get_value(packet);
                    // Store the characteristic configuration
                    server_characteristic_configurations[k] = little_endian_read_16(descriptor, 0) ;
                    // Set the notifications as enabled or disabled
                    if ((char)server_characteristic_configurations[k]) {
                        notifications_enabled[k] = 1 ;
                    }
                    else {
                        notifications_enabled[k] = 0 ;
                    }
                    break ;
                // Finished with a query. Do we need to do another?
                case GATT_EVENT_QUERY_COMPLETE:
                    // Increment k one time
                    k++ ;
                    // Find the next characteristic with a descriptor.
                    while((k<num_characteristics) && (server_characteristic_descriptor[k][0].uuid16!=CHARACTERISTIC_CONFIGURATION)) {
                        k++ ;
                    }
                    // Have we gone thru all characteristics? If not, read the next configuration
                    if (k<num_characteristics) {
                        gatt_client_read_characteristic_descriptor(handle_gatt_client_event, connection_handle, &server_characteristic_descriptor[k][0]) ;
                    }
                    // If so, move to next state
                    else {
                        // Signal thread that all characteristics have been acquired
                        PT_SEM_SAFE_SIGNAL(pt, &characteristics_discovered) ;

                        // Transition to an idle state
                        state = TC_W4_READY ;

                        // Turn on notifications
                        // Register handler for notifications
                        listener_registered = true;
                        gatt_client_listen_for_characteristic_value_updates(&notification_listener, handle_gatt_client_event, connection_handle, NULL);
                    }
                    break ;


                default:
                    break ;

            }
            break ;

        // We've just enabled or disabled notifications for a particular characteristic.
        case TC_W4_ENABLE_NOTIFICATIONS_COMPLETE:
            switch(type_of_packet) {
                // Notifications have been enabled, transition to idle state
                case GATT_EVENT_QUERY_COMPLETE:
                    DEBUG_LOG("Notifications enabled, ATT status 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
                    if (gatt_event_query_complete_get_att_status(packet) != ATT_ERROR_SUCCESS) break;
                    state = TC_W4_READY;
                    break;

                default:
                    break;
            }
            break;

        // An idle state
        case TC_W4_READY:
            break;

        default:
            printf("error\n");
            break;
    }
}

// Callback function for HCI events
static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    // Retrieve event type
    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        // Starting up
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                gap_local_bd_addr(local_addr);
                printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
                client_start();
            } else {
                state = TC_OFF;
            }
            break;
        // We've received an advertising report!
        case GAP_EVENT_ADVERTISING_REPORT:
            // We're placed into this state by client_start(), above
            if (state != TC_W4_SCAN_RESULT) return;
            // Confirm that this advertising report includes our custom service UUID
            if (!advertisement_report_contains_service(CUSTOM_SERVICE, packet)) return;
            // Store the address of the sender, and the type of the server
            gap_event_advertising_report_get_address(packet, server_addr);
            server_addr_type = gap_event_advertising_report_get_address_type(packet);
            // Perform a state transition
            state = TC_W4_CONNECT;
            // Stop scanning
            gap_stop_scan();
            // Print a little message
            printf("Connecting to device with addr %s.\n", bd_addr_to_str(server_addr));
            // Connect to the server that sent the advertising report
            gap_connect(server_addr, server_addr_type);
            break;
        case HCI_EVENT_LE_META:
            // Wait for connection complete
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    // Confirm that we are in the proper state
                    if (state != TC_W4_CONNECT) return;
                    // Retrieve connection handle from packet
                    connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    // initialize gatt client context with handle, and add it to the list of active clients
                    // query primary services
                    // Search for the custom service which was advertised
                    DEBUG_LOG("Search for custom service.\n");
                    // Perform state transition (ATT state machine above)
                    state = TC_W4_SERVICE_RESULT;
                    // Search for our custom service by its 128-bit UUID
                    gatt_client_discover_primary_services_by_uuid128(handle_gatt_client_event, connection_handle, service_name);
                    break;
                default:
                    break;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            // unregister listener
            connection_handle = HCI_CON_HANDLE_INVALID;
            if (listener_registered){
                listener_registered = false;
                gatt_client_stop_listening_for_characteristic_value_updates(&notification_listener);
            }
            printf("Disconnected %s\n", bd_addr_to_str(server_addr));
            // Turn off the LED to indicate disconnection to server
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // Initialize relevant variables
            k = 0 ;
            k2 = 0 ;
            num_characteristics = 0 ;
            if (state == TC_OFF) break;
            // Start looking for another server
            client_start();
            break;
        default:
            break;
    }
}

// Protothread that prints all characteristic values
static PT_THREAD (protothread_client(struct pt *pt))
{
    PT_BEGIN(pt) ;

    // For incrementing through characteristics
    static int counter = 0 ;

    while(1) {

        // Wait to be signalled by the GATT state machine
        PT_SEM_SAFE_WAIT(pt, &characteristics_discovered) ;

        // Save cursor position
        printf("\033[s") ;
        // Move cursor to 10th row
        printf("\033[10;0H") ;

        // Make cursor invisible
        printf("\033[?25l") ;

        // Print all characteristic values
        printf("Discovered characteristics:\n\n") ;
        for (counter = 0; counter < num_characteristics; counter++) {
            printf("Characteristic ID.:\033[K\t %c\n", (counter+97)) ;
            printf("User description:\033[K\t %s\n", server_characteristic_user_description[counter]) ;
            // printf("Config:\033[K\t\t\t %04x\n", notifications_enabled[counter]) ;
            printf("Access permissions:\033[K\t ") ;
            // Only print notification status if characteristic allows for notifications
            parsePermissions(server_characteristic[counter].properties) ;
            if ((notifications_enabled[counter]>=0) && (notifications_enabled[counter]<2)) {
                if (notifications_enabled[counter]) printf("\033[1mNotify:\033[K\t\t\t ON\033[22m\n") ;
                else printf("Notify:\033[K\t\t\t OFF\n") ;
            }
            printf("Value:\033[K\t\t\t %s\n\n", server_characteristic_values[counter]) ;
        }
        // Erase down
        printf("\033[J") ;

        // Restore cursor position
        printf("\033[s") ;
    }

    PT_END(pt) ;
}

// Protothread which implements a user interface
static PT_THREAD (protothread_ui(struct pt *pt))
{
    PT_BEGIN(pt) ;

    // For holding user input
    static char temp_var ;
    static char temp_var2 ;

    while(1) {
        // Move cursor to line above data
        printf("\033[10;0H") ;
        // Erase to top
        printf("\033[1J") ;
        // Move cursor to top
        printf("\033[H") ;

        // Print user instructions
        sprintf(pt_serial_out_buffer, "Type 0 to refresh characteristic values.\n\r") ;
        serial_write ;
        sprintf(pt_serial_out_buffer, "Type the ID of a characteristic to write.\n\r") ;
        serial_write ;
        sprintf(pt_serial_out_buffer, "Type the ID of a characteristic (UPPERCASE) to toggle notifications on/off.\n\r") ;
        serial_write ;

        // Wait for user input
        serial_read ;

        // Parse user input
        temp_var = pt_serial_in_buffer[0]-97 ;
        temp_var2 = pt_serial_in_buffer[0]-65 ;

        // Did the user specify a valid (lowercase) characteristic ID?
        if ((temp_var>=0) && (temp_var<num_characteristics)) {

            // Does this characteristic include write permissions?
            if (server_characteristic[temp_var].properties & (1u<<2)) {
                // Move cursor to row 4
                printf("\033[4;0H") ;

                // Prompt for user input
                sprintf(pt_serial_out_buffer, "Please type a value for characteristic %c.\n\r\033[K", temp_var+97) ;
                serial_write ;
                serial_read ;

                // Send user input to server
                int status = gatt_client_write_value_of_characteristic_without_response(connection_handle, server_characteristic[temp_var].value_handle, strlen(pt_serial_in_buffer), pt_serial_in_buffer);
            }

            // If characteristic does not include write permissions, tell the user.
            else {
                printf("\033[4;0H") ;
                sprintf(pt_serial_out_buffer, "\033[1mNo write permission for that characteristic.\n\r\033[22m") ;
                serial_write ;
                PT_YIELD_usec(500000) ;
            }
        }

        // Did the user alternatively specify a valid (UPPERCASE) characteristic ID?
        else if ((temp_var2>=0) && (temp_var2<num_characteristics)) {

            // Does this characteristic include notifications?
            if (server_characteristic[temp_var2].properties & (1u<<4)) {

                // Wait until our state machine is ready for a new instruction
                PT_YIELD_UNTIL(pt, state==TC_W4_READY) ;

                // If notifications are presently enabled, disable them
                if (notifications_enabled[temp_var2]) {
                    state = TC_W4_ENABLE_NOTIFICATIONS_COMPLETE;
                    gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, connection_handle,
                        &server_characteristic[temp_var2], GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NONE);
                }

                // If notifications are presently disabled, enable them
                else {
                    state = TC_W4_ENABLE_NOTIFICATIONS_COMPLETE;
                    gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, connection_handle,
                        &server_characteristic[temp_var2], GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
                }
            }

            // If not, tell the user
            else {
                printf("\033[4;0H") ;
                sprintf(pt_serial_out_buffer, "\033[1mNo notification permission for that characteristic.\n\r\033[22m") ;
                serial_write ;
                PT_YIELD_usec(500000) ;
            }
        }

        // Wait until the state machine is ready to receive a new instruction
        PT_YIELD_UNTIL(pt, state==TC_W4_READY) ;

        // Reset characteristic counters
        k = 0 ;
        k2 = 0 ;

        // Refresh all characteristic values
        state = TC_W4_CHARACTERISTIC_VALUE_PRINT ;
        gatt_client_read_value_of_characteristic(handle_gatt_client_event, connection_handle, &server_characteristic[k]) ;
    }

    PT_END(pt) ;
}


int main() {

    // Initialize stdio
    stdio_init_all();

    // Make cursor invisible
    printf("\033[?25l") ;

    // Initialize the semaphore
    PT_SEM_SAFE_INIT(&characteristics_discovered, 0) ;

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    // Initialize L2CAP and Security Manager
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    // setup empty ATT server - only needed if LE Peripheral does ATT queries on its own, e.g. Android and iOS
    att_server_init(NULL, NULL, NULL);

    // Initialize GATT client
    gatt_client_init();

    // Register the HCI event callback function
    hci_event_callback_registration.callback = &hci_event_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Turn on!
    hci_power_control(HCI_POWER_ON);

    // Add and schedule threads
    pt_add_thread(protothread_client) ;
    pt_add_thread(protothread_ui) ;
    pt_sched_method = SCHED_ROUND_ROBIN ;
    pt_schedule_start ;

}
