
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <libtock-sync/sensors/ambient_light.h>
#include <libtock-sync/services/alarm.h>
#include <libtock/kernel/ipc.h>
#include <libtock/tock.h>

#define SAMPLE_INTERVAL_MS 15000  // 15 seconds
#define SHARED_BUF_SIZE 32

size_t _ot_service = -1;

// ACK tracking
bool _ack_received = false;

uint8_t _ot_buf[SHARED_BUF_SIZE] __attribute__((aligned(SHARED_BUF_SIZE)));

// Callback for receiving ACKs from OpenThread service
static void ipc_callback(__attribute__((unused)) int pid,
                             __attribute__((unused)) int len,
                             __attribute__((unused)) int buf,
                             __attribute__((unused)) void* ud) {
  _ack_received = true;
}

int main(void) {
  // printf("Fixed Temp Sensor App\n");

  // if (!libtock_temperature_exists()) {
  //   printf("Error: No temperature sensor found!\n");
  //   return -1;
  // }

  ipc_register_service_callback("amb-light",
                                ipc_callback,
                                NULL);

  int ret = ipc_discover("openthread-ipc", &_ot_service);
  if (ret != RETURNCODE_SUCCESS) {
    printf("No Open Thread Service\n");
  }

  if (_ot_service != (size_t) -1) {
    ipc_register_client_callback(_ot_service, ipc_callback, NULL);
    ipc_share(_ot_service, _ot_buf, SHARED_BUF_SIZE);
  }

  while (1) {
    // Read temperature sensor (returns in hundredths of degrees C)
    int lux;
    ret = libtocksync_ambient_light_read_intensity(&lux);
    if (ret != RETURNCODE_SUCCESS) {
      printf("Error reading light value: %d\n", ret);
      libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);
      continue;
    }

    if (_ot_service == (size_t) -1) return -1;

    _ot_buf[0] = lux;
    yield_for(&_ack_received);
    
    libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);
  }

  return 0;
}