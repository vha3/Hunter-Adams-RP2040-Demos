///////////////////////////////////////////////////////////////////////////////
//////////////////////////// USER-MODIFIED PARAMETERS /////////////////////////
///////////////////////////////////////////////////////////////////////////////
// The size of the UDP payload (measured in bytes, set at compile time).
// This is what establishes the length of the udp_payload character array.
#define DEF_UDP_PAYLOAD_SIZE    (18)

// UDP payload (modified at runtime! this is just an initialization)
// This is the character array that you modify in your application code.
char udp_payload[DEF_UDP_PAYLOAD_SIZE] = "Hello from Pico: 0" ;

// Ethernet destination address (MAC)
unsigned char ethernet_destination[6] = {0x8c, 0xae, 0x4c, 0xde, 0x72, 0x91};
// Ethernet source address (auto-generated from Pico's unique flash ID)
// If you want to specify this, comment out GenerateUniqueMAC() from AssemblePacket().
unsigned char ethernet_source[6]      = {} ;

// IP source and destination addresses. Source address will be that of the Pico.
unsigned char ip_source[4]  = {169, 254, 123, 101};
unsigned char ip_dest[4]    = {169, 254, 4, 174};

// UPD source and destination ports (set to a default of 1024)
unsigned char udp_src_port[2] = {0x04, 0x00} ; // 1024
unsigned char udp_dst_port[2] = {0x04, 0x00} ; // 1024