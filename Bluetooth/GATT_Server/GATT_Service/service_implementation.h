/*

	V. Hunter Adams (vha3@cornell.edu)
	Custom GATT Service implementation
	Modeled off examples from BTstack

*/



#include "btstack_defines.h"
#include "ble/att_db.h"
#include "ble/att_server.h"
#include "btstack_util.h"
#include "bluetooth_gatt.h"
#include "btstack_debug.h"

#include "server_demo_gattfile.h"
#include "Protothreads/pt_cornell_rp2040_v1_3.h"



// Create a struct for managing this service
typedef struct {

	// Connection handle for service
	hci_con_handle_t con_handle ;

	// Characteristic A information
	char * 		characteristic_a_value ;
	uint16_t 	characteristic_a_client_configuration ;
	char * 		characteristic_a_user_description ;

	// Characteristic B information
	char *  	characteristic_b_value ;
	uint16_t  	characteristic_b_client_configuration ;
	char *  	characteristic_b_user_description ;

	// Characteristic C information
	char *  	characteristic_c_value ;
	uint16_t  	characteristic_c_client_configuration ;
	char *  	characteristic_c_user_description ;

	// Characteristic D information
	char *  	characteristic_d_value ;
	uint16_t  	characteristic_d_client_configuration ;
	char *  	characteristic_d_user_description ;

	// Characteristic E information
	char *  	characteristic_e_value ;
	char *  	characteristic_e_user_description ;

	// Characteristic F information
	char *  	characteristic_f_value ;
	char *  	characteristic_f_user_description ;

	// Characteristic A handles
	uint16_t  	characteristic_a_handle ;
	uint16_t 	characteristic_a_client_configuration_handle ;
	uint16_t 	characteristic_a_user_description_handle ;

	// Characteristic B handles
	uint16_t  	characteristic_b_handle ;
	uint16_t 	characteristic_b_client_configuration_handle ;
	uint16_t 	characteristic_b_user_description_handle ;

	// Characteristic C handles
	uint16_t  	characteristic_c_handle ;
	uint16_t 	characteristic_c_client_configuration_handle ;
	uint16_t 	characteristic_c_user_description_handle ;

	// Characteristic D handles
	uint16_t  	characteristic_d_handle ;
	uint16_t 	characteristic_d_client_configuration_handle ;
	uint16_t 	characteristic_d_user_description_handle ;

	// Characteristic E handles
	uint16_t  	characteristic_e_handle ;
	uint16_t 	characteristic_e_user_description_handle ;

	// Characteristic E handles
	uint16_t  	characteristic_f_handle ;
	uint16_t 	characteristic_f_user_description_handle ;

	// Callback functions
	btstack_context_callback_registration_t callback_a ;
	btstack_context_callback_registration_t callback_b ;
	btstack_context_callback_registration_t callback_c ;
	btstack_context_callback_registration_t callback_d ;

} custom_service_t ;

// Create a callback registration object, and an att service handler object
static att_service_handler_t 	service_handler ;
static custom_service_t 		service_object ;

// Characteristic user descriptions (appear in LightBlue app)
char characteristic_a[] = "Read-only Counter" ;
char characteristic_b[] = "DDS Frequency" ;
char characteristic_c[] = "String from Pico" ;
char characteristic_d[] = "String to Pico" ;
char characteristic_e[] = "LED Status and Control" ;
char characteristic_f[] = "Color Selection" ;

// Protothreads semaphore
struct pt_sem BLUETOOTH_READY ;

