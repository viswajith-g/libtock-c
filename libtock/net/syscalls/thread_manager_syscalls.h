#pragma once

#include "../../tock.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_NUM_THREAD_MANAGER 0x31001

// Check if the Thread network driver exists
bool libtock_thread_manager_driver_exists(void);

// Command: Get app ID
returncode_t libtock_thread_manager_command_get_app_id(uint32_t* app_id);

// Command: Send packet
returncode_t libtock_thread_manager_command_send(uint32_t len);

// Command: Set PAN ID
returncode_t libtock_thread_manager_command_set_pan_id(uint16_t pan_id);

// Command: Set channel
returncode_t libtock_thread_manager_command_set_channel(uint8_t channel);

// Command: Get channel
returncode_t libtock_thread_manager_command_get_channel(uint8_t* channel);

// Command: Get PAN ID
returncode_t libtock_thread_manager_command_get_pan_id(uint16_t* pan_id);

// Command: Set TX power
returncode_t libtock_thread_manager_command_set_tx_power(int8_t power_dbm);

// Command: Get TX power
returncode_t libtock_thread_manager_command_get_tx_power(int8_t* power_dbm);

// Allow: Set TX buffer
returncode_t libtock_thread_manager_set_allow_readonly_tx_buffer(const uint8_t* buffer, size_t len);

// Allow: Set RX buffer
returncode_t libtock_thread_manager_set_allow_readwrite_rx_buffer(uint8_t* buffer, size_t len);

// Subscribe: RX callback
returncode_t libtock_thread_manager_subscribe_rx(subscribe_upcall callback, void* userdata);

// Subscribe: TX done callback
returncode_t libtock_thread_manager_subscribe_tx_done(subscribe_upcall callback, void* userdata);

#ifdef __cplusplus
}
#endif