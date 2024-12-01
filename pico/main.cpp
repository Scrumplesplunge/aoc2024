#include "../common/coro.hpp"
#include "schedule.hpp"
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

Task<void> Day01(tcp::Socket& socket) {
  int a[1000];
  int b[1000];

  std::println("parsing input...");
  for (int i = 0; i < 1000; i++) {
    // Here we are requiring exactly 14 bytes per line:
    //   * 5 bytes for the first number
    //   * 3 spaces
    //   * 5 bytes for the second number
    //   * A newline
    char buffer[14];
    std::span<const char> line = co_await socket.Read(buffer);
    if (line.size() != 14) {
      co_await socket.Write("bad input");
      co_return;
    }
    assert(line[5] == ' ' && line[6] == ' ' && line[7] == ' ');
    assert(line[13] == '\n');

    a[i] = std::stoi(std::string(line.data(), 5));
    b[i] = std::stoi(std::string(line.data() + 8, 5));
  }

  std::println("sorting...");
  std::ranges::sort(a);
  std::ranges::sort(b);

  int delta = 0;
  for (int i = 0; i < 1000; i++) {
    delta += std::abs(a[i] - b[i]);
  }

  std::println("part 1: {}", delta);

  int score = 0;
  for (int i = 0; i < 1000; i++) {
    auto [first, last] = std::ranges::equal_range(b, a[i]);
    score += a[i] * (last - first);
  }

  std::println("part 2: {}", score);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", delta, score);
  co_await socket.Write(std::span<const char>(result, end - result));
}

Task<void> Serve() {
  std::println("Serve");
  tcp::Acceptor acceptor(0xA0C);
  std::println("Opened acceptor");

  while (true) {
    std::println("Waiting for connection...");
    tcp::Socket socket = co_await acceptor.Accept();
    co_await Day01(socket);
  }
}

void Run() {
  if (!Init()) std::exit(1);
  SetLed(true);
  ConnectToWifi();
  SetLed(false);

  Task<void> server = Serve();
  server.Start([] { std::println("Stopped serving."); });

  bool state = false;
  while (true) {
    SetLed(state);
    state = !state;
    sleep_ms(500);
  }
}

}  // namespace
}  // namespace aoc2024

int main() { aoc2024::Run(); }