// Callback functions for ATT notifications on characteristics
static void characteristic_a_callback(void * context){
	// Associate the void pointer input with our custom service object
	custom_service_t * instance = (custom_service_t *) context ;
	// Send a notification
	att_server_notify(instance->con_handle, instance->characteristic_a_handle, instance->characteristic_a_value, strlen(instance->characteristic_a_value)) ;
}
static void characteristic_b_callback(void * context) {
	// Associate the void pointer input with our custom service object
	custom_service_t * instance = (custom_service_t *) context ;
	// Send a notification
	att_server_notify(instance->con_handle, instance->characteristic_b_handle, instance->characteristic_b_value, strlen(instance->characteristic_b_value));
}
static void characteristic_c_callback(void * context) {
	// Associate the void pointer input with our custom service object
	custom_service_t * instance = (custom_service_t *) context ;
	// Send a notification
	att_server_notify(instance->con_handle, instance->characteristic_c_handle, instance->characteristic_c_value, strlen(instance->characteristic_c_value));
}
static void characteristic_d_callback(void * context) {
	// Associate the void pointer input with our custom service object
	custom_service_t * instance = (custom_service_t *) context ;
	// Send a notification
	att_server_notify(instance->con_handle, instance->characteristic_d_handle, instance->characteristic_d_value, strlen(instance->characteristic_d_value));
}


