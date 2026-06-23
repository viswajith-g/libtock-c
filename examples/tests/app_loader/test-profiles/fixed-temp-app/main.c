// // #include <stdio.h>
// #include <string.h>
// // #include <stdlib.h>
// #include <stdbool.h>

// #include <libtock-sync/sensors/temperature.h>
// #include <libtock-sync/services/alarm.h>
// #include <libtock/kernel/ipc.h>
// #include <libtock/tock.h>

// #define SAMPLE_INTERVAL_MS 30000  // 30 seconds
// #define SHARED_BUF_SIZE 32

// static uint8_t temperature_f = 0;

// size_t _ot_service = -1;

// // ACK tracking
// bool _ack_received = false;

// uint8_t _ot_buf[SHARED_BUF_SIZE] __attribute__((aligned(SHARED_BUF_SIZE)));

// static int celsius_to_fahrenheit(int temp_celsius) {
//   return (temp_celsius * 9 / 5) + 32;
// }

// // Callback for receiving ACKs from OpenThread service
// static void ipc_callback(__attribute__((unused)) int pid,
//                              __attribute__((unused)) int len,
//                              __attribute__((unused)) int buf,
//                              __attribute__((unused)) void* ud) {
//   _ack_received = true;
// }

// static int clamp_temperature(int temp, int min, int max) {
//   if (temp < min) return min;
//   if (temp > max) return max;
//   return temp;
// }

// int main(void) {
//   // printf("Fixed Temp Sensor App\n");

//   if (!libtock_temperature_exists()) {
//     // printf("Error: No temperature sensor found!\n");
//     return -1;
//   }

//   int ret;
// do {
//     ret = ipc_discover("openthread_app", &_ot_service);
//     if (ret != RETURNCODE_SUCCESS) {
//         libtocksync_alarm_delay_ms(100);
//     }
// } while (ret != RETURNCODE_SUCCESS);
//   // int ret = ipc_discover("openthread-ipc", &_ot_service);
//   // if (ret != RETURNCODE_SUCCESS) {
//   //   printf("No Open Thread Service\n");
//   //   return -1;
//   // }

//   // ipc_register_service_callback("fixed-temp",
//   //                               ipc_callback,
//   //                               NULL);

//   if (_ot_service != (size_t) -1) {
//     ipc_register_client_callback(_ot_service, ipc_callback, NULL);
//     ipc_share(_ot_service, _ot_buf, SHARED_BUF_SIZE);
//   }

//   // give the openthread app time to init
//   libtocksync_alarm_delay_ms(5000);

//   while (1) {
//     // Read temperature sensor (returns in hundredths of degrees C)
//     int temp_c_raw;
//     int ret1 = libtocksync_temperature_read(&temp_c_raw);
    
//     if (ret1 != RETURNCODE_SUCCESS) {
//       // printf("Error reading temperature: %d\n", ret1);
//       libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);
//       continue;
//     }

//     // Convert to integer Celsius
//     int temp_c = temp_c_raw / 100;

//     // printf("Temperature: %d°C\n", temp_c);

//     /* FIX: validate reading is within nRF52840 sensor range */
//     int temp_c_clamped = clamp_temperature(temp_c, -40, 80);
                

//     int temp_f_int = celsius_to_fahrenheit(temp_c_clamped);
//     temperature_f = (uint8_t)temp_f_int;

//     // printf("Temperature: %d°C -> %d°F\n", temp_c, temperature_f);

//     if (_ot_service == (size_t) -1) return -1;

//     // printf("about to tx packet\n");

//     _ot_buf[0] = temperature_f;
//     _ack_received = false;          // reset before notify
//     ipc_notify_service(_ot_service); // tell OT app data is ready
//     yield_for(&_ack_received);      // wait for ACK
    
//     libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);
//   }

//   return 0;
// }

#include <stdio.h>
#include <string.h>
// #include <stdlib.h>
#include <stdbool.h>

