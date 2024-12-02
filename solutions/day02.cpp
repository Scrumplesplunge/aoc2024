#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cstring>
#include <print>

namespace aoc2024 {

class BufferedReader {
 public:
  explicit BufferedReader(tcp::Socket& socket, std::span<char> buffer)
      : socket_(&socket), buffer_(buffer) {}

  // Read a line. The returned span is only valid until the next read operation.
  Task<std::string_view> ReadLine() {
    auto i = std::find(data_.begin(), data_.end(), '\n');
    if (i == data_.end()) {
      co_await Refill();
      i = std::find(data_.begin(), data_.end(), '\n');
      if (i == data_.end()) throw std::runtime_error("line is too long");
    }
    const std::string_view line(data_.data(), i - data_.begin());
    data_ = data_.subspan(i + 1 - data_.begin());
    co_return line;
  }

  Task<std::span<char>> ReadLongLine(std::span<char> buffer) {
    std::span<char> unused = buffer;
    while (true) {
      const auto i = std::find(data_.begin(), data_.end(), '\n');
      const std::span<char> bytes(data_.begin(), i - data_.begin());
      if (bytes.size() > unused.size()) {
        throw std::runtime_error("line is too long");
      }
      std::memcpy(unused.data(), bytes.data(), bytes.size());
      if (i != data_.end()) {
        // Remove the rest of the line, including the newline, from the buffer.
        data_ = data_.subspan(i + 1 - data_.begin());
        co_return buffer.subspan(0, buffer.size() - unused.size());
      }
      // More to come.
      unused = unused.subspan(bytes.size());
      if (!co_await Refill()) {
        throw std::runtime_error("unterminated line");
      }
    }
  }

  bool AtEnd() const { return data_.empty() && eof_; }

 private:
  Task<bool> Refill() {
    std::memmove(buffer_.data(), data_.data(), data_.size());
    const std::span<char> new_data =
        co_await socket_->Read(buffer_.subspan(data_.size()));
    data_ = buffer_.subspan(0, data_.size() + new_data.size());
    eof_ = data_.size() < buffer_.size();
    co_return data_.size();
  }

  tcp::Socket* socket_;
  std::span<char> buffer_;
  std::span<char> data_;
  bool eof_ = false;
};

int ConsumeInt(std::string_view& text) {
  int value;
  const auto i = text.find_first_not_of(" ");
  if (i == text.npos) throw std::runtime_error("expected int in " + std::string(text));
  text.remove_prefix(i);
  auto [end, error] = std::from_chars(text.begin(), text.end(), value);
  if (error != std::errc()) throw std::runtime_error("expected int in " + std::string(text));
  text.remove_prefix(end - text.begin());
  return value;
}

bool IsSafe(std::span<const std::int8_t> values) {
  assert(values.size() >= 2);
  const int a = values[0];
  const int b = values[1];
  const int delta = std::abs(b - a);
  if (delta < 1 || 3 < delta) return false;  // Initial values too far apart.
  const bool ascending = a < b;
  int previous = b;
  for (int i = 2, n = values.size(); i < n; i++) {
    if ((previous < values[i]) != ascending) return false;  // Direction change.
    const int delta = std::abs(values[i] - previous);
    if (delta < 1 || 3 < delta) return false;  // Values too far apart.
    previous = values[i];
  }
  return true;
}

bool IsMostlySafe(std::span<const std::int8_t> values) {
  assert(values.size() <= 8);
  std::int8_t buffer[8];
  std::memcpy(buffer, values.data() + 1, values.size() - 1);
  const std::span<std::int8_t> most_values(buffer, values.size() - 1);
  if (IsSafe(most_values)) return true;
  // This forward pass will overwrite to produce all sequences excluding one
  // element: 1 2 3 4; [0] 2 3 4; 0 [1] 2 4; 0 1 [2] 3.
  // The last iteration will write `buffer[n - 1]` which is one past the end of
  // `most_values` but within bounds for `buffer`.
  for (int i = 0, n = values.size(); i < n; i++) {
    buffer[i] = values[i];
    if (IsSafe(most_values)) return true;
  }
  return false;
}

Task<void> Day02(tcp::Socket& socket) {
  char buffer[1000];
  BufferedReader reader(socket, buffer);

  int num_safe = 0;
  int num_mostly_safe = 0;
  while (!reader.AtEnd()) {
    std::string_view line = co_await reader.ReadLine();

    // Parse the values.
    std::int8_t buffer[8];
    int num_values = 0;
    while (!line.empty()) {
      assert(num_values < 8);
      buffer[num_values++] = ConsumeInt(line);
    }
    const std::span<const std::int8_t> values(buffer, num_values);
    num_safe += IsSafe(values);
    num_mostly_safe += IsMostlySafe(values);
  }

  std::println("part1: {}\npart2: {}", num_safe, num_mostly_safe);

  char result[32];
  const char* end =
      std::format_to(result, "{}\n{}\n", num_safe, num_mostly_safe);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
