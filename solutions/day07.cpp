#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

#include <cctype>
#include <algorithm>
#include <cstring>
#include <print>
#include <ranges>

namespace aoc2024 {
namespace {

struct Record {
  static constexpr int kMaxValues = 15;

  std::uint64_t target;
  std::uint16_t num_values = 0;
  std::uint16_t values[kMaxValues];
};

template <std::integral I>
I ParseInt(std::string_view value) {
  I result;
  const char* first = value.data();
  const char* last = first + value.size();
  auto [i, error] = std::from_chars(first, last, result);
  if (i != last || error != std::errc()) throw std::runtime_error("bad int");
  return result;
}

struct Input {
  static constexpr int kMaxRecords = 850;

  Task<void> Read(tcp::Socket& socket) {
    char buffer[26000];
    std::string_view input(co_await socket.Read(buffer));
    assert(input.back() == '\n');
    input.remove_suffix(1);

    for (const auto line_range : std::ranges::views::split(input, '\n')) {
      std::string_view line(line_range);
      if (num_records == kMaxRecords) {
        throw std::runtime_error("too many lines");
      }
      assert(num_records < kMaxRecords);
      Record& record = records[num_records++];
      const auto separator = line.find(": ");
      if (separator == line.npos) throw std::runtime_error("bad line");
      record.target = ParseInt<std::uint64_t>(line.substr(0, separator));
      for (const auto entry :
           std::ranges::views::split(line.substr(separator + 2), ' ')) {
        if (record.num_values == Record::kMaxValues) {
          throw std::runtime_error("too many values in line");
        }
        record.values[record.num_values++] =
            ParseInt<std::uint16_t>(std::string_view(entry));
      }
    }
  }

  int num_records = 0;
  Record records[kMaxRecords];
};

// Attempts to consume a suffix of `value` which matches `suffix` when the
// values are expressed in decimal. If successful, `value` is updated to the
// remaining prefix and the function returns true. Otherwise, `value` is not
// changed.
bool ConsumeSuffix(std::uint64_t& value, std::uint16_t suffix) {
  assert(suffix != 0);
  std::uint64_t copy = value;
  while (suffix != 0) {
    if (copy % 10 != suffix % 10) return false;
    copy /= 10;
    suffix /= 10;
  }
  value = copy;
  return true;
}

template <bool use_concatenation>
bool CanProduce(std::span<const std::uint16_t> values, std::uint64_t target) {
  assert(!values.empty());
  if (values.size() == 1) return values[0] == target;
  if (target < values.back()) return false;
  const std::uint16_t last = values.back();
  const std::span<const std::uint16_t> before =
      values.subspan(0, values.size() - 1);
  // Assuming the last operation is an addition, check if we can produce the
  // remaining amount required.
  if (CanProduce<use_concatenation>(before, target - last)) return true;
  // Assuming the last operation is a multiplication and the target is divisible
  // by the last number, check if we can produce the quotient.
  assert(last != 0);
  if (target % last == 0 &&
      CanProduce<use_concatenation>(before, target / last)) {
    return true;
  }
  if (!use_concatenation) return false;
  // Assuming the last operation is a concatenation and the last number matches
  // a suffix of the target number, check if we can produce the prefix.
  if (ConsumeSuffix(target, last)) {
    return CanProduce<use_concatenation>(before, target);
  }
  return false;
}

template <auto F>
std::uint64_t CalibrationResult(const Input& input) {
  std::uint64_t total = 0;
  for (const Record& record : std::span(input.records, input.num_records)) {
    if (F(std::span(record.values, record.num_values), record.target)) {
      total += record.target;
    }
  }
  return total;
}

}  // namespace

Task<void> Day07(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const std::uint64_t part1 = CalibrationResult<CanProduce<false>>(input);
  const std::uint64_t part2 = CalibrationResult<CanProduce<true>>(input);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
