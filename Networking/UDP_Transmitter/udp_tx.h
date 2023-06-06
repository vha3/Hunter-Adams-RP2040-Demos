/**
 * V. Hunter Adams (vha3@cornell.edu)
 * 
 * Non-blocking UDP transmitter for RP2040
 * 
 * Hardware connections
 * - GPIO 3 -----> TX+
 * - GPIO 2 -----> TX-
 * 
 * Resources utilized
 * - All DMA channels
 * - PIO state machines 0, 1, and 2 on PIO block 0
 * - PWM slice 7
 */

#include "pico/unique_id.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "udp_transmit.pio.h"
#include "udp_tx_parameters.h"


///////////////////////////////////////////////////////////////////////////////
////////////////////////// Probably do not need to modify /////////////////////
///////////////////////////////////////////////////////////////////////////////
// Payload + Eth CRC (4) + UDP CRC (2) + UPD len (2)
#define DEF_UDP_LEN (DEF_UDP_PAYLOAD_SIZE + 8)

// Payload + Eth CRC (4) + UDP CRC (2) + UPD len (2) + IP header (20) + Eth (14)
#define PACKET_LEN  (DEF_UDP_PAYLOAD_SIZE + 8 + 20 + 14)

// Length of preamble
#define PREAMBLE_LEN 8

// Length of Ethernet checksum (computed by DMA sniffer)
#define CRC_LEN 4

// Preamble, byte aligned for DMA read address wrapping
unsigned char preamble[PREAMBLE_LEN] __attribute__ ((aligned (8))) = {  0x55, 0x55, 0x55, 0x55,
                                                                        0x55, 0x55, 0x55, 0xD5} ;
unsigned char * preamble_pointer = &preamble[0] ;

// Ethernet type (IP)
unsigned char ethernet_type[2] = {0x08, 0x00} ;

// IP Version (v4) and header length (5, for 20 bytes, which is 5 32-bit increments)
#define IP_VERSION 4 
#define IP_HEAD_LEN 5 
unsigned char ip_v_and_head_len = (((IP_VERSION<<4) & 0xF0) | (IP_HEAD_LEN & 0x0F)) ;

// IP type of service (outine precedence, normal delay)
static const uint8_t ip_type_of_service = 0 ;

// Total length of packet (20 bytes for header, plus UDP length)
static const unsigned char ip_total_len[2] = {  ((DEF_UDP_LEN + 20)>>8) & 0xFF,
                                                ((DEF_UDP_LEN + 20)) & 0xFF} ;

// The IP identifier. Could be different for each packet. If the IP packet is fragmented,
// each will use the same identification number. For testing, this is of a fixed value.
unsigned char ip_identifier[2] = {0xB3, 0xFE} ;

// IP flags and fragment offset (we aren't fragmenging packets)
static const unsigned char ip_flags_and_fragment[2] = {0x00, 0x00} ;

// IP time to live. Decremented each pass thru router, dropped at 0
static const uint8_t ip_time_to_live = 0x80 ;

// IP prototocol (set to UDP)
static const uint8_t ip_protocol = 0x11 ;

// UDP payload length
unsigned char udp_payload_len[2] = {((DEF_UDP_LEN >> 8) & 0xFF), (DEF_UDP_LEN & 0xFF)} ;

// UDP payload checksum (must be set to zero if not used)
unsigned char udp_checksum[2] = {0x00, 0x00};//{0x2D, 0xE8} ;

// This is where we will store the ethernet CRC (computed by DMA sniffer)
// Byte aligned for read pointer wrapping on the DMA channel
unsigned char crc_dest[4] __attribute__ ((aligned (8))) = {0} ;

// The packet which we populate and transmit
unsigned char assembled_packet[PACKET_LEN] = {} ;
unsigned char * assembled_packet_pointer = &assembled_packet[0] ;


// Unique board ID for MAC address generation
pico_unique_board_id_t board_identification ;