// Read callback (no client configuration handles on characteristics without Notify)
static uint16_t custom_service_read_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
	UNUSED(con_handle);

	// Characteristic A
	if (attribute_handle == service_object.characteristic_a_handle){
		return att_read_callback_handle_blob(service_object.characteristic_a_value, strlen(service_object.characteristic_a_value), offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_a_client_configuration_handle){
		return att_read_callback_handle_little_endian_16(service_object.characteristic_a_client_configuration, offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_a_user_description_handle) {
		return att_read_callback_handle_blob(service_object.characteristic_a_user_description, strlen(service_object.characteristic_a_user_description), offset, buffer, buffer_size);
	}

	// Characteristic B
	if (attribute_handle == service_object.characteristic_b_handle){
		return att_read_callback_handle_blob(service_object.characteristic_b_value, strlen(service_object.characteristic_b_value), offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_b_client_configuration_handle){
		return att_read_callback_handle_little_endian_16(service_object.characteristic_b_client_configuration, offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_b_user_description_handle) {
		return att_read_callback_handle_blob(service_object.characteristic_b_user_description, strlen(service_object.characteristic_b_user_description), offset, buffer, buffer_size);
	}

	// Characteristic C
	if (attribute_handle == service_object.characteristic_c_handle){
		return att_read_callback_handle_blob(service_object.characteristic_c_value, strlen(service_object.characteristic_c_value), offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_c_client_configuration_handle){
		return att_read_callback_handle_little_endian_16(service_object.characteristic_c_client_configuration, offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_c_user_description_handle) {
		return att_read_callback_handle_blob(service_object.characteristic_c_user_description, strlen(service_object.characteristic_c_user_description), offset, buffer, buffer_size);
	}

	// Characteristic D
	if (attribute_handle == service_object.characteristic_d_handle){
		return att_read_callback_handle_blob(service_object.characteristic_d_value, strlen(service_object.characteristic_d_value), offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_d_client_configuration_handle){
		return att_read_callback_handle_little_endian_16(service_object.characteristic_d_client_configuration, offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_d_user_description_handle) {
		return att_read_callback_handle_blob(service_object.characteristic_d_user_description, strlen(service_object.characteristic_d_user_description), offset, buffer, buffer_size);
	}

	// Characteristic E
	if (attribute_handle == service_object.characteristic_e_handle){
		return att_read_callback_handle_blob(service_object.characteristic_e_value, strlen(service_object.characteristic_e_value), offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_e_user_description_handle) {
		return att_read_callback_handle_blob(service_object.characteristic_e_user_description, strlen(service_object.characteristic_e_user_description), offset, buffer, buffer_size);
	}

	// Characteristic E
	if (attribute_handle == service_object.characteristic_f_handle){
		return att_read_callback_handle_blob(service_object.characteristic_f_value, strlen(service_object.characteristic_f_value), offset, buffer, buffer_size);
	}
	if (attribute_handle == service_object.characteristic_f_user_description_handle) {
		return att_read_callback_handle_blob(service_object.characteristic_f_user_description, strlen(service_object.characteristic_f_user_description), offset, buffer, buffer_size);
	}

	return 0;
}

// Write callback
static int custom_service_write_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){
	UNUSED(transaction_mode);
	UNUSED(offset);
	UNUSED(buffer_size);

	// Enable/disable notifications
	if (attribute_handle == service_object.characteristic_a_client_configuration_handle){
		service_object.characteristic_a_client_configuration = little_endian_read_16(buffer, 0);
		service_object.con_handle = con_handle;
	}
	// Enable/disable notifications
	if (attribute_handle == service_object.characteristic_b_client_configuration_handle){
		service_object.characteristic_b_client_configuration = little_endian_read_16(buffer, 0);
		service_object.con_handle = con_handle;
	}

	// Write characteristic value
	if (attribute_handle == service_object.characteristic_b_handle) {
		custom_service_t * instance = &service_object ;
		buffer[buffer_size] = 0 ;
		memset(service_object.characteristic_b_value, 0, strlen(service_object.characteristic_b_value)) ;
		memcpy(service_object.characteristic_b_value, buffer, strlen(buffer)) ;
		// If client has enabled notifications, register a callback
		if (instance->characteristic_b_client_configuration) {
			instance->callback_b.callback = &characteristic_b_callback ;
			instance->callback_b.context = (void*) instance ;
			att_server_register_can_send_now_callback(&instance->callback_b, instance->con_handle) ;
		}
		// Alert the application of a bluetooth RX
        PT_SEM_SAFE_SIGNAL(pt, &BLUETOOTH_READY) ;
	}

	// Enable/disable notificatins
	if (attribute_handle == service_object.characteristic_c_client_configuration_handle){
		service_object.characteristic_c_client_configuration = little_endian_read_16(buffer, 0);
		service_object.con_handle = con_handle;
	}

	// Enable/disable notificatins
	if (attribute_handle == service_object.characteristic_d_client_configuration_handle){
		service_object.characteristic_d_client_configuration = little_endian_read_16(buffer, 0);
		service_object.con_handle = con_handle;
	}

	// Write characteristic value
	if (attribute_handle == service_object.characteristic_d_handle) {
		custom_service_t * instance = &service_object ;
		buffer[buffer_size] = 0 ;
		memset(service_object.characteristic_d_value, 0, sizeof(service_object.characteristic_d_value)) ;
		memcpy(service_object.characteristic_d_value, buffer, buffer_size) ;
		// Null-terminate the string
		service_object.characteristic_d_value[buffer_size] = 0 ;
		// If client has enabled notifications, register a callback
		if (instance->characteristic_d_client_configuration) {
			instance->callback_d.callback = &characteristic_d_callback ;
			instance->callback_d.context = (void*) instance ;
			att_server_register_can_send_now_callback(&instance->callback_d, instance->con_handle) ;
		}
		// Alert the application of a bluetooth RX
        PT_SEM_SAFE_SIGNAL(pt, &BLUETOOTH_READY) ;
	}

	// Write characteristic value
	if (attribute_handle == service_object.characteristic_e_handle) {
		custom_service_t * instance = &service_object ;
		buffer[buffer_size] = 0 ;
		if (!strcmp(buffer, "OFF")) {
			memset(service_object.characteristic_e_value, 0, sizeof(service_object.characteristic_e_value)) ;
			memcpy(service_object.characteristic_e_value, buffer, buffer_size) ;
			service_object.characteristic_e_value[buffer_size] = 0 ;
			cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
			// Alert the application of a bluetooth RX
			PT_SEM_SAFE_SIGNAL(pt, &BLUETOOTH_READY) ;
		}
		else if (!strcmp(buffer, "ON")) {
			memset(service_object.characteristic_e_value, 0, sizeof(service_object.characteristic_e_value)) ;
			memcpy(service_object.characteristic_e_value, buffer, buffer_size) ;
			service_object.characteristic_e_value[buffer_size] = 0 ;
			cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
			// Alert the application of a bluetooth RX
			PT_SEM_SAFE_SIGNAL(pt, &BLUETOOTH_READY) ;
		}
	}

	// Write characteristic value
	if (attribute_handle == service_object.characteristic_f_handle) {
		custom_service_t * instance = &service_object ;
		buffer[buffer_size] = 0 ;
		if(atoi(buffer)<16) {
			memset(service_object.characteristic_f_value, 0, strlen(service_object.characteristic_f_value)) ;
			memcpy(service_object.characteristic_f_value, buffer, strlen(buffer)) ;
			// Null-terminate the string
			service_object.characteristic_f_value[buffer_size] = 0 ;
			// Alert the application of a bluetooth RX
        	PT_SEM_SAFE_SIGNAL(pt, &BLUETOOTH_READY) ;
		}
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
////////////////////////////// USER API /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Initialize our custom service handler
void custom_service_server_init(char * a_ptr, char * b_ptr, char * c_ptr, char * d_ptr, char * e_ptr, char * f_ptr){

	// Initialize the semaphore
	PT_SEM_SAFE_INIT(&BLUETOOTH_READY, 0) ;

	// Pointer to our service object
	custom_service_t * instance = &service_object ;

	// Assign characteristic value
	instance->characteristic_a_value = a_ptr ;
	instance->characteristic_b_value = b_ptr ;
	instance->characteristic_c_value = c_ptr ;
	instance->characteristic_d_value = d_ptr ;
	instance->characteristic_e_value = e_ptr ;
	instance->characteristic_f_value = f_ptr ;

	// Assign characteristic user description
	instance->characteristic_a_user_description = characteristic_a ;
	instance->characteristic_b_user_description = characteristic_b ;
	instance->characteristic_c_user_description = characteristic_c ;
	instance->characteristic_d_user_description = characteristic_d ;
	instance->characteristic_e_user_description = characteristic_e ;
	instance->characteristic_f_user_description = characteristic_f ;

	// Assign handle values (from generated gatt header)
	instance->characteristic_a_handle = ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE ;
	instance->characteristic_a_client_configuration_handle = ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE ;
	instance->characteristic_a_user_description_handle = ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE ;

	instance->characteristic_b_handle = ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE ;
	instance->characteristic_b_client_configuration_handle = ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE ;
	instance->characteristic_b_user_description_handle = ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE ;

	instance->characteristic_c_handle = ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE ;
	instance->characteristic_c_client_configuration_handle = ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE ;
	instance->characteristic_c_user_description_handle = ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE ;

	instance->characteristic_d_handle = ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE ;
	instance->characteristic_d_client_configuration_handle = ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE ;
	instance->characteristic_d_user_description_handle = ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE ;

	instance->characteristic_e_handle = ATT_CHARACTERISTIC_0000FF15_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE ;
	instance->characteristic_e_user_description_handle = ATT_CHARACTERISTIC_0000FF15_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE ;

	instance->characteristic_f_handle = ATT_CHARACTERISTIC_0000FF16_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE ;
	instance->characteristic_f_user_description_handle = ATT_CHARACTERISTIC_0000FF16_0000_1000_8000_00805F9B34FB_01_USER_DESCRIPTION_HANDLE ;

	// Service start and end handles (modeled off heartrate example)
	service_handler.start_handle = 0 ;
	service_handler.end_handle = 0xFFFF ;
	service_handler.read_callback = &custom_service_read_callback ;
	service_handler.write_callback = &custom_service_write_callback ;

	// Register the service handler
	att_server_register_service_handler(&service_handler);
}

// Update Characteristic A value
void set_characteristic_a_value(int value){

	// Pointer to our service object
	custom_service_t * instance = &service_object ;

	// Update field value
	sprintf(instance->characteristic_a_value, "%d", value) ;

	// Are notifications enabled? If so, register a callback
	if (instance->characteristic_a_client_configuration){
		instance->callback_a.callback = &characteristic_a_callback;
		instance->callback_a.context  = (void*) instance;
		att_server_register_can_send_now_callback(&instance->callback_a, instance->con_handle);;
	}
}

// Update Characteristic C value
void set_characteristic_c_value(char * c_ptr) {

	// Pointer to our service object
	custom_service_t * instance = &service_object ;

	// Point the c characteristic value to our character array input
	instance->characteristic_c_value = c_ptr ;

	// If client has enabled notifications, register a callback
	if (instance->characteristic_c_client_configuration) {
		instance->callback_c.callback = &characteristic_c_callback ;
		instance->callback_c.context = (void*) instance ;
		att_server_register_can_send_now_callback(&instance->callback_c, instance->con_handle) ;
	}

}
































