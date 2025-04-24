

// Includes
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// Wifi information
#define WIFI_SSID "V. Hunter Adams’s iPhone"
#define WIFI_PASSWORD ""
uint32_t country = CYW43_COUNTRY_USA ;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK ;

// Function for connecting to a WiFi access point
int connectWifi(uint32_t country, const char *ssid, const char *pass, uint32_t auth) {
    
    // Initialize the hardware
    if (cyw43_arch_init_with_country(country)) {
        printf("Failed to initialize hardware.\n") ;
        return 1 ;
    }

    // Make sure the LED is off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0) ;

    // Put the device into station mode
    cyw43_arch_enable_sta_mode() ;

    // Print a status message
    printf("Attempting connection . . . \n") ;

    // Connect to the network
    if (cyw43_arch_wifi_connect_blocking(ssid, pass, auth)) {
        return 2 ;
    }

    // Use the LED to indicate connection success
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1) ;

    // Report the IP, netmask, and gateway that we've been assigned
    printf("IP: %s\n", ip4addr_ntoa(netif_ip_addr4(netif_default))) ;
    printf("Mask: %s\n", ip4addr_ntoa(netif_ip_netmask4(netif_default))) ;
    printf("Gateway: %s\n", ip4addr_ntoa(netif_ip_gw4(netif_default))) ;

    return 0 ;
}