// #include <stdio.h>
// #include <string.h>
// #include <stdbool.h>

// #include <libtock-sync/sensors/temperature.h>
// #include <libtock-sync/services/alarm.h>
// #include <libtock/kernel/ipc.h>
// #include <libtock/interface/led.h>
// #include <libtock/tock.h>

// #define SAMPLE_INTERVAL_MS 30000  // 30 seconds
// #define ACK_TIMEOUT_MS 1000       // 1 second timeout for ACK
// #define TEMP_THRESHOLD_C 50       // 25°C threshold for LED alert

// // Shared temperature data (1 byte)
// static uint8_t temperature_f = 0;

// // OpenThread service tracking
// static int ot_pid = -1;
// static bool ot_available = false;

// // ACK tracking
// static volatile bool ack_received = false;

// // LED tracking
// static bool led_available = false;

// // Statistics
// static uint32_t alert_count = 0;

// // Correct conversion formula
// static int celsius_to_fahrenheit(int temp_celsius) {
//   return (temp_celsius * 9 / 5) + 32;
// }

// // NEW: Check temperature threshold and control LED
// static bool check_threshold_and_led(int temp_c) {
//   bool over_threshold = (temp_c >= TEMP_THRESHOLD_C);
  
//   if (led_available) {
//     if (over_threshold) {
//       libtock_led_on(0);
//     } else {
//       libtock_led_off(0);
//     }
//   }
  
//   return over_threshold;
// }

// // Callback for receiving ACKs from OpenThread service
// static void ipc_ack_callback(__attribute__((unused)) int pid,
//                              __attribute__((unused)) int len,
//                              __attribute__((unused)) int buf,
//                              __attribute__((unused)) void* ud) {
//   ack_received = true;
// }

// int main(void) {
//   printf("Temperature Sensor Service with LED Alert\n");
//   printf("==========================================\n\n");

//   // NEW: Initialize LED
//   int num_leds;
//   int led_err = libtock_led_count(&num_leds);
  
//   if (led_err >= 0 && num_leds > 0) {
//     led_available = true;
//     libtock_led_off(0);
//     printf("LED alert enabled (threshold: %d°C / %d°F)\n",
//            TEMP_THRESHOLD_C, celsius_to_fahrenheit(TEMP_THRESHOLD_C));
//   } else {
//     printf("Warning: No LED available for alerts\n");
//   }
//   printf("\n");

//   // Register a callback to receive ACKs from OpenThread
//   ipc_register_service_callback("led-temp",
//                                 ipc_ack_callback,
//                                 NULL);

//   uint32_t sample_count = 0;

//   while (1) {
//     // Read temperature sensor (returns in hundredths of degrees C)
//     int temp_c_raw;
//     int ret = libtocksync_temperature_read(&temp_c_raw);
    
//     if (ret != RETURNCODE_SUCCESS) {
//       printf("Error reading temperature: %d\n", ret);
//       libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);
//       continue;
//     }

//     sample_count++;

//     // Convert to integer Celsius
//     int temp_c = temp_c_raw / 100;

//     // Convert to integer Fahrenheit
//     int temp_f_int = celsius_to_fahrenheit(temp_c);
//     temperature_f = (uint8_t)temp_f_int;

//     // NEW: Check threshold and control LED
//     bool alert_active = check_threshold_and_led(temp_c);
//     if (alert_active) {
//       alert_count++;
//     }

//     printf("[%lu] Temperature: %d°C -> %d°F", sample_count, temp_c, temperature_f);
    
//     // NEW: Show alert status
//     if (alert_active) {
//       printf("ALERT");
//     }

//     // Try to discover OpenThread service if not yet available
//     if (!ot_available) {
//       ot_pid = ipc_discover("openthread-ipc");
      
//       if (ot_pid >= 0) {
//         printf(" | Found OpenThread service (PID: %d)\n", ot_pid);
//         ot_available = true;
        
//         // Share our temperature buffer with the OpenThread service
//         ipc_share(ot_pid, &temperature_f, sizeof(temperature_f));
//         printf("    Buffer shared with OpenThread\n");
//       } else {
//         printf(" | OpenThread not found, will retry\n");
//       }
//     }

//     // Send temperature to OpenThread if available
//     if (ot_available) {
      
//       ack_received = false;

//       // Notify the OpenThread service
//       int result = ipc_notify_svc(ot_pid);
      
//       if (result < 0) {
//         printf("    ERROR: Failed to notify OpenThread (code: %d)\n", result);
//         ot_available = false;  // Retry discovery next time
//       } else {
//         printf("    Notification sent, waiting for ACK...\n");
        
//         yield_for(&_ack_received);
//         ot_available = false;
//       }
//     } else {
//       printf("\n");
//     }

//     // NEW: Print statistics every 10 samples
//     if (sample_count % 10 == 0) {
//       float alert_pct = (alert_count * 100.0) / sample_count;
//       printf("\n[Stats] Samples: %lu | Alerts: %lu (%.1f%%)\n\n",
//              sample_count, alert_count, alert_pct);
//     }

//     // Wait 30 seconds before next sample
//     libtocksync_alarm_delay_ms(SAMPLE_INTERVAL_MS);
//   }

//   return 0;
// }

// #include <stdio.h>
#include <string.h>
// #include <stdlib.h>
#include <stdbool.h>

#include <libtock-sync/sensors/temperature.h>
// #include <libtock-sync/sensors/ambient_light.h>
#include <libtock-sync/services/alarm.h>
#include <libtock/interface/led.h>
#include <libtock/kernel/ipc.h>
#include <libtock/tock.h>

#define SAMPLE_INTERVAL_MS 30000  // 30 seconds
// #define AMBIENT_INTERVAL_MS 17000 // 17 seconds
#define SHARED_BUF_SIZE 32
#define TEMP_THRESHOLD_C 50

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

static bool check_threshold_and_led(int temp_c) {
  if (temp_c >= TEMP_THRESHOLD_C) {
    libtock_led_on(0);
  } else {
    libtock_led_off(0);
  }
}

int main(void) {
  // printf("Fixed Temp Sensor App\n");

  if (!libtock_temperature_exists()) {
    // printf("Error: No temperature sensor found!\n");
    return -1;
  }

  int num_leds;
  int led_err = libtock_led_count(&num_leds);
  
  if (led_err >= 0 && num_leds > 0) {
    libtock_led_off(0);
  } else {
    return -1;
  }

  int ret;
do {
    ret = ipc_discover("openthread_app", &_ot_service);
    if (ret != RETURNCODE_SUCCESS) {
        libtocksync_alarm_delay_ms(100);
    }
} while (ret != RETURNCODE_SUCCESS);

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

      check_threshold_and_led(temp_c_clamped);
                  

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