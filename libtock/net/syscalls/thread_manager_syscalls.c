#include "thread_manager_syscalls.h"

bool libtock_thread_manager_driver_exists(void) {
  return driver_exists(DRIVER_NUM_THREAD_MANAGER);
}

returncode_t libtock_thread_manager_command_get_app_id(uint32_t* app_id) {
  syscall_return_t res = command(DRIVER_NUM_THREAD_MANAGER, 1, 0, 0);
  return tock_command_return_u32_to_returncode(res, app_id);
}

returncode_t libtock_thread_manager_command_send(uint32_t len) {
  syscall_return_t res = command(DRIVER_NUM_THREAD_MANAGER, 2, len, 0);
  return tock_command_return_novalue_to_returncode(res);
}

returncode_t libtock_thread_manager_command_set_pan_id(uint16_t pan_id) {
  syscall_return_t res = command(DRIVER_NUM_THREAD_MANAGER, 3, pan_id, 0);
  return tock_command_return_novalue_to_returncode(res);
}

returncode_t libtock_thread_manager_command_set_channel(uint8_t channel) {
  syscall_return_t res = command(DRIVER_NUM_THREAD_MANAGER, 4, channel, 0);
  return tock_command_return_novalue_to_returncode(res);
}

returncode_t libtock_thread_manager_command_get_channel(uint8_t* channel) {
  syscall_return_t res = command(DRIVER_NUM_THREAD_MANAGER, 5, 0, 0);
  returncode_t ret = tock_command_return_u32_to_returncode(res, (uint32_t*)channel);
  if (ret == RETURNCODE_SUCCESS) {
    *channel = (uint8_t)(res.data[0] & 0xFF);
  }
  return ret;
}

returncode_t libtock_thread_manager_command_get_pan_id(uint16_t* pan_id) {
  syscall_return_t res = command(DRIVER_NUM_THREAD_MANAGER, 6, 0, 0);
  returncode_t ret = tock_command_return_u32_to_returncode(res, (uint32_t*)pan_id);
  if (ret == RETURNCODE_SUCCESS) {
    *pan_id = (uint16_t)(res.data[0] & 0xFFFF);
  }
  return ret;
}

returncode_t libtock_thread_manager_command_set_tx_power(int8_t power_dbm) {
  syscall_return_t res = command(DRIVER_NUM_THREAD_MANAGER, 7, (uint32_t)power_dbm, 0);
  return tock_command_return_novalue_to_returncode(res);
}

returncode_t libtock_thread_manager_command_get_tx_power(int8_t* power_dbm) {
  syscall_return_t res = command(DRIVER_NUM_THREAD_MANAGER, 8, 0, 0);
  returncode_t ret = tock_command_return_u32_to_returncode(res, (uint32_t*)power_dbm);
  if (ret == RETURNCODE_SUCCESS) {
    *power_dbm = (int8_t)(res.data[0] & 0xFF);
  }
  return ret;
}

returncode_t libtock_thread_manager_set_allow_readonly_tx_buffer(const uint8_t* buffer, size_t len) {
  allow_ro_return_t res = allow_readonly(DRIVER_NUM_THREAD_MANAGER, 0, (void*)buffer, len);
  return tock_allow_ro_return_to_returncode(res);
}

returncode_t libtock_thread_manager_set_allow_readwrite_rx_buffer(uint8_t* buffer, size_t len) {
  allow_rw_return_t res = allow_readwrite(DRIVER_NUM_THREAD_MANAGER, 0, (void*)buffer, len);
  return tock_allow_rw_return_to_returncode(res);
}

returncode_t libtock_thread_manager_subscribe_rx(subscribe_upcall callback, void* userdata) {
  subscribe_return_t res = subscribe(DRIVER_NUM_THREAD_MANAGER, 0, callback, userdata);
  return tock_subscribe_return_to_returncode(res);
}

returncode_t libtock_thread_manager_subscribe_tx_done(subscribe_upcall callback, void* userdata) {
  subscribe_return_t res = subscribe(DRIVER_NUM_THREAD_MANAGER, 1, callback, userdata);
  return tock_subscribe_return_to_returncode(res);
}