///////////////////////////////////////////////////////////////////////////////
///////////////////////// Functions for setting up packet /////////////////////
///////////////////////////////////////////////////////////////////////////////
// In the event that source/destination addresses are not hard-coded, this function
// will prompt the user to provide that information.
void GetNetworkData() {
    char str1[50] ;
    char s[2] = "." ;
    char * token ;
    unsigned char val ;

    printf("Enter IP destination address, separated by dots, in decimal:\n");
    scanf("%15s", str1) ;
    printf("%s\n", str1) ;
    token = strtok(str1, s) ;

    int i = 0 ;

    while( token != NULL ) {
      val = atoi(token) ;
      ip_dest[i] = val ;
      i += 1 ;
      token = strtok(NULL, s);
   }

    printf("Enter IP source address, separated by dots, in decimal:\n");
    scanf("%15s", str1) ;
    printf("%s\n", str1) ;
    token = strtok(str1, s) ;

    i = 0 ;

    while( token != NULL ) {
      val = atoi(token) ;
      ip_source[i] = val ;
      i += 1 ;
      token = strtok(NULL, s);
    }

    printf("Enter destination MAC address, separated by dots, in hexadecimal:\n");
    scanf("%18s", str1) ;
    printf("%s\n", str1) ;
    token = strtok(str1, s) ;

    i = 0 ;

    while( token != NULL ) {
        val = strtol(token, NULL, 16) ;
        ethernet_destination[i] = val ;
        i += 1 ;
        token = strtok(NULL, s);
    }

    printf("Enter source MAC address, separated by dots, in hexadecimal:\n");
    scanf("%18s", str1) ;
    printf("%s\n", str1) ;
    token = strtok(str1, s) ;

    i = 0 ;

    while( token != NULL ) {
        val = strtol(token, NULL, 16) ;
        ethernet_source[i] = val ;
        i += 1 ;
        token = strtok(NULL, s);
    }
}


void GenerateUniqueMAC() {
    // Get the unique flash memory ID, we'll use this for our MAC address
    pico_get_unique_board_id(&board_identification) ;
    // Populate ethernet source address with first 6 bytes of unique ID
    for (int i=0; i<5; i++) {
        ethernet_source[i] = board_identification.id[i] ;
    }
}

// Generates the ethernet header
void BuildEthernetHeader() {
    // Copy over ethernet destination
    memcpy(&assembled_packet[0],  &ethernet_destination[0], 6) ;
    // Copy over ethernet source (unique to RP2040)
    memcpy(&assembled_packet[6],  &ethernet_source[0], 6) ;
    // Copy over ethernet type
    memcpy(&assembled_packet[12], &ethernet_type[0], 2) ;
}

// Generates the IP header
void BuildIPHeader() {
    // Some local variables for computing the IP checksum
    unsigned short ip_crc_array[9] = {} ;
    unsigned int ip_chk_sum1 = 0;
    unsigned int ip_chk_sum2 = 0;
    unsigned int ip_chk_sum3 = 0;
    unsigned short final_checksum = 0 ;
    unsigned char ip_checksum_array[2] = {} ;

    // Copy over IP version and head len, and type of service
    memcpy(&assembled_packet[14], &ip_v_and_head_len, 1) ;
    memcpy(&assembled_packet[15], &ip_type_of_service, 1) ;
    ip_crc_array[0] = ((assembled_packet[14]<<8) & 0xFF00) | (assembled_packet[15] & 0x00FF) ;

    // Copy over IP total length
    memcpy(&assembled_packet[16], &ip_total_len, 2) ;
    memcpy(&ip_crc_array[1], &assembled_packet[16], 2) ;
    ip_crc_array[1] = ((assembled_packet[16]<<8) & 0xFF00) | (assembled_packet[17] & 0x00FF) ;

    // Copy over IP identifier
    memcpy(&assembled_packet[18], &ip_identifier, 2) ;
    ip_crc_array[2] = ((assembled_packet[18]<<8) & 0xFF00) | (assembled_packet[19] & 0x00FF) ;

    // Copy over IP flags and fragmentation settings
    memcpy(&assembled_packet[20], &ip_flags_and_fragment, 2) ;
    ip_crc_array[3] = ((assembled_packet[20]<<8) & 0xFF00) | (assembled_packet[21] & 0x00FF) ;

    // Copy over IP time to live and protocol
    memcpy(&assembled_packet[22], &ip_time_to_live, 1) ;
    memcpy(&assembled_packet[23], &ip_protocol, 1) ;
    ip_crc_array[4] = ((assembled_packet[22]<<8) & 0xFF00) | (assembled_packet[23] & 0x00FF) ;

    // Next is checksum. Skip for now! We still need to compute it.

    // Copy over IP source address
    memcpy(&assembled_packet[26], &ip_source, 4) ;
    ip_crc_array[5] = ((assembled_packet[26]<<8) & 0xFF00) | (assembled_packet[27] & 0x00FF) ;
    ip_crc_array[6] = ((assembled_packet[28]<<8) & 0xFF00) | (assembled_packet[29] & 0x00FF) ;

    // Copy over IP destination address
    memcpy(&assembled_packet[30], &ip_dest, 4) ;
    ip_crc_array[7] = ((assembled_packet[30]<<8) & 0xFF00) | (assembled_packet[31] & 0x00FF) ;
    ip_crc_array[8] = ((assembled_packet[32]<<8) & 0xFF00) | (assembled_packet[33] & 0x00FF) ;

    // Compute the checksum
    for (int i=0; i<9; i++) {
        ip_chk_sum1 += ip_crc_array[i] ;
    }
    ip_chk_sum2 = (ip_chk_sum1 & 0x0000FFFF) + (ip_chk_sum1 >> 16);
    ip_chk_sum3 = (ip_chk_sum2 & 0x0000FFFF) + (ip_chk_sum2 >> 16);
    final_checksum = ~ip_chk_sum3 ;

    // Populate a local checksum array
    ip_checksum_array[0] = (final_checksum>>8) & 0xFF ;
    ip_checksum_array[1] = (final_checksum & 0xFF) ;

    // Copy local array over to the packet
    memcpy(&assembled_packet[24], &ip_checksum_array[0], 2) ;
}

