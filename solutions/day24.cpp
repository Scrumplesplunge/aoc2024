#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <print>

namespace aoc2024 {
namespace {

class Id {
 public:
  Id() = default;

  explicit Id(std::string_view name) {
    assert(name.size() <= 3);
    std::memcpy(data_, name.data(), name.size());
    data_[name.size()] = '\0';
  }

  std::string_view Name() const { return data_; }
  operator std::string_view() const { return Name(); }

  friend inline bool operator==(const Id& l, const Id& r) = default;
  friend inline auto operator<=>(const Id& l, const Id& r) = default;

 private:
  char data_[4];
};

bool IsLower(char c) { return 'a' <= c && c <= 'z'; }
bool IsDigit(char c) { return '0' <= c && c <= '9'; }
bool IsName(char c) { return IsLower(c) || IsDigit(c); }

bool ScanValue(std::string_view& input, Id& id) {
  if (input.size() < 3) return false;
  if (!IsLower(input[0]) || !IsName(input[1]) || !IsName(input[2])) {
    return false;
  }
  id = Id(input.substr(0, 3));
  input.remove_prefix(3);
  return true;
}

struct Gate {
  enum Type : std::uint8_t {
    kConst,  // a
    kAnd,    // gates[a] & gates[b]
    kOr,     // gates[a] | gates[b]
    kXor,    // gates[a] ^ gates[b]
  };
  Id id;
  Type type;

  struct Wires { Id a, b; };
  union {
    Wires wires;
    bool value;
  };
};

bool ScanValue(std::string_view& input, Gate::Type& type) {
  if (input.size() < 3) return false;
  if (ScanPrefix(input, "AND")) {
    type = Gate::kAnd;
  } else if (ScanPrefix(input, "OR")) {
    type = Gate::kOr;
  } else if (ScanPrefix(input, "XOR")) {
    type = Gate::kXor;
  } else {
    return false;
  }
  return true;
}

struct Input {
  Task<void> Read(tcp::Socket& socket) {
    char buffer[5000];
    std::string_view input(co_await socket.Read(buffer));

    {
      Id id;
      std::uint8_t value;
      while (ScanPrefix(input, "{}: {}\n", id, value)) {
        if (num_gates == kMaxGates) throw std::runtime_error("too many gates");
        gates[num_gates++] = {.id = id, .type = Gate::kConst, .value = !!value};
      }
    }
    if (!ScanPrefix(input, "\n")) throw std::runtime_error("syntax");

    Gate::Type type;
    Id a, b, c;
    while (ScanPrefix(input, "{} {} {} -> {}\n", a, type, b, c)) {
      if (num_gates == kMaxGates) throw std::runtime_error("too many gates");
      gates[num_gates++] = {.id = c, .type = type, .wires = {.a = a, .b = b}};
    }
  }

  std::uint16_t Get(Id id) const {
    for (int i = 0; i < num_gates; i++) {
      if (gates[i].id == id) return i;
    }
    throw std::runtime_error("not found");
  }

  auto& operator[](this auto&& self, Id id) {
    return self.gates[self.Get(id)];
  }

  static constexpr int kMaxGates = 512;
  int num_gates = 0;
  Gate gates[kMaxGates];
};

std::uint64_t Part1(const Input& input) {
  std::map<Id, bool> values;
  for (int i = 0; i < input.num_gates; i++) {
    if (input.gates[i].type == Gate::kConst) {
      values[input.gates[i].id] = input.gates[i].value;
    }
  }
  while (int(values.size()) < input.num_gates) {
    // Add any gates which are now ready.
    for (int i = 0; i < input.num_gates; i++) {
      const Gate& gate = input.gates[i];
      if (values.contains(gate.id)) continue;
      const auto a = values.find(gate.wires.a);
      const auto b = values.find(gate.wires.b);
      if (a == values.end() || b == values.end()) continue;
      switch (gate.type) {
        case Gate::kAnd:
          values[gate.id] = a->second && b->second;
          break;
        case Gate::kOr:
          values[gate.id] = a->second || b->second;
          break;
        case Gate::kXor:
          values[gate.id] = a->second != b->second;
          break;
        default:
          throw std::logic_error("bad gate");
      }
    }
  }
  const auto first = values.lower_bound(Id("z00"));
  const auto last = values.upper_bound(Id("z99"));
  // 28415794793469 too low
  //   (bits were reversed)
  // 52728619468518 correct
  std::uint64_t value = 0;
  std::uint64_t shift = 1;
  for (auto i = first; i != last; i++) {
    if (i->second) value |= shift;
    shift <<= 1;
  }
  return value;
}

int Part2(const Input&) {
  return 0;
}

}  // namespace

Task<void> Day24(tcp::Socket& socket) {
  std::println("parsing input...");
  Input input;
  co_await input.Read(socket);

  const std::uint64_t part1 = Part1(input);
  const int part2 = Part2(input);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
