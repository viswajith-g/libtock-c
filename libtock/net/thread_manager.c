#include "thread_manager.h"
#include "syscalls/thread_manager_syscalls.h"

// Internal state for callbacks
typedef struct {
  libtock_thread_manager_rx_callback rx_callback;
  void* rx_userdata;
  libtock_thread_manager_tx_callback tx_callback;
  void* tx_userdata;
  uint8_t* rx_buffer;
} thread_manager_state_t;

static thread_manager_state_t state = {0};

// Internal upcall handler for RX
static void rx_upcall(int app_id, int len, int unused, void* userdata) {
  (void)unused;
  thread_manager_state_t* st = (thread_manager_state_t*)userdata;
  if (st->rx_callback) {
    st->rx_callback((uint16_t)app_id, (uint32_t)len, st->rx_userdata);
  }
}

// Internal upcall handler for TX done
static void tx_done_upcall(int result, int unused1, int unused2, void* userdata) {
  (void)unused1;
  (void)unused2;
  thread_manager_state_t* st = (thread_manager_state_t*)userdata;
  if (st->tx_callback) {
    st->tx_callback((uint32_t)result, st->tx_userdata);
    st->tx_callback = NULL;  // One-shot callback
  }
}

bool libtock_thread_manager_exists(void) {
  return libtock_thread_manager_driver_exists();
}

returncode_t libtock_thread_manager_get_app_id(uint16_t* app_id) {
  uint32_t tid;
  returncode_t ret = libtock_thread_manager_command_get_app_id(&tid);
  if (ret == RETURNCODE_SUCCESS) {
    *app_id = (uint16_t)tid;
  }
  return ret;
}

returncode_t libtock_thread_manager_send(const uint8_t* buffer,
                                     uint32_t len,
                                     libtock_thread_manager_tx_callback callback,
                                     void* userdata) {
  returncode_t ret;

  // Set TX callback
  state.tx_callback = callback;
  state.tx_userdata = userdata;
  
  ret = libtock_thread_manager_subscribe_tx_done(tx_done_upcall, &state);
  if (ret != RETURNCODE_SUCCESS) return ret;

  // Set TX buffer
  ret = libtock_thread_manager_set_allow_readonly_tx_buffer(buffer, len);
  if (ret != RETURNCODE_SUCCESS) return ret;

  // Initiate send
  return libtock_thread_manager_command_send(len);
}

returncode_t libtock_thread_manager_receive(uint8_t* rx_buffer,
                                        uint32_t rx_buffer_len,
                                        libtock_thread_manager_rx_callback callback,
                                        void* userdata) {
  returncode_t ret;

  // Save state
  state.rx_callback = callback;
  state.rx_userdata = userdata;
  state.rx_buffer = rx_buffer;

  // Subscribe to RX callback
  ret = libtock_thread_manager_subscribe_rx(rx_upcall, &state);
  if (ret != RETURNCODE_SUCCESS) return ret;

  // Set RX buffer
  return libtock_thread_manager_set_allow_readwrite_rx_buffer(rx_buffer, rx_buffer_len);
}

returncode_t libtock_thread_manager_set_pan_id(uint16_t pan_id) {
  return libtock_thread_manager_command_set_pan_id(pan_id);
}

returncode_t libtock_thread_manager_get_pan_id(uint16_t* pan_id) {
  return libtock_thread_manager_command_get_pan_id(pan_id);
}

returncode_t libtock_thread_manager_set_channel(uint8_t channel) {
  return libtock_thread_manager_command_set_channel(channel);
}

returncode_t libtock_thread_manager_get_channel(uint8_t* channel) {
  return libtock_thread_manager_command_get_channel(channel);
}

returncode_t libtock_thread_manager_set_tx_power(int8_t power_dbm) {
  return libtock_thread_manager_command_set_tx_power(power_dbm);
}

returncode_t libtock_thread_manager_get_tx_power(int8_t* power_dbm) {
  return libtock_thread_manager_command_get_tx_power(power_dbm);
}