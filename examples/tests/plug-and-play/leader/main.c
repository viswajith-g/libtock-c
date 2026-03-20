#include <stdio.h>
#include <libtock/tock.h>
#include <libtock/peripherals/gpio.h>
#include <libtock/kernel/app_loader.h>
#include <libtock-sync/services/alarm.h>

// Size of the app sitting on the QSPI chip.
// The follower should send this as part of the handshake,
// but for now hardcode it.
#define SIGNAL_GPIO 0
#define QSPI_ADDR 0x12000000
#define APP_SIZE  0x4000

/******************************************************************************************************
* Callback Tracking Flags
******************************************************************************************************/
static bool load_done     = false;

/******************************************************************************************************
* Callbacks
******************************************************************************************************/
static void app_load_done_callback(int arg0,
                                   __attribute__((unused)) int arg1,
                                   __attribute__((unused)) int arg2,
                                   __attribute__((unused)) void* ud) {
  if (arg0 != RETURNCODE_SUCCESS) {
    printf("[Error] Process creation failed: %d\n", arg0);
  } else {
    printf("[Success] Process created successfully.\n");
  }
  load_done = true;
}

/******************************************************************************************************
* Load from QSPI — setup and finalize with no write phase
******************************************************************************************************/
static void load_from_qspi(void) {
  printf("[Leader] Triggering XIP load from 0x12000000...\n");

  int ret = libtock_app_loader_load_xip(QSPI_ADDR, APP_SIZE, app_load_done_callback);
  if (ret != RETURNCODE_SUCCESS) {
    printf("[Error] load_xip failed: %d\n", ret);
    return;
  }

  yield_for(&load_done);
  load_done = false;
  printf("[Leader] New App Loaded.\n");
  
}

/******************************************************************************************************
* Main
******************************************************************************************************/
int main(void) {
  printf("[Leader] Started. Checking app loader driver.\n");

  if (!libtock_app_loader_exists()) {
    printf("[Error] No app loader driver found.\n");
    return -1;
  }

  printf("[Leader] Listening for follower handshake..\n");
  libtock_gpio_enable_input(SIGNAL_GPIO, libtock_pull_down);

  printf("Waiting for signal\n");
  // Poll until follower drives pin HIGH
  int val = 0;
  while (val == 0) {
      libtock_gpio_read(SIGNAL_GPIO, &val);
      libtocksync_alarm_delay_ms(10);
  }
  printf("[Leader] Signal received. Loading app from QSPI.\n");
  load_from_qspi();

  printf("[Leader] Done. Halting coordinator.\n");
  while (1) yield();
}