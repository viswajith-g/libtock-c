#include <stdio.h>
#include <string.h>

#include <libtock/net/thread_manager.h>
#include <libtock-sync/services/alarm.h>

#define RX_BUFFER_SIZE 128
#define TX_BUFFER_SIZE 64

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t tx_buffer[TX_BUFFER_SIZE];
static bool tx_done = false;
static uint32_t rx_count = 0;
static uint16_t my_app_id = 0;

// TX completion callback
static void tx_callback(uint32_t result, void* userdata) {
  (void)userdata;
  if (result == 0) {
    printf("[Receiver] Response sent successfully\n");
  } else {
    printf("[Receiver] Response send failed: %lu\n", result);
  }
  tx_done = true;
}

// RX callback - responds to every packet received
static void rx_callback(uint16_t sender_app_id, uint32_t len, void* userdata) {
  (void)userdata;
  returncode_t ret;
  
  rx_count++;
  
  // Null-terminate for safe printing
  if (len < RX_BUFFER_SIZE) {
    rx_buffer[len] = '\0';
  }
  
  printf("\n[Receiver] Rx #%lu from app %u (%lu bytes)\n", 
         rx_count, sender_app_id, len);
  printf("[Receiver] Data: %s\n", rx_buffer);

  // Check if this was meant for us (not our own packets)
  if (sender_app_id == my_app_id) {
    printf("[Receiver] (Ignoring own packet)\n");
    return;
  }

  // Send response
  int written = snprintf((char*)tx_buffer, TX_BUFFER_SIZE,
                        "ACK from TID:%u for seq:%lu",
                        my_app_id, rx_count);
  
  if (written < 0 || written >= TX_BUFFER_SIZE) {
    printf("[Receiver] Error: Buffer overflow\n");
    return;
  }

  uint32_t response_len = (uint32_t)written;
  
  printf("[Receiver] >>> Sending response (%lu bytes)\n", response_len);
  tx_done = false;
  
  ret = libtock_thread_manager_send(tx_buffer, response_len, tx_callback, NULL);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Receiver] Error: Failed to send response\n");
  } else {
    yield_for(&tx_done);
  }
}

int main(void) {
  returncode_t ret;
  uint16_t pan_id;
  uint8_t channel;

  printf("=================================\n");
  printf("Thread Network Receiver App\n");
  printf("=================================\n");

  // Check if driver exists
  if (!libtock_thread_manager_exists()) {
    printf("[Receiver] Error: Thread network driver not found!\n");
    return -1;
  }

  // Get my app ID
  ret = libtock_thread_manager_get_app_id(&my_app_id);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Receiver] Error: Failed to get app ID\n");
    return -1;
  }
  printf("[Receiver] My Tenant ID: %u\n", my_app_id);

  // Configure radio
  printf("[Receiver] Configuring radio...\n");
  ret = libtock_thread_manager_set_pan_id(0xABCD);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Receiver] Error: Failed to set PAN ID\n");
    return -1;
  }

  ret = libtock_thread_manager_set_channel(26);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Receiver] Error: Failed to set channel\n");
    return -1;
  }

  // Verify configuration
  ret = libtock_thread_manager_get_pan_id(&pan_id);
  if (ret == RETURNCODE_SUCCESS) {
    printf("[Receiver] PAN ID: 0x%04x\n", pan_id);
  }

  ret = libtock_thread_manager_get_channel(&channel);
  if (ret == RETURNCODE_SUCCESS) {
    printf("[Receiver] Channel: %u\n", channel);
  }

  // Start receiving
  ret = libtock_thread_manager_receive(rx_buffer, RX_BUFFER_SIZE, rx_callback, NULL);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Receiver] Error: Failed to start RX\n");
    return -1;
  }

  printf("[Receiver] Listening for packets...\n");

  while (1) {
    yield();
  }

  return 0;
}