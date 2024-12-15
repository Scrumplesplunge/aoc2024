#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>
#include <ranges>

namespace aoc2024 {
namespace {

struct Vec {
  friend bool operator==(const Vec&, const Vec&) = default;
  std::int8_t x, y;
};
Vec operator+(Vec l, Vec r) { return Vec(l.x + r.x, l.y + r.y); }
Vec operator-(Vec l, Vec r) { return Vec(l.x - r.x, l.y - r.y); }
Vec& operator+=(Vec& l, Vec r) { return l = l + r; }

struct Grid {
  char& operator[](int x, int y) {
    assert(0 <= x && x < width && 0 <= y && y < height);
    return data[y * (width + 1) + x];
  }

  char& operator[](Vec v) { return (*this)[v.x, v.y]; }

  std::span<char> data;
  int width, height;
};

struct Input {
  Input() = default;

  // Non-copyable (self-referential).
  Input(const Input&) = delete;
  Input& operator=(const Input&) = delete;

  Task<void> Read(tcp::Socket& socket) {
    std::span<char> input = co_await socket.Read(input_buffer);
    if (input.empty() || input.back() != '\n') {
      throw std::runtime_error("bad input (truncated)");
    }

    // This must succeed: the input ends with '\n'.
    grid.width = std::string_view(input).find('\n');
    // This can fail, so we need to check for npos and avoid overflow by
    // preserving the size_type instead of using an int.
    const auto grid_end = std::string_view(input).find("\n\n");
    if (grid_end == std::string_view::npos) {
      throw std::runtime_error("bad input (no grid end)");
    }
    if ((grid_end + 1) % (grid.width + 1) != 0) {
      throw std::runtime_error("grid is not rectangular");
    }
    grid.data = input.subspan(0, grid_end);
    grid.height = (grid_end + 1) / (grid.width + 1);
    input = input.subspan(grid_end + 2);

    const auto robot_index = std::string_view(grid.data).find('@');
    if (robot_index == std::string_view::npos) {
      throw std::runtime_error("no robot");
    }
    robot = Vec(robot_index % (grid.width + 1), robot_index / (grid.width + 1));

    // Parse the sequence lines.
    int num_sequences = 0;
    const int max_sequences = std::size(sequence_buffer);
    if (input.empty()) throw std::runtime_error("bad input (no sequences)");
    input = input.subspan(0, input.size() - 1);
    for (const auto line : std::ranges::views::split(input, '\n')) {
      if (num_sequences == max_sequences) {
        throw std::runtime_error("too many sequences");
      }
      std::span sequence(line);
      if (std::string_view(sequence).find_first_not_of("^v<>") !=
          std::string_view::npos) {
        throw std::runtime_error("bad instruction in sequence");
      }
      sequence_buffer[num_sequences++] = sequence;
    }
    sequences = std::span(sequence_buffer).subspan(0, num_sequences);
  }

