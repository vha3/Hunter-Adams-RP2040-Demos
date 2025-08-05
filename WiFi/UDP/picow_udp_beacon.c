/**
 * Copyright (c) 2022 Andrew McDonnell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Standard libraries
#include <string.h>
#include "pico/sync.h"
// LWIP libraries
#include "lwip/pbuf.h"
#include "lwip/udp.h"
// Pico SDK hardware support libraries
#include "hardware/sync.h"
// Our header for making WiFi connection
#include "connect.h"
// Protothreads
#include "pt_cornell_rp2040_v1_4.h"

// Destination port and IP address
#define UDP_PORT 1234
#define BEACON_TARGET "172.20.10.2"

// Maximum length of our message
#define BEACON_MSG_LEN_MAX 127

// Protocol control block for UDP receive connection
static struct udp_pcb *udp_rx_pcb;

// Buffer in which to copy received messages
char received_data[BEACON_MSG_LEN_MAX] ;

// Semaphore for signaling a new received message
semaphore_t new_message ;

static void udpecho_raw_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                 const ip_addr_t *addr, u16_t port)
{
  LWIP_UNUSED_ARG(arg);

  // Check that there's something in the pbuf
  if (p != NULL) {
    // Copy the contents of the payload
    memcpy(received_data, p->payload, BEACON_MSG_LEN_MAX) ;
    // Semaphore-signal a thread
    PT_SEM_SDK_SIGNAL(pt, &new_message) ;
    // Reset the payload buffer
    memset(p->payload, 0, BEACON_MSG_LEN_MAX+1);
    // Free the PBUF
    pbuf_free(p);
  }
  else printf("NULL pt in callback");
}

// ===================================
// Define the recv callback 
void  udpecho_raw_init(void)
{

  // Initialize the RX protocol control block
  udp_rx_pcb  = udp_new_ip_type(IPADDR_TYPE_ANY);

  // Make certain that the pcb has initialized, else print a message
  if (udp_rx_pcb != NULL) {
    // Err_t object for error codes
    err_t err;
    // Bind this PCB to our assigned IP, and our chosen port. Received messages
    // to this port will be directed to our bound protocol control block.
    err = udp_bind(udp_rx_pcb, netif_ip_addr4(netif_default), UDP_PORT);
    // Check that the bind was successful, else print a message
    if (err == ERR_OK) {
      // Setup the receive callback function
      udp_recv(udp_rx_pcb, udpecho_raw_recv, NULL);
    } else {
      printf("bind error");
    }
  } else {
    printf("udp_rx_pcb error");
  }
}

static PT_THREAD (protothread_receive(struct pt *pt))
{
  // Begin thread
  PT_BEGIN(pt) ;

  while(1) {
    // Wait on a semaphore
    PT_SEM_SDK_WAIT(pt, &new_message) ;

    // Print received message
    printf("%s\n", received_data);

  }

  // End thread
  PT_END(pt) ;
}

static PT_THREAD (protothread_send(struct pt *pt))
{
    // Begin thread
    PT_BEGIN(pt) ;

    // Make a static local UDP protocol control block
    static struct udp_pcb* pcb;
    // Initialize that protocol control block
    pcb = udp_new() ;

    // Create a static local IP_ADDR_T object
    static ip_addr_t addr;
    // Set the value of this IP address object to our destination IP address
    ipaddr_aton(BEACON_TARGET, &addr);

    while(1) {
        // Prompt the user
        sprintf(pt_serial_out_buffer, "> ");
        serial_write ;

        // Perform a non-blocking serial read for a string
        serial_read ;

        // Allocate a PBUF
        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, BEACON_MSG_LEN_MAX+1, PBUF_RAM);

        // Pointer to the payload of the pbuf
        char *req = (char *)p->payload;

        // Clear the pbuf payload
        memset(req, 0, BEACON_MSG_LEN_MAX+1);

        // Fill the pbuf payload
        snprintf(req, BEACON_MSG_LEN_MAX, "%s: %s \n",
          ip4addr_ntoa(netif_ip_addr4(netif_default)), pt_serial_in_buffer);

        // Send the UDP packet
        err_t er = udp_sendto(pcb, p, &addr, UDP_PORT);

        // Free the PBUF
        pbuf_free(p);

        // Check for errors
        if (er != ERR_OK) {
            printf("Failed to send UDP packet! error=%d", er);
        } 
    }
    // End thread
    PT_END(pt) ;
}


int main() {

    // Initialize stdio
    stdio_init_all();

    // Connect to WiFi
    if (connectWifi(country, WIFI_SSID, WIFI_PASSWORD, auth)) {
      printf("Failed connection.\n") ;
    }

    // Initialize semaphore
    sem_init(&new_message, 0, 1) ;

    //============================
    // UDP recenve ISR routines
    udpecho_raw_init();

    // Add threads, start scheduler
    pt_add_thread(protothread_send) ;
    pt_add_thread(protothread_receive) ;
    pt_schedule_start ;

    return 0;
}