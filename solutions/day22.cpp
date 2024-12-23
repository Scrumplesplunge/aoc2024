#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>

namespace aoc2024 {
namespace {

struct Input {
  Task<void> Read(tcp::Socket& socket) {
    char input_buffer[18000];
    std::string_view input(co_await socket.Read(input_buffer));

    int num_values = 0;
    for (int x; ScanPrefix(input, "{}\n", x);) {
      if (num_values == kMaxValues) throw std::runtime_error("too many lines");
      buffer[num_values++] = x;
    }
    values = std::span(buffer, num_values);
  }

  static constexpr int kMaxValues = 2100;
  int buffer[kMaxValues];
  std::span<const int> values;
};

std::uint32_t Step(std::uint32_t secret) {
  secret ^= secret * 64;
  secret %= 16777216;
  secret ^= secret / 32;
  secret %= 16777216;
  secret ^= secret * 2048;
  secret %= 16777216;
  return secret;
}

std::uint64_t Part1(const Input& input) {
  std::uint64_t total = 0;
  for (int value : input.values) {
    std::uint32_t secret = value;
    for (int i = 0; i < 2000; i++) secret = Step(secret);
    total += secret;
  }
  return total;
}

int Part2(const Input& input) {
  // The overall approach here is to simulate each sequence of 2000 values and
  // keep track of the first time we see each sequence of 4 price changes for
  // each monkey sequence, and the corresponding price we get for it. Logically,
  // we have a big array of banana counts per four-change-sequence.
  //
  // There are roughly 100k different four-change-sequence patterns (we can use
  // a sequence of 5 digits instead of four deltas, and we can normalise it by
  // subtracting the minimum digit value from each digit).
  //
  // This is too large for us to keep in the Pico's RAM all at once. Instead, we
  // can process in batches of four-change-sequence ranges by re-simulating the
  // monkey sequences multiple times and only keeping track of the counts for
  // the four-change-sequences which are within the range for the current batch.
  //
  // At the extreme end, we could iterate over every four-change sequence
  // individually (kBatchSize = 1), but we only need two batches
  // (kBatchSize = 50000) to fit within the RAM of the Pico.
  constexpr int kDomainSize = 100000;
  constexpr int kBatchSize = kDomainSize / 2;
  static_assert(kDomainSize % kBatchSize == 0);
  int best = 0;
  for (int batch_start = 0; batch_start < kDomainSize;
       batch_start += kBatchSize) {
    std::vector<std::uint16_t> counts(kBatchSize);
    const int batch_end = batch_start + kBatchSize;
    for (int value : input.values) {
      std::vector<bool> seen(kBatchSize);
      std::uint32_t secret = value;
      std::uint8_t digits[5] = {};
      digits[0] = secret % 10;
      for (int i = 1; i <= 2000; i++) {
        secret = Step(secret);
        digits[i % 5] = secret % 10;
        if (i < 4) continue;  // Wait until we have 5 values (4 price changes).
        const std::uint8_t min = *std::ranges::min_element(digits);
        int index = 0;
        for (int j = 1; j <= 5; j++) {
          const std::uint8_t digit = digits[(i + j) % 5] - min;
          index = 10 * index + digit;
        }
        if (batch_start <= index && index < batch_end) {
          index -= batch_start;
          if (seen[index]) continue;
          seen[index] = true;
          counts[index] += digits[i % 5];
        }
      }
    }
    const int batch_best = *std::ranges::max_element(counts);
    best = std::max(best, batch_best);
  }
  return best;
}

}  // namespace

Task<void> Day22(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const std::uint64_t part1 = Part1(input);
  const int part2 = Part2(input);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
