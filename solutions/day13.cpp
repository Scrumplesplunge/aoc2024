#include <algorithm>
#include <cctype>
#include <cstring>
#include <print>
#include <ranges>

#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

namespace aoc2024 {
namespace {

using std::literals::operator""sv;

struct Vec { std::int64_t x, y; };
struct Machine { Vec a, b, prize; };

template <typename T>
bool ReadInt(std::string_view& input, T& result) {
  auto [i, error] =
      std::from_chars(input.data(), input.data() + input.size(), result);
  if (error != std::errc()) return false;
  input.remove_prefix(i - input.data());
  return true;
}

bool ConsumePrefix(std::string_view& input, std::string_view prefix) {
  if (!input.starts_with(prefix)) return false;
  input.remove_prefix(prefix.size());
  return true;
}

class ArgumentParser {
 public:
  template <typename T>
  explicit ArgumentParser(T& value) {
    data_ = &value;
    func_ = [](std::string_view& input, void* erased_output) -> bool {
      return ReadInt(input, *reinterpret_cast<T*>(erased_output));
    };
  }

  bool operator()(std::string_view& input) const { return func_(input, data_); }

 private:
  void* data_;
  bool (*func_)(std::string_view& format, void* data);
};

template <typename... Args>
bool Scan(std::string_view input, std::string_view format, Args&... args) {
  const ArgumentParser parsers[] = {ArgumentParser(args)...};
  int next_arg = 0;
  const int num_args = sizeof...(Args);
  while (true) {
    auto literal_end = format.find_first_of("{}");
    if (literal_end == format.npos) literal_end = format.size();
    if (!ConsumePrefix(input, format.substr(0, literal_end))) return false;
    format.remove_prefix(literal_end);
    if (format.empty()) return true;
    assert(format.front() == '{' || format.front() == '}');
    if (format.size() < 2) throw std::logic_error("bad format string");
    if (format[0] == format[1]) {
      // Parse an escaped literal like `{{` or `}}`.
      if (!ConsumePrefix(input, format.substr(0, 1))) return false;
      format.remove_prefix(2);
    } else if (format[0] == '{') {
      assert(format[1] == '}');
      if (next_arg == num_args) {
        throw std::runtime_error("too many placeholders");
      }
      if (!parsers[next_arg++](input)) return false;
      format.remove_prefix(2);
    } else {
      throw std::logic_error("unescaped '}'");
    }
  }
}

Task<std::span<Machine>> ReadInput(tcp::Socket& socket,
                                   std::span<Machine> machines) {
  const int max_machines = machines.size();
  int num_machines = 0;
  char buffer[22000];
  const std::string_view input(co_await socket.Read(buffer));
  for (const auto entry : std::ranges::views::split(input, "\n\n"sv)) {
    if (num_machines == max_machines) {
      throw std::runtime_error("too many machines");
    }
    Machine& machine = machines[num_machines++];
    if (!Scan(std::string_view(entry),
              "Button A: X+{}, Y+{}\n"
              "Button B: X+{}, Y+{}\n"
              "Prize: X={}, Y={}",
              machine.a.x, machine.a.y, machine.b.x, machine.b.y,
              machine.prize.x, machine.prize.y)) {
      throw std::runtime_error("bad machine description");
    }
  }
  co_return machines.subspan(0, num_machines);
}

std::int64_t TicketsRequired(Machine m) {
  // Supposing we can reach the prize by pressing button A `p` times and
  // button B `q` times. Then:
  //
  // |ax bx|.|p| = |prize.x|
  // |ay by| |q|   |prize.y|
  //
  // This can be rearranged to:
  //
  // |p| = |ax bx|^-1 . |prize.x| = _______1_______| by -bx|.|prize.x|
  // |q|   |ay by|      |prize.y|   (ax*by - ay*bx)|-ay  ax| |prize.y|
  const std::int64_t det = m.a.x * m.b.y - m.a.y * m.b.x;
  if (det == 0) return 0;  // Unwinnable.
  const std::int64_t pdet = m.b.y * m.prize.x - m.b.x * m.prize.y;
  const std::int64_t qdet = m.a.x * m.prize.y - m.a.y * m.prize.x;
  if (pdet % det != 0 || qdet % det != 0) return 0;  // Non-integer.
  return 3 * pdet / det + qdet / det;
}

std::int64_t Part1(std::span<const Machine> machines) {
  std::int64_t total = 0;
  for (const Machine& m : machines) total += TicketsRequired(m);
  return total;
}

std::int64_t Part2(std::span<const Machine> machines) {
  std::int64_t total = 0;
  for (Machine m : machines) {
    m.prize.x += 10'000'000'000'000;
    m.prize.y += 10'000'000'000'000;
    total += TicketsRequired(m);
  }
  return total;
}

}  // namespace

Task<void> Day13(tcp::Socket& socket) {
  Machine buffer[320];
  std::span<Machine> machines = co_await ReadInput(socket, buffer);

  const std::int64_t part1 = Part1(machines);
  const std::int64_t part2 = Part2(machines);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
