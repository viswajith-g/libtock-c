#pragma once

#include "../tock.h"

#ifdef __cplusplus
extern "C" {
#endif

// Callback for received packets
// app_id: The app ID of the sender
// len: Length of received packet
typedef void (*libtock_thread_manager_rx_callback)(uint16_t app_id, uint32_t len, void* userdata);

// Callback for TX completion
// result: 0 for success, error code otherwise
typedef void (*libtock_thread_manager_tx_callback)(uint32_t result, void* userdata);

// Check if the Thread network driver exists
bool libtock_thread_manager_exists(void);

// Get this process's app ID
returncode_t libtock_thread_manager_get_app_id(uint16_t* app_id);

// Send a packet (async)
// buffer: Data to send
// len: Length of data
// callback: Called when TX completes
// userdata: Passed to callback
returncode_t libtock_thread_manager_send(const uint8_t* buffer, 
                                     uint32_t len,
                                     libtock_thread_manager_tx_callback callback,
                                     void* userdata);

// Start receiving packets
// rx_buffer: Buffer for received packets (must remain valid)
// rx_buffer_len: Size of RX buffer
// callback: Called when packet received
// userdata: Passed to callback
returncode_t libtock_thread_manager_receive(uint8_t* rx_buffer,
                                        uint32_t rx_buffer_len,
                                        libtock_thread_manager_rx_callback callback,
                                        void* userdata);

// Configure 802.15.4 radio
returncode_t libtock_thread_manager_set_pan_id(uint16_t pan_id);
returncode_t libtock_thread_manager_get_pan_id(uint16_t* pan_id);
returncode_t libtock_thread_manager_set_channel(uint8_t channel);
returncode_t libtock_thread_manager_get_channel(uint8_t* channel);
returncode_t libtock_thread_manager_set_tx_power(int8_t power_dbm);
returncode_t libtock_thread_manager_get_tx_power(int8_t* power_dbm);

#ifdef __cplusplus
}
#endif