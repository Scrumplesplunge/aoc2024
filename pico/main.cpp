#include "wifi.hpp"

#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <print>

namespace aoc2024 {
namespace {

void SetLed(bool on) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on); }

bool Init() {
  if (!stdio_init_all()) return false;
  if (cyw43_arch_init_with_country(WIFI_COUNTRY) != 0) return false;
  return true;
}

void ConnectToWifi() {
  cyw43_arch_enable_sta_mode();
  std::println("Connecting to WiFi...");
  while (!cyw43_arch_wifi_connect_timeout_ms(
             WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000) != 0) {
    std::println("Failed to connect to WiFi. Trying again.");
  }
  std::println("Connected.");
}

void Run() {
  if (!Init()) std::exit(1);
  SetLed(true);
  ConnectToWifi();
  SetLed(false);

  bool state = false;
  while (true) {
    state = !state;
    SetLed(state);
    sleep_ms(500);
  }
}

}  // namespace
}  // namespace aoc2024

int main() { aoc2024::Run(); }
