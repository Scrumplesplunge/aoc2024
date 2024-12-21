#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>

namespace aoc2024 {
namespace {

struct Input {
  struct Code {
    std::string_view text;
    int number;
  };

  Task<void> Read(tcp::Socket& socket) {
    std::string_view input(co_await socket.Read(buffer));
    if (input.size() != 25) throw std::runtime_error("bad input");
    for (int code = 0; code < 5; code++) {
      int number = 0;
      const std::string_view entry = input.substr(5 * code, 5);
      for (int i = 0; i < 3; i++) {
        if (!('0' <= entry[i] && entry[i] <= '9')) {
          throw std::runtime_error("bad code");
        }
        number = 10 * number + (entry[i] - '0');
      }
      if (entry[3] != 'A' || entry[4] != '\n') {
        throw std::runtime_error("bad code");
      }
      codes[code] = Code{.text = entry.substr(0, 4), .number = number};
    }
  }

  char buffer[26];
  Code codes[5];
};

struct Vec { int x, y; };
Vec operator-(Vec l, Vec r) { return Vec(l.x - r.x, l.y - r.y); }

struct Grid {
  Vec operator[](char c) const {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 3; x++) {
        if (buttons[y][x] == c) return Vec(x, y);
      }
    }
    throw std::logic_error("not found");
  }

  auto& operator[](this auto&& self, Vec position) {
    return self.buttons[position.y][position.x];
  }

  char buttons[4][3];
};

template <Grid kGrid, bool kUpFirst>
std::string_view Type(std::string_view code, std::span<char>& output_buffer) {
  char* const first = output_buffer.data();
  char* const end = first + output_buffer.size();
  char* out = first;
  Vec position = kGrid['A'];
  for (char c : code) {
    // 8 is bigger than the maximum path between two buttons.
    if (end - out < 8) throw std::runtime_error("not enough space");
    const Vec target = kGrid[c];
    const Vec delta = target - position;
    position = target;
    if (kUpFirst) {
      for (int i = delta.y; i < 0; i++) *out++ = '^';
    } else {
      for (int i = 0; i < delta.y; i++) *out++ = 'v';
    }
    for (int i = delta.x; i < 0; i++) *out++ = '<';
    for (int i = 0; i < delta.x; i++) *out++ = '>';
    if (kUpFirst) {
      for (int i = 0; i < delta.y; i++) *out++ = 'v';
    } else {
      for (int i = delta.y; i < 0; i++) *out++ = '^';
    }
    *out++ = 'A';
  }
  output_buffer = output_buffer.subspan(out - first);
  return std::string_view(first, out - first);
}

constexpr Grid kNumpad = {
    .buttons = {{'7',  '8', '9'},
                {'4',  '5', '6'},
                {'1',  '2', '3'},
                {'\0', '0', 'A'}},
};

constexpr Grid kArrows = {
    .buttons = {{'\0', '^', 'A'},
                {'<',  'v', '>'}},
};

std::string_view TypeOnNumpad(std::string_view code,
                              std::span<char>& output_buffer) {
  return Type<kNumpad, true>(code, output_buffer);
}

std::string_view TypeOnArrows(std::string_view code,
                              std::span<char>& output_buffer) {
  return Type<kArrows, false>(code, output_buffer);
}

// 6327 too low
//
// 379A
//   789
//   456
//   123
//    0A
// ^A^^<<A>>AvvvA
//    ^A
//   <v>
// <A>A<AAv<AA>>^AvAA^Av<AAA>^A
// v<<A>>^AvA^Av<<A>>^AAv<A<A>>^AAvAA<^A>Av<A>^AA<A>Av<A<A>>^AAAvA<^A>A

std::string Eval(std::string_view code, const Grid& grid) {
  Vec position = grid['A'];
  std::string result;
  for (char c : code) {
    switch (c) {
      case '^':
        position.y--;
        break;
      case 'v':
        position.y++;
        break;
      case '<':
        position.x--;
        break;
      case '>':
        position.x++;
        break;
      case 'A':
        result.push_back(grid[position]);
        break;
      default:
        throw std::runtime_error("bad position");
    }
  }
  return result;
}

int Part1(const Input& input) {
  int total = 0;
  for (int i = 0; i < 5; i++) {
    char buffer[1024];
    std::span<char> unused = buffer;
    const Input::Code code = input.codes[i];
    std::println("{}", code.text);
    const std::string_view l1 = TypeOnNumpad(code.text, unused);
    if (Eval(l1, kNumpad) != code.text) {
      std::println("BAD: {}", l1);
      return 0;
    }
    std::println("{}", l1);
    const std::string_view l2 = TypeOnArrows(l1, unused);
    if (Eval(l2, kArrows) != l1) {
      std::println("BAD: {}", l2);
      return 0;
    }
    std::println("{}", l2);
    const std::string_view l3 = TypeOnArrows(l2, unused);
    if (Eval(l3, kArrows) != l2) {
      std::println("BAD: {}", l3);
      return 0;
    }
    std::println("{}", l3);
    const int sequence_length = l3.size();
    std::println("{} * {}", sequence_length, code.number);
    total += sequence_length * code.number;
  }
  return total;
}

}  // namespace

Task<void> Day21(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  //     3            7                           9           A
  // me: ^A           ^^<<A                       >>A         vvvA
  // eg: ^A           <<^^A                       >>A         vvvA
  //
  // me: <A>A         <AAv<AA>>^A                 vAA^A       v<AAA>^A
  // eg: <A>A         v<<AA>^AA>A                 vAA^A       <vAAA>^A
  //
  // me: v<<A>>^AvA^A v<<A>>^AAv<A<A>>^AAvAA<^A>A v<A>^AA<A>A v<A<A>>^AAAvA<^A>A
  // eg: <v<A>>^AvA^A <vA<AA>>^AAvA<^A>AAvA^A     <vA>^AA<A>A <v<A>A>^AAAvA<^A>A

  std::println("TEST for 379A:");
  std::string code = "<v<A>>^AvA^A<vA<AA>>^AAvA<^A>AAvA^A<vA>^AA<A>A<v<A>A>^AAAvA<^A>A";
  std::println("{}", code);
  code = Eval(code, kArrows);
  std::println("{}", code);
  code = Eval(code, kArrows);
  std::println("{}", code);
  std::println("{}", Eval(code, kNumpad));
  std::println("END TEST");

  const int part1 = Part1(input);
  const int part2 = 0;

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
