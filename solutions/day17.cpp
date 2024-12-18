#include <algorithm>
#include <generator>
#include <print>

#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

namespace aoc2024 {
namespace {

enum Operation : std::uint8_t {
  kAdv,  // a = a >> arg
  kBxl,  // b ^= lit
  kBst,  // b = a % 8
  kJnz,  // if (A != 0) goto lit
  kBxc,  // b ^= c
  kOut,  // emit(arg)
  kBdv,  // b = a >> arg
  kCdv,  // c = a >> arg
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

  std::uint64_t a, b, c;
  std::uint8_t buffer[16];
  std::span<std::uint8_t> code;
};

std::generator<std::uint8_t> Run(std::uint64_t a, std::uint64_t b,
                                 std::uint64_t c,
                                 std::span<const std::uint8_t> code) {
  int pc = 0;
  const auto combo = [&](std::uint8_t x) -> std::uint64_t {
    if (x < 4) return x;
    switch (x) {
      case 4: return a;
      case 5: return b;
      case 6: return c;
    }
    std::abort();
  };
  while (pc + 1 < int(code.size())) {
    const std::uint8_t op = code[pc], arg = code[pc + 1];
    pc += 2;
    switch (op) {
      case kAdv:
        a = a >> combo(arg);
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
        co_yield combo(arg) % 8;
        break;
      case kBdv:
        b = a >> combo(arg);
        break;
      case kCdv:
        c = a >> combo(arg);
        break;
    }
  }
}

std::string_view Part1(Input& input, std::span<char> output_buffer) {
  char* out = output_buffer.data();
  int space = output_buffer.size();
  for (std::uint8_t value : Run(input.a, input.b, input.c, input.code)) {
    auto [i, n] = std::format_to_n(out, space, "{},", value);
    if (n > space) throw std::runtime_error("output too long");
    space -= n;
    out = i;
  }
  if (out == output_buffer.data()) throw std::runtime_error("no output");
  assert(out[-1] == ',');
  return std::string_view(output_buffer.data(), out - 1);
}

// Given a `prefix` value for `a` which will result in the suffix
// `code.subspan(n + 1)` being output, search for a value of `a` which will
// result in the entire of `code` being output.
//
// This relies on the assumption that:
//
//   * The only state that survives across loop iterations is the value of `a`.
//   * Each loop iteration produces exactly one output and shifts `a` by 3, in
//     some order.
std::uint64_t FindMatch(std::uint64_t prefix, Input& input, int n) {
  const std::span<const std::uint8_t> expected = input.code.subspan(n);
  for (std::uint8_t j = 0; j < 8; j++) {
    const std::uint64_t a = prefix << 3 | j;
    if (std::ranges::equal(Run(a, input.b, input.c, input.code), expected)) {
      if (n == 0) return a;
      const std::uint64_t result = FindMatch(a, input, n - 1);
      if (result) return result;
    }
  }
  return 0;
}

std::uint64_t Part2(Input& input) {
  return FindMatch(0, input, input.code.size() - 1);
}

}  // namespace

Task<void> Day17(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  char part1_buffer[128];
  const std::string_view part1 = Part1(input, part1_buffer);
  const std::uint64_t part2 = Part2(input);
  std::println("part1: {}\npart2: {}", part1, part2);

  char result[64];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
