#include <stdio.h>
#include <libtock/peripherals/gpio.h>
#include <libtock/interface/button.h>
#include <libtock-sync/services/alarm.h>

#define SIGNAL_GPIO 0

const int QSPI_PINS[] = {17, 19, 20, 21, 22, 23};

static bool button_pressed = false;

static void button_callback(__attribute__((unused)) returncode_t retval,
                             int btn_num,
                             __attribute__((unused)) bool pressed) {
  if (btn_num == 0) {
    button_pressed = true;
  }
}

int main(void) {
  printf("[Follower] Started. Waiting for button press.\n");

  int count;
  int err = libtock_button_count(&count);
  if (err < 0) return err;

  for (int i = 0; i < count; i++) {
    libtock_button_notify_on_press(i, button_callback);
  }

  libtock_gpio_enable_output(SIGNAL_GPIO);
  libtock_gpio_clear(SIGNAL_GPIO);

  // Wait for button press
  yield_for(&button_pressed);

  printf("[Follower] Button pressed, sending secret...\n");

  for (int i = 0; i < 6; i++) {
    // We use GPIO to force the pins into a non-driving state
    libtock_gpio_enable_input(QSPI_PINS[i], libtock_pull_none);
  }

  libtock_gpio_set(SIGNAL_GPIO);

  // Hand off complete — halt and let leader own the QSPI bus
  printf("[Follower] Halting.\n");
  libtocksync_alarm_delay_ms(20);
  libtock_gpio_clear(SIGNAL_GPIO);
  while (1) yield();
}