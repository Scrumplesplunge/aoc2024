#include "../common/coro.hpp"
#include "schedule.hpp"
#include "solve.hpp"
#include "tcp.hpp"
#include "wifi.hpp"

#include <algorithm>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <print>

namespace aoc2024 {
namespace {

void SetLed(bool on) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on); }

bool Init() {
  if (!stdio_init_all()) return false;
  if (cyw43_arch_init_with_country(WIFI_COUNTRY) != 0) return false;
  if (!SchedulerInit()) return false;
  return true;
}

void ConnectToWifi() {
  cyw43_arch_enable_sta_mode();
  std::println("Connecting to WiFi...");
  while (cyw43_arch_wifi_connect_timeout_ms(
             WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000) != 0) {
    std::println("Failed to connect to WiFi. Trying again.");
  }
  std::println("Connected.");
}

Task<void> Serve() {
  std::println("Serve");
  tcp::Acceptor acceptor(0xA0C);
  std::println("Opened acceptor");

  while (true) {
    std::println("Waiting for connection...");
    tcp::Socket socket = co_await acceptor.Accept();
    SetLed(true);
    std::println("Solving...");
    co_await Solve(/*day=*/1, socket);
    SetLed(false);
  }
}

void Run() {
  if (!Init()) std::exit(1);
  SetLed(true);
  ConnectToWifi();
  SetLed(false);

  Task<void> server = Serve();
  server.Start([] {
    std::println("Stopped serving.");
    std::exit(1);
  });
  while (true) sleep_ms(60000);
}

}  // namespace
}  // namespace aoc2024

int main() { aoc2024::Run(); }
