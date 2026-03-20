#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <libtock-sync/sensors/temperature.h>
#include <libtock-sync/services/alarm.h>
#include <libtock/kernel/ipc.h>
#include <libtock/tock.h>

#define SAMPLE_INTERVAL_MS 30000  // 30 seconds
#define SHARED_BUF_SIZE 32

static uint8_t temperature_f = 0;

size_t _ot_service = -1;

// ACK tracking
bool _ack_received = false;

uint8_t _ot_buf[SHARED_BUF_SIZE] __attribute__((aligned(SHARED_BUF_SIZE)));

static int celsius_to_fahrenheit(int temp_celsius) {
  return (temp_celsius * 9 / 5) + 32;
}

// Callback for receiving ACKs from OpenThread service
static void ipc_callback(__attribute__((unused)) int pid,
                             __attribute__((unused)) int len,
                             __attribute__((unused)) int buf,
                             __attribute__((unused)) void* ud) {
  _ack_received = true;
}

int main(void) {
  // printf("Fixed Temp Sensor App\n");

  if (!libtock_temperature_exists()) {
    printf("Error: No temperature sensor found!\n");
    return -1;
  }

  ipc_register_service_callback("fixed-temp",
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
    int temp_c_raw;
    int ret = libtocksync_temperature_read(&temp_c_raw);
    
    if (ret != RETURNCODE_SUCCESS) {
      printf("Error reading temperature: %d\n", ret);
      libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);
      continue;
    }

    int fixed_loop_val = 1000;
    int count = 0;
    for (int i = 0; i < fixed_loop_val; i++){
      count += 1;
    }

    // Convert to integer Celsius
    int temp_c = temp_c_raw / 100;

    // Convert to integer Fahrenheit (BUGGY conversion)
    int temp_f_int = celsius_to_fahrenheit(temp_c);
    temperature_f = (uint8_t)temp_f_int;

    printf("Temperature: %d°C -> %d°F", temp_c, temperature_f);

    if (_ot_service == (size_t) -1) return;

    _ot_buf[0] = temperature_f;
    yield_for(&_ack_received);
    
    libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);
  }

  return 0;
}