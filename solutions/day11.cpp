#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <cctype>
#include <algorithm>
#include <cstring>
#include <print>
#include <ranges>
#include <unordered_map>

namespace aoc2024 {
namespace {

struct StoneType { std::uint64_t marking, count; };

bool ScanValue(std::string_view& input, StoneType& type) {
  if (!aoc2024::ScanValue(input, type.marking)) return false;
  type.count = 1;
  return true;
}

// Given a span of StoneType entries, combines counts for identical elements.
// The return value is a prefix of the input span containing the compacted
// result.
// The problem description makes a point of saying that the order of the stones
// is preserved throughout, but this doesn't actually matter, so we can use the
// count of stones. The simplest approach for this would be to use a map from
// marking to stone count and iterate forwards. However, because of the limited
// available memory on the pico, we can't get away with a std::map or
// a std::unordered_map. Instead, we use a std::vector<StoneType> and eliminate
// duplicates using `Combine` between iterations (or when we run out of space
// during an iteration because it splits too many times).
std::span<StoneType> Combine(std::span<StoneType> entries) {
  std::ranges::sort(entries, std::less<>(), &StoneType::marking);
  int j = 1;
  for (int i = 1, n = entries.size(); i < n; i++) {
    auto& previous = entries[j - 1];
    auto& current = entries[i];
    if (previous.marking == current.marking) {
      previous.count += current.count;
    } else {
      entries[j++] = current;
    }
  }
  return entries.subspan(0, j);
}

Task<std::span<StoneType>> ReadInput(tcp::Socket& socket,
                                     std::span<StoneType> buffer) {
  char text[128];
  std::string_view input(co_await socket.Read(text));
  if (!ScanPrefix(input, "{}", buffer[0])) {
    throw std::runtime_error("no stones");
  }
  int num_stones = 1;
  const int max_stones = buffer.size();
  while (!ScanPrefix(input, "\n")) {
    if (num_stones == max_stones) {
      throw std::runtime_error("too many stones");
    }
    if (!ScanPrefix(input, " {}", buffer[num_stones++])) {
      throw std::runtime_error("bad syntax");
    }
  }
  if (!input.empty()) throw std::runtime_error("multiple lines");
  co_return Combine(buffer.subspan(0, num_stones));
}

int NumDigits(std::uint64_t x) {
  int num_digits = 1;
  std::uint64_t threshold = 10;
  while (x >= threshold) {
    num_digits++;
    threshold *= 10;
  }
  return num_digits;
}

std::pair<std::uint64_t, std::uint64_t> Split(std::uint64_t x) {
  const int n = NumDigits(x);
  std::uint64_t split = 1;
  for (int i = n / 2; i > 0; i--) split *= 10;
  return {x / split, x % split};
}

std::uint64_t Count(const std::span<StoneType>& stones) {
  std::uint64_t total = 0;
  for (const auto& [marking, count] : stones) total += count;
  return total;
}

std::span<StoneType> Blink(std::span<const StoneType> before,
                           std::span<StoneType> buffer) {
  const int n = buffer.size();
  int num_stones = 0;
  auto emit = [&](std::uint64_t marking, std::uint64_t count) {
    if (num_stones == n) {
      num_stones = Combine(buffer).size();
      if (num_stones == n) throw std::runtime_error("too many stones");
    }
    buffer[num_stones++] = StoneType{.marking = marking, .count = count};
  };
  for (const auto [marking, count] : before) {
    if (marking == 0) {
      emit(1, count);
      continue;
    }
    const int n = NumDigits(marking);
    if (n % 2 == 0) {
      const auto [l, r] = Split(marking);
      emit(l, count);
      emit(r, count);
    } else {
      emit(marking * 2024, count);
    }
  }
  std::span<StoneType> result = Combine(buffer.subspan(0, num_stones));
  return result;
}

}  // namespace

Task<void> Day11(tcp::Socket& socket) {
  // We have space for two lists of stones. Each iteration reads from one buffer
  // and writes to the other buffer.
  StoneType buffers[2][4096];
  std::span<StoneType> stones = co_await ReadInput(socket, buffers[1]);
  for (int i = 0; i < 25; i++) stones = Blink(stones, buffers[i % 2]);
  const std::uint64_t part1 = Count(stones);
  for (int i = 25; i < 75; i++) stones = Blink(stones, buffers[i % 2]);
  const std::uint64_t part2 = Count(stones);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
