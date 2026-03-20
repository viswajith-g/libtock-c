#include <stdio.h>
#include <string.h>

#include <libtock/net/thread_manager.h>

#define RX_BUFFER_SIZE 128

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint32_t rx_count = 0;
static uint16_t my_app_id = 0;

// RX callback - should ONLY see our own packets (testing isolation)
static void rx_callback(uint16_t sender_app_id, uint32_t len, void* userdata) {
  (void)userdata;
  
  rx_count++;
  
  // Null-terminate for safe printing
  if (len < RX_BUFFER_SIZE) {
    rx_buffer[len] = '\0';
  }
  
  printf("\n[Sniffer]<<< Packet #%lu\n", rx_count);
  printf("[Sniffer]     From App: %u\n", sender_app_id);
  printf("[Sniffer]     My ID:   %u\n", my_app_id);
  printf("[Sniffer]     Length:      %lu bytes\n", len);
  printf("[Sniffer]     Data:        %s\n", rx_buffer);
  
  if (sender_app_id != my_app_id) {
    printf("[Sniffer]: Isolation Violation!\n");
    printf("[Sniffer]: Received packet from different app!\n");
  } else {
    printf("[Sniffer] (This is my own broadcast - OK)\n");
  }
}

int main(void) {
  returncode_t ret;

  printf("=================================\n");
  printf("Thread Network Sniffer App\n");
  printf("Testing Multi-Tenant Isolation\n");
  printf("=================================\n");

  // Check if driver exists
  if (!libtock_thread_manager_exists()) {
    printf("[Sniffer] Error: Thread network driver not found!\n");
    return -1;
  }

  // Get my app ID
  ret = libtock_thread_manager_get_app_id(&my_app_id);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Sniffer] Error: Failed to get app ID\n");
    return -1;
  }
  printf("[Sniffer] My Tenant ID: %u\n", my_app_id);

  // Configure radio (same settings as other apps)
  ret = libtock_thread_manager_set_pan_id(0xABCD);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Sniffer] Error: Failed to set PAN ID\n");
    return -1;
  }

  ret = libtock_thread_manager_set_channel(26);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Sniffer] Error: Failed to set channel\n");
    return -1;
  }

  printf("[Sniffer] Radio configured: PAN 0xABCD, Channel 26\n");

  // Start receiving
  ret = libtock_thread_manager_receive(rx_buffer, RX_BUFFER_SIZE, rx_callback, NULL);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Sniffer] Error: Failed to start RX\n");
    return -1;
  }

  printf("\n[Sniffer] Monitoring network traffic...\n");
  printf("[Sniffer] Checking if we can sniff other apps' packets\n");

  // Just yield forever and see what we receive
  while (1) {
    yield();
  }

  return 0;
}