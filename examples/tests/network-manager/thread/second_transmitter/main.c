#include <stdio.h>
#include <string.h>

#include <libtock/net/thread_manager.h>
#include <libtock-sync/services/alarm.h>

#define RX_BUFFER_SIZE 128
#define Tx_BUFFER_SIZE 64

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t tx_buffer[Tx_BUFFER_SIZE];
static bool tx_done = false;

// Tx completion callback
static void tx_callback(uint32_t result, void* userdata) {
  (void)userdata;
  if (result == 0) {
    printf("[Transmitter_2]  Tx Success\n");
  } else {
    printf("[Transmitter_2]  Tx Failed: %lu\n", result);
  }
  tx_done = true;
}

// Rx callback
static void rx_callback(uint16_t app_id, uint32_t len, void* userdata) {
  (void)userdata;
  
  // Null-terminate for safe printing
  if (len < RX_BUFFER_SIZE) {
    rx_buffer[len] = '\0';
  }
  
  printf("[Transmitter_2]  Rx from app %u (%lu bytes): %s\n", 
         app_id, len, rx_buffer);
}

int main(void) {
  returncode_t ret;
  uint16_t my_app_id;
  uint32_t counter = 0;

  printf("Thread Network Beacon App 2\n");

  // Check if driver exists
  if (!libtock_thread_manager_exists()) {
    printf("[Transmitter_2]  Error: Thread network driver not found!\n");
    return -1;
  }

  // Get app ID
  ret = libtock_thread_manager_get_app_id(&my_app_id);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Transmitter_2] Error: Failed to get app ID\n");
    return -1;
  }
  printf("[Transmitter_2] Tenant ID: %u\n", my_app_id);

  // Configure radio
  printf("[Transmitter_2] Configuring radio...\n");
  ret = libtock_thread_manager_set_pan_id(0xABCD);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Transmitter_2] Error: Failed to set PAN ID\n");
    return -1;
  }

  ret = libtock_thread_manager_set_channel(26);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Transmitter_2] Error: Failed to set channel\n");
    return -1;
  }

  printf("[Transmitter_2] Radio configured: PAN 0xABCD, Channel 26\n");

  // Start receiving
  ret = libtock_thread_manager_receive(rx_buffer, RX_BUFFER_SIZE, rx_callback, NULL);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Transmitter_2] Error: Failed to start Rx\n");
    return -1;
  }
  printf("[Transmitter_2] Rx started\n");

  // Main loop: send beacon every 3 seconds
  printf("[Transmitter_2] Starting beacon transmission...\n");
  while (1) {
    // Prepare beacon packet
    int written = snprintf((char*)tx_buffer, Tx_BUFFER_SIZE, 
                          "Beacon from TID:%u seq:%lu", 
                          my_app_id, counter);
    
    if (written < 0 || written >= Tx_BUFFER_SIZE) {
      printf("[Transmitter_2] Error: Buffer overflow\n");
      continue; 
    }

    uint32_t len = (uint32_t)written;

    printf("\n[Transmitter_2] Preparing to send beacon #%lu (%lu bytes)\n", counter, len);
    printf("[Transmitter_2] Data: %s\n", tx_buffer);

    printf("\n[Transmitter_2] Sending beacon #%lu (%lu bytes)\n", counter, len);
    tx_done = false;
    
    ret = libtock_thread_manager_send(tx_buffer, len, tx_callback, NULL);
    if (ret != RETURNCODE_SUCCESS) {
      printf("[Transmitter_2] Error: Failed to initiate send\n");
    } else {
      // Wait for Tx completion
      yield_for(&tx_done);
    }

    counter++;
    
    // 3 second delay
    libtocksync_alarm_delay_ms(10000);
  }

  return 0;
}