  char input_buffer[24000];
  std::span<char> sequence_buffer[20];
  Grid grid;
  Vec robot;
  std::span<std::span<char>> sequences;
};

Vec Direction(char direction) {
  switch (direction) {
    case '^': return Vec(0, -1);
    case 'v': return Vec(0, 1);
    case '<': return Vec(-1, 0);
    case '>': return Vec(1, 0);
  }
  std::abort();
}

int Part1(const Input& input) {
  // Make a copy of the input grid which we can modify.
  char buffer[2550];
  assert(input.grid.data.size() <= 2550);
  std::span<char> data = std::span(buffer).subspan(0, input.grid.data.size());
  std::ranges::copy(input.grid.data, data.data());
  Grid grid = Grid(data, input.grid.width, input.grid.height);
  Vec robot = input.robot;
  for (std::span<char> sequence : input.sequences) {
    for (char move : sequence) {
      const Vec d = Direction(move);
      switch (grid[robot + d]) {
        case '#':
          break;
        case '.':
          grid[robot] = '.';
          robot += d;
          grid[robot] = '@';
          break;
        case 'O': {
          Vec i = robot + d;
          while (grid[i] == 'O') i += d;
          if (grid[i] == '.') {
            grid[i] = 'O';
            grid[robot + d] = '@';
            grid[robot] = '.';
            robot = robot + d;
          }
          break;
        }
      }
    }
  }
  // 1554744 too high
  //   (fixed bug where I wasn't updating the robot position in the grid).
  // 1552463 right answer
  int total = 0;
  for (int y = 0; y < grid.height; y++) {
    for (int x = 0; x < grid.width; x++) {
      if (grid[x, y] == 'O') total += 100 * y + x;
    }
  }
  return total;
}

struct ExpandedGrid {
  Grid grid;
  Vec robot;
};

// Expands the input grid into the wider grid for part 2, using the provided
// buffer for storage space.
ExpandedGrid ExpandGrid(Grid input, std::span<char> buffer) {
  const int required_size = (input.width * 2 + 1) * input.height;
  assert(required_size <= int(buffer.size()));
  Grid output{.data = buffer.subspan(0, required_size),
              .width = 2 * input.width,
              .height = input.height};
  std::optional<Vec> robot;
  for (int y = 0; y < input.height; y++) {
    for (int x = 0; x < input.width; x++) {
      switch (input[x, y]) {
        case '#':
        case '.':
          output[2 * x, y] = output[2 * x + 1, y] = input[x, y];
          break;
        case 'O':
          output[2 * x, y] = '[';
          output[2 * x + 1, y] = ']';
          break;
        case '@':
          robot = Vec(2 * x, y);
          output[2 * x, y] = '@';
          output[2 * x + 1, y] = '.';
          break;
        default:
          throw std::runtime_error("unexpected character in grid");
      }
    }
  }
  if (!robot) throw std::runtime_error("no robot");
  return {.grid = output, .robot = *robot};
}

std::span<Vec> LeftPushableBoxes(Vec robot, Grid grid, std::span<Vec> boxes) {
  const int y = robot.y;
  assert((grid[robot.x - 2, y] == '[' && grid[robot.x - 1, y] == ']'));
  int num_boxes = 0;
  const int max_boxes = boxes.size();
  int x = robot.x - 1;
  while (true) {
    if (grid[x, y] != ']') break;
    if (num_boxes == max_boxes) throw std::runtime_error("too many boxes move");
    boxes[num_boxes++] = Vec(x - 1, y);
    x -= 2;
  }
  return grid[x, y] == '#' ? std::span<Vec>() : boxes.subspan(0, num_boxes);
}

std::span<Vec> RightPushableBoxes(Vec robot, Grid grid, std::span<Vec> boxes) {
  const int y = robot.y;
  assert((grid[robot.x + 1, y] == '[' && grid[robot.x + 2, y] == ']'));
  int num_boxes = 0;
  const int max_boxes = boxes.size();
  int x = robot.x + 1;
  while (true) {
    if (grid[x, y] != '[') break;
    if (num_boxes == max_boxes) throw std::runtime_error("too many boxes move");
    boxes[num_boxes++] = Vec(x, y);
    x += 2;
  }
  return grid[x, y] == '#' ? std::span<Vec>() : boxes.subspan(0, num_boxes);
}

std::span<Vec> VerticallyPushableBoxes(Vec robot, int dy, Grid grid,
                                       std::span<Vec> boxes) {
  const Vec initial = robot + Vec(0, dy);
  assert(grid[initial] == '[' || grid[initial] == ']');
  int num_boxes = 0;
  const int max_boxes = boxes.size();
  auto add_box = [&](Vec box) {
    if (num_boxes == max_boxes) throw std::runtime_error("too many boxes move");
    boxes[num_boxes++] = box;
  };
  // We track box positions by the position of the '['.
  add_box(grid[initial] == '[' ? initial : initial - Vec(1, 0));
  for (int i = 0; i < num_boxes; i++) {
    const Vec b = boxes[i];
    assert(grid[b] == '[' && grid[b + Vec(1, 0)] == ']');
    if (grid[b.x, b.y + dy] == '#' || grid[b.x + 1, b.y + dy] == '#') {
      // The robot is transitively pushing against a wall, so nothing will move.
      return {};
    }
    if (grid[b.x, b.y + dy] == '[') add_box(Vec(b.x, b.y + dy));
    if (grid[b.x, b.y + dy] == ']') add_box(Vec(b.x - 1, b.y + dy));
    if (grid[b.x + 1, b.y + dy] == '[') add_box(Vec(b.x + 1, b.y + dy));
  }
  return boxes.subspan(0, num_boxes);
}

// Finds all boxes that can be transitively pushed from a given position.
// This must only be called when the robot is pushing against a box. If an empty
// span is returned, this means that no box can be pushed and the robot should
// not move.
std::span<Vec> PushableBoxes(Vec robot, Vec direction, Grid grid,
                             std::span<Vec> boxes) {
  assert(!boxes.empty());
  if (direction.x == -1) return LeftPushableBoxes(robot, grid, boxes);
  if (direction.x == 1) return RightPushableBoxes(robot, grid, boxes);
  assert(direction.x == 0 && direction.y != 0);
  return VerticallyPushableBoxes(robot, direction.y, grid, boxes);
}

int Part2(const Input& input) {
  char buffer[5050];
  auto [grid, robot] = ExpandGrid(input.grid, buffer);
  for (std::span<char> sequence : input.sequences) {
    for (char move : sequence) {
      const Vec d = Direction(move);
      switch (grid[robot + d]) {
        case '#':
          break;
        case '.':
          grid[robot] = '.';
          robot += d;
          grid[robot] = '@';
          break;
        case '[':
        case ']': {
          Vec box_buffer[100];
          std::span<Vec> boxes = PushableBoxes(robot, d, grid, box_buffer);
          if (boxes.empty()) break;
          for (Vec box : boxes) {
            grid[box] = '.';
            grid[box.x + 1, box.y] = '.';
          }
          for (Vec box : boxes) {
            Vec p = box + d;
            grid[p] = '[';
            grid[p.x + 1, p.y] = ']';
          }
          grid[robot] = '.';
          robot += d;
          grid[robot] = '@';
          break;
        }
      }
    }
  }
  int total = 0;
  for (int y = 0; y < grid.height; y++) {
    for (int x = 0; x < grid.width; x++) {
      if (grid[x, y] == '[') total += 100 * y + x;
    }
  }
  return total;
}

}  // namespace

Task<void> Day15(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const int part1 = Part1(input);
  const int part2 = Part2(input);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
