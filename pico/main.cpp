#include "wifi.hpp"

#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>

int main() {
  stdio_init_all();

  if (cyw43_arch_init_with_country(WIFI_COUNTRY) != 0) return 1;

  bool state = false;
  while (true) {
    state = !state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
    sleep_ms(500);
  }
}