// Generates the UDP header
void BuildUDPHeader() {
    memcpy(&assembled_packet[34], &udp_src_port, 2) ;
    memcpy(&assembled_packet[36], &udp_dst_port, 2) ;
    memcpy(&assembled_packet[38], &udp_payload_len, 2) ;
    memcpy(&assembled_packet[40], &udp_checksum, 2) ;
}

// Memcopy's over the UDP payload
void SetUDPData() {
    memcpy(&assembled_packet[42], &udp_payload[0], DEF_UDP_PAYLOAD_SIZE) ;
    // for (int i=0; i<DEF_UDP_PAYLOAD_SIZE; i++) {
        // assembled_packet[42+i] = udp_payload[i] ;
    // }
}

void AssemblePacket() {
    GenerateUniqueMAC() ;
    BuildEthernetHeader() ;
    BuildIPHeader() ;
    BuildUDPHeader() ;
    SetUDPData() ;
}


////////////////////////// PIO params ////////////////////////////
// We'll use PIO0
PIO pio = pio0;


////////////////////////// PWM params ////////////////////////////
// Setup for wrap every 16ms (triggers an NLP)
#define WRAPVAL 60000
#define CLKDIV  64


/////////////////// DMA-utilized variables //////////////////////
// Name the DMA channels
int chan_0 = 0 ;
int chan_1 = 1 ;
int chan_2 = 2 ;
int chan_3 = 3 ;
int chan_4 = 4 ;
int chan_5 = 5 ;
int chan_6 = 6 ;
int chan_7 = 7 ;
int chan_8 = 8 ;
int chan_9 = 9 ;
int chan_10 = 10 ;
int chan_11 = 11 ;
// Change these if you want to use other PWM channels
// These values are DMA's to PWM control registers
unsigned int pwm_kill           = 0x00000000 ;
unsigned int pwm_revive         = 0x00000001 ;
unsigned int pwm_counter_reset  = 0x00000000 ;
// Initial value for DMA sniffer result register
unsigned int sniff_init = 0xffffffff ;
// Dummy data for NLP DMA channel
unsigned int nlp_dummy = 0xffffffff ;
// THIS NUMBER IS A MYSTERY - delay for starting TP_IDL PIO
unsigned int idl_delay = 327 ;
// Dummy source/dest for a reset DMA channel
unsigned int dummy_source = 0xffffffff ;
unsigned int dummy_dest = 0 ;


///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Our API ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define START_NLP dma_start_channel_mask((1u << chan_0))
#define SEND_PACKET SetUDPData() ; dma_start_channel_mask((1u << chan_2))



void initUDP(unsigned int txminus_pin, irq_handler_t handler) {
    ///////////////////////////////////////////////////////////////////
    ////////////////////////// PIO SETUP //////////////////////////////
    ///////////////////////////////////////////////////////////////////
    // State machines (on PIO0)
    uint sm_tx  = 0 ;
    uint sm_nlp = 1 ;
    uint sm_idl = 2 ;

    // Load programs into instruction memory
    uint offset_tx  = pio_add_program(pio, &manchester_tx_program);
    uint offset_nlp = pio_add_program(pio, &nlp_program);
    uint offset_idl = pio_add_program(pio, &tp_idl_program) ;

    // Initialize PIO programs with appropriate clock divider values
    manchester_tx_program_init(pio, sm_tx, offset_tx, txminus_pin, 3.f);
    tp_idl_program_init(pio, sm_idl, offset_idl, txminus_pin, 3.f);
    nlp_program_init(pio, sm_nlp, offset_nlp, txminus_pin, 4.f);


    ///////////////////////////////////////////////////////////////////
    ////////////////////////// PWM SETUP //////////////////////////////
    ///////////////////////////////////////////////////////////////////
    // Slice number chosen arbitrarily
    int slice_num = 7 ;

    // Experimentation shows we don't need to map this to a GPIO
    // or configure a particular duty cycle. Configured for a wraptime
    // of 16ms (NLP interval)
    pwm_set_wrap(slice_num, WRAPVAL) ;
    pwm_set_clkdiv(slice_num, CLKDIV) ;
    pwm_set_enabled(slice_num, true) ;

    ///////////////////////////////////////////////////////////////////
    ////////////////////////// DMA NLP SETUP //////////////////////////
    ///////////////////////////////////////////////////////////////////

    // Triggers the NLP machine, started by PWM watchdog channel
    dma_channel_config c0 = dma_channel_get_default_config(chan_0); // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_32);        // 32-bit txfers
    channel_config_set_read_increment(&c0, false);                  // no read incrementing
    channel_config_set_write_increment(&c0, false);                 // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PWM_WRAP7) ;                  // DREQ_PWM_WRAP7 pacing
    channel_config_set_chain_to(&c0, chan_1);                       // chain to chan 1

    dma_channel_configure(
        chan_0,                // Channel to be configured
        &c0,                   // The configuration we just created
        &pio->txf[sm_nlp],     // write address (NLP PIO TX FIFO)
        &nlp_dummy,            // The initial read address (dummy value)
        1,                     // Number of transfers; in this case each is 4 byte.
        false                  // Don't start immediately.
    );

    // Channel One (resets NLP pulse machine)
    dma_channel_config c1 = dma_channel_get_default_config(chan_1);  // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);         // 32-bit txfers
    channel_config_set_read_increment(&c1, false);                   // no read incrementing
    channel_config_set_write_increment(&c1, false);                  // no write incrementing
    channel_config_set_chain_to(&c1, chan_0);                        // chain back to chan 0

    dma_channel_configure(
        chan_1,            // Channel to be configured
        &c1,               // The configuration we just created
        &dummy_dest,       // write address (dummy)
        &dummy_source,     // The initial read address (dummy)
        1,                 // Number of transfers; in this case each is 4 byte.
        false              // Don't start immediately.
    );

    ///////////////////////////////////////////////////////////////////
    ////////////////////////// DMA PACKET SETUP ///////////////////////
    ///////////////////////////////////////////////////////////////////
    // Disable the PWM channel
    dma_channel_config c2 = dma_channel_get_default_config(chan_2);  // default configs
    channel_config_set_transfer_data_size(&c2, DMA_SIZE_32);         // 32-bit txfers
    channel_config_set_read_increment(&c2, false);                   // no read incrementing
    channel_config_set_write_increment(&c2, false);                  // no write incrementing
    channel_config_set_chain_to(&c2, chan_3);                        // chain to channel 3

    dma_channel_configure(
        chan_2,                                 // Channel to be configured
        &c2,                                    // The configuration we just created
        &pwm_hw->slice[slice_num].csr,          // write address (csr reg of pwm)
        &pwm_kill,                              // The initial read address (zero variable)
        1,                                      // Number of transfers; in this case each is 4 byte.
        false                                   // Don't start immediately.
    );

    // Zero PWM counter
    dma_channel_config c3 = dma_channel_get_default_config(chan_3);  // default configs
    channel_config_set_transfer_data_size(&c3, DMA_SIZE_32);         // 32-bit txfers
    channel_config_set_read_increment(&c3, false);                   // no read incrementing
    channel_config_set_write_increment(&c3, false);                  // no write incrementing
    channel_config_set_chain_to(&c3, chan_4);                        // chain to channel 4

    dma_channel_configure(
        chan_3,                                 // Channel to be configured
        &c3,                                    // The configuration we just created
        &pwm_hw->slice[slice_num].ctr,          // write address (pwm counter register)
        &pwm_counter_reset,                     // The initial read address (zero variable)
        1,                                      // Number of transfers; in this case each is 4 byte.
        false                                   // Don't start immediately.
    );

    // Reset the sniffer accumulator
    dma_channel_config c4 = dma_channel_get_default_config(chan_4); // default configs
    channel_config_set_transfer_data_size(&c4, DMA_SIZE_32);        // 32-bit txfers
    channel_config_set_read_increment(&c4, false);                  // no read incrementing
    channel_config_set_write_increment(&c4, false);                 // no write incrementing
    channel_config_set_chain_to(&c4, chan_5);                       // chain to channel 5

    dma_channel_configure(
        chan_4,                // Channel to be configured
        &c4,                   // The configuration we just created
        &dma_hw->sniff_data,   // write address (sniffer data reg)
        &sniff_init,           // The initial read address (variable containing 0xffffffff)
        1,                     // Number of transfers; in this case each is 4 byte.
        false                  // Don't start immediately.
    );

    // Reset packet read pointer
    dma_channel_config c5 = dma_channel_get_default_config(chan_5);  // default configs
    channel_config_set_transfer_data_size(&c5, DMA_SIZE_32);         // 32-bit txfers
    channel_config_set_read_increment(&c5, false);                   // no read incrementing
    channel_config_set_write_increment(&c5, false);                  // no write incrementing
    channel_config_set_chain_to(&c5, chan_6);                        // chain to channel 6

    dma_channel_configure(
        chan_5,                                 // Channel to be configured
        &c5,                                    // The configuration we just created
        &dma_hw->ch[chan_7].read_addr,          // write address (dma 7 read address)
        &assembled_packet_pointer,              // The initial read address (pointer to address)
        1,                                      // Number of transfers; in this case each is 4 byte.
        false                                   // Don't start immediately.
    );

    // Do preamble transaction (8 bytes long, ring wrap to avoid a read pointer reset)
    dma_channel_config c6 = dma_channel_get_default_config(chan_6); // default configs
    channel_config_set_transfer_data_size(&c6, DMA_SIZE_8);         // 8-bit txfers
    channel_config_set_read_increment(&c6, true);                   // yes read incrementing
    channel_config_set_write_increment(&c6, false);                 // no write incrementing
    channel_config_set_ring(&c6, false, 3) ;                        // ring wrap read address!
    channel_config_set_dreq(&c6, DREQ_PIO0_TX0) ;                   // DREQ_PIO0_TX0 pacing (FIFO)
    channel_config_set_chain_to(&c6, chan_7);                       // chain to channel 7

    dma_channel_configure(
        chan_6,                 // Channel to be configured
        &c6,                    // The configuration we just created
        &pio->txf[sm_tx],       // write address (TX FIFO for packet serializer PIO)
        &preamble[0],           // The initial read address (pointer to character array)
        PREAMBLE_LEN,           // Number of transfers; in this case each is 1 byte.
        false                   // Don't start immediately.
    );

    // Do packet transaction - add sniffer here!
    // Finicky setup. See link below.
    // Don't do bswaps because 8 bit txfers. Otherwise might need them.
    // https://github.com/raspberrypi/pico-feedback/issues/247
    dma_channel_config c7 = dma_channel_get_default_config(chan_7); // default configs
    channel_config_set_transfer_data_size(&c7, DMA_SIZE_8);         // 8-bit txfers
    channel_config_set_read_increment(&c7, true);                   // yes read incrementing
    channel_config_set_write_increment(&c7, false);                 // no write incrementing
    channel_config_set_dreq(&c7, DREQ_PIO0_TX0) ;                   // DREQ_PIO0_TX0 pacing (FIFO)
    channel_config_set_chain_to(&c7, chan_8);                       // chain to channel 8

    dma_channel_configure(
        chan_7,                 // Channel to be configured
        &c7,                    // The configuration we just created
        &pio->txf[sm_tx],       // write address (TX FIFO for packet serializer PIO)
        &assembled_packet[0],   // The initial read address (pointer to packet array)
        PACKET_LEN,             // Number of transfers; in this case each is 1 byte.
        false                   // Don't start immediately.
    );

    // Configure the sniffer! Rather tricky setup, this is what worked.
    dma_sniffer_enable(chan_7, 1, true);
    hw_set_bits(&dma_hw->sniff_ctrl, (DMA_SNIFF_CTRL_OUT_INV_BITS | DMA_SNIFF_CTRL_OUT_REV_BITS));

    // Send sniffed CRC to character array 
    dma_channel_config c8 = dma_channel_get_default_config(chan_8); // default configs
    channel_config_set_transfer_data_size(&c8, DMA_SIZE_32);        // 32-bit txfers
    channel_config_set_read_increment(&c8, false);                  // no read incrementing
    channel_config_set_write_increment(&c8, false);                 // no write incrementing
    channel_config_set_bswap(&c8, false);                           // not necessary b/c char array
    channel_config_set_chain_to(&c8, chan_9);                       // chain to channel 9

    dma_channel_configure(
        chan_8,                // Channel to be configured
        &c8,                   // The configuration we just created
        &crc_dest[0],          // write address (checksum buffer character array)
        &dma_hw->sniff_data,   // The initial read address (DMA sniffer data)
        1,                     // Number of transfers; in this case each is 4 byte.
        false                  // Don't start immediately.
    );

    // Send sniffed character array to PIO (use ring wrap to avoid a reset channel)
    dma_channel_config c9 = dma_channel_get_default_config(chan_9); // default configs
    channel_config_set_transfer_data_size(&c9, DMA_SIZE_8);         // 8-bit txfers
    channel_config_set_read_increment(&c9, true);                   // yes read incrementing
    channel_config_set_write_increment(&c9, false);                 // no write incrementing
    channel_config_set_ring(&c9, false, 2) ;                        // ring wrap read addrsses!
    channel_config_set_dreq(&c9, DREQ_PIO0_TX0) ;                   // DREQ_PIO0_TX0 pacing (FIFO)
    channel_config_set_chain_to(&c9, chan_10);                      // chain to channel 10

    dma_channel_configure(
        chan_9,                 // Channel to be configured
        &c9,                    // The configuration we just created
        &pio->txf[sm_tx],       // write address (TX FIFO for packet serializer PIO)
        &crc_dest[0],           // The initial read address (pointer to checksum array)
        CRC_LEN,                // Number of transfers; in this case each is 1 byte.
        false                   // Don't start immediately.
    );

    // Trigger a TP_IDL pulse (another PIO machine)
    dma_channel_config c10 = dma_channel_get_default_config(chan_10);   // default configs
    channel_config_set_transfer_data_size(&c10, DMA_SIZE_32);           // 32-bit txfers
    channel_config_set_read_increment(&c10, false);                     // no read incrementing
    channel_config_set_write_increment(&c10, false);                    // no write incrementing
    channel_config_set_dreq(&c10, DREQ_PIO0_TX2) ;                      // DREQ_PIO0_TX2 pacing
    channel_config_set_chain_to(&c10, chan_11);                         // chain to channel 11

    dma_channel_configure(
        chan_10,                // Channel to be configured
        &c10,                   // The configuration we just created
        &pio->txf[sm_idl],      // write address (TP_IDL PIO state machine TX fifo)
        &idl_delay,             // The initial read address (variable holding wait time)
        1,                      // Number of transfers; in this case each is 4 byte.
        false                   // Don't start immediately.
    );

    // Revive the PWM channel after packet transaction
    dma_channel_config c11 = dma_channel_get_default_config(chan_11);   // default configs
    channel_config_set_transfer_data_size(&c11, DMA_SIZE_32);           // 32-bit txfers
    channel_config_set_read_increment(&c11, false);                     // no read incrementing
    channel_config_set_write_increment(&c11, false);                    // no write incrementing

    dma_channel_configure(
        chan_11,                           // Channel to be configured
        &c11,                              // The configuration we just created
        &pwm_hw->slice[slice_num].csr,     // write address (pwm csr register)
        &pwm_revive,                       // The initial read address (1 variable)
        1,                                 // Number of transfers; in this case each is 4 byte.
        false                              // Don't start immediately.
    );


    ///////////////////////////////////////////////////////////////////
    ////////////////////////// Sys startup ////////////////////////////
    ///////////////////////////////////////////////////////////////////
    // Start the PIO state machines
    pio_enable_sm_mask_in_sync(pio, ((1u << sm_tx) | (1u << sm_idl)));
    pio_enable_sm_mask_in_sync(pio, ((1u << sm_nlp)));

    // Setup interrupts
    pio_interrupt_clear(pio0, 1) ;
    pio_set_irq0_source_enabled(pio0, PIO_INTR_SM1_LSB, true) ;
    irq_set_exclusive_handler(PIO0_IRQ_0, handler) ;
    irq_set_enabled(PIO0_IRQ_0, true) ;

    // Start the NLP DMA channel - begins generating NLP at 16ms intervals
    START_NLP ;

    // Use that network data to assemble the packet
    AssemblePacket() ;
}




























