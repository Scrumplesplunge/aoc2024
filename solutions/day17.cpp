#include <algorithm>
#include <print>

#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

namespace aoc2024 {
namespace {

enum Operation : std::uint8_t {
  kAdv,  // a /= 1 << arg
  kBxl,  // b ^= lit
  kBst,  // b %= 8
  kJnz,  // if (A != 0) goto lit
  kBxc,  // b ^= c
  kOut,  // emit(arg)
  kBdv,  // b = a / (1 << arg)
  kCdv,  // c = a / (1 << arg)
};

struct Input {
  Task<void> Read(tcp::Socket& socket) {
    char input_buffer[128];
    std::string_view input(co_await socket.Read(input_buffer));
    if (!ScanPrefix(input,
                    "Register A: {}\n"
                    "Register B: {}\n"
                    "Register C: {}\n"
                    "\n"
                    "Program: ",
                    a, b, c)) {
      throw std::runtime_error("bad input (syntax)");
    }
    int num_operations = 0;
    const int max_operations = std::size(buffer);
    while (true) {
      if (input.size() < 2) throw std::runtime_error("bad input (truncated)");
      if (num_operations == max_operations) {
        throw std::runtime_error("bad input (too many operations)");
      }
      if (!('0' <= input[0] && input[0] <= '7')) {
        throw std::runtime_error("bad input (invalid 3-bit value)");
      }
      buffer[num_operations++] = input[0] - '0';
      if (input[1] == '\n') break;
      if (input[1] != ',') throw std::runtime_error("bad input (code syntax)");
      input.remove_prefix(2);
    }
    code = std::span(buffer).subspan(0, num_operations);
  }

  int a, b, c;
  std::uint8_t buffer[16];
  std::span<std::uint8_t> code;
};

std::string_view Part1(Input& input, std::span<char> output_buffer) {
  int a = input.a, b = input.b, c = input.c;
  int pc = 0;
  const auto combo = [&](std::uint8_t x) -> int {
    if (x < 4) return x;
    switch (x) {
      case 4: return a;
      case 5: return b;
      case 6: return c;
    }
    std::abort();
  };
  char* out = output_buffer.data();
  int space = output_buffer.size();
  const auto emit = [&](int x) {
    auto [i, n] = std::format_to_n(out, space, "{},", x);
    if (n > space) throw std::runtime_error("output too long");
    space -= n;
    out = i;
  };
  while (pc + 1 < int(input.code.size())) {
    const std::uint8_t op = input.code[pc], arg = input.code[pc + 1];
    pc += 2;
    switch (op) {
      case kAdv:
        a /= 1 << combo(arg);
        break;
      case kBxl:
        b ^= arg;
        break;
      case kBst:
        b = combo(arg) % 8;
        break;
      case kJnz:
        if (a != 0) pc = arg;
        break;
      case kBxc:
        b ^= c;
        break;
      case kOut:
        emit(combo(arg) % 8);
        break;
      case kBdv:
        b = a / (1 << combo(arg));
        break;
      case kCdv:
        c = a / (1 << combo(arg));
        break;
    }
  }
  if (out == output_buffer.data()) throw std::runtime_error("no output");
  assert(out[-1] == ',');
  return std::string_view(output_buffer.data(), out - 1);
}

}  // namespace

Task<void> Day17(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  char part1_buffer[256];
  std::string_view part1 = Part1(input, part1_buffer);
  std::println("part1: {}", part1);

  char result[32];
  const char* end = std::format_to(result, "{}\n", part1);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