#include <libtock-sync/sensors/temperature.h>
// #include <libtock-sync/sensors/ambient_light.h>
#include <libtock-sync/services/alarm.h>
#include <libtock/kernel/ipc.h>
#include <libtock/tock.h>

#define SAMPLE_INTERVAL_MS 30000  // 30 seconds
// #define AMBIENT_INTERVAL_MS 17000 // 17 seconds
#define SHARED_BUF_SIZE 32

static uint8_t temperature_f = 0;

size_t _ot_service = -1;

// ACK tracking
bool _ack_received = false;
bool temp_ready = false;
// bool lux_ready = false;

uint8_t _ot_buf[SHARED_BUF_SIZE] __attribute__((aligned(SHARED_BUF_SIZE)));

static int celsius_to_fahrenheit(int temp_celsius) {
  return (temp_celsius * 9 / 5) + 32;
}

static void temp_cb(__attribute__((unused)) uint32_t now, __attribute__((unused)) uint32_t expiration, __attribute__((unused)) void* ud) {
  temp_ready = true;
}

// static void lux_cb(__attribute__((unused)) uint32_t now, __attribute__((unused)) uint32_t expiration, __attribute__((unused)) void* ud) {
//   lux_ready = true;
// }

// Callback for receiving ACKs from OpenThread service
static void ipc_callback(__attribute__((unused)) int pid,
                             __attribute__((unused)) int len,
                             __attribute__((unused)) int buf,
                             __attribute__((unused)) void* ud) {
  _ack_received = true;
}

static int clamp_temperature(int temp, int min, int max) {
  if (temp < min) return min;
  if (temp > max) return max;
  return temp;
}

int main(void) {
  // printf("Fixed Temp Sensor App\n");

  if (!libtock_temperature_exists()) {
    // printf("Error: No temperature sensor found!\n");
    return -1;
  }

  // libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);

//   int ret;
// do {
//     ret = ipc_discover("openthread_app", &_ot_service);
//     if (ret != RETURNCODE_SUCCESS) {
//         libtocksync_alarm_delay_ms(100);
//     }
// } while (ret != RETURNCODE_SUCCESS);

  if (_ot_service != (size_t) -1) {
    ipc_register_client_callback(_ot_service, ipc_callback, NULL);
    ipc_share(_ot_service, _ot_buf, SHARED_BUF_SIZE);
  }

  // give the openthread app time to init
  libtocksync_alarm_delay_ms(5000);

  libtock_alarm_t t1;
  // libtock_alarm_t t2;

  libtock_alarm_repeating_every_ms(SAMPLE_INTERVAL_MS, temp_cb, (void*)1, &t1);
  // libtock_alarm_repeating_every_ms(AMBIENT_INTERVAL_MS, lux_cb, (void*)2, &t2);

  while (1) {
    // Read temperature sensor (returns in hundredths of degrees C)

    if (temp_ready){
      temp_ready = false;
      int temp_c_raw;
      int _ret1 = libtocksync_temperature_read(&temp_c_raw);

      // Convert to integer Celsius
      int temp_c = temp_c_raw / 100;

      // printf("Temperature: %d°C\n", temp_c);

      int temp_c_clamped = clamp_temperature(temp_c, -40, 80);
                  

      int temp_f_int = celsius_to_fahrenheit(temp_c_clamped);
      temperature_f = (uint8_t)temp_f_int;

      // printf("Temperature: %d°C -> %d°F\n", temp_c, temperature_f);

      if (_ot_service == (size_t) -1) return -1;

      // printf("about to tx packet\n");
      _ot_buf[0] = 0x00;
      _ot_buf[1] = temperature_f;
      _ack_received = false;          // reset before notify
      ipc_notify_service(_ot_service); // tell OT app data is ready
      yield_for(&_ack_received);      // wait for ACK
    }
  }
  return 0;
}