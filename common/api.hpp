#ifndef AOC2024_API_HPP_
#define AOC2024_API_HPP_

#include <cassert>
#include <chrono>
#include <cstdint>
#include <format>
#include <span>

namespace aoc2024 {

struct RequestHeader {
  static constexpr int kNumBytes = 1;

  static RequestHeader Decode(std::span<const char, kNumBytes> bytes);
  static RequestHeader Decode(std::span<const char> bytes);
  void EncodeTo(std::span<char, kNumBytes> bytes) const;

  std::int8_t day;
};

// Only uses the top two bits of the byte. Other bits are repurposed.
enum class ResponsePacketType : std::uint8_t {
  // Channels used for textual output to stdout/stderr respectively. Payload is
  // a 16-bit length followed by the corresponding number of payload bytes.
  kOutput = 0 << 6,
  kDebug = 1 << 6,
  // A timing sample for a given event label. Payload is a 32-bit time in
  // microseconds relative to the start of the request, then an 8-bit label
  // length, then the corresponding number of bytes for the label.
  kEvent = 2 << 6,
};

enum class Event : std::uint8_t {
  kRequestParsed,  // Request parsed.
  kInputParsed,    // Input parsed (if done separately from solving).
  kPart1Done,      // Part 1 solved (if done separately from part 2).
  kDone,           // Both parts solved.
};

class Response {
 public:
  explicit Response(std::span<char> buffer)
      : buffer_(buffer), unused_(buffer) {}

  template <typename... Args>
  void Print(std::format_string<Args...> format, Args&&... args);

  template <typename... Args>
  void Debug(std::format_string<Args...> format, Args&&... args);

  void RecordEvent(Event event);

  std::span<char> bytes() const;

 private:
  using Clock = std::chrono::steady_clock;
  using Time = Clock::time_point;

  void Advance(int x);

  template <typename... Args>
  void Format(ResponsePacketType type, std::format_string<Args...> format,
              Args&&... args);

  const Time start_ = Clock::now();
  // `buffer_` is the entire available space, `unused_` is the suffix which has
  // not yet been written to.
  const std::span<char> buffer_;
  std::span<char> unused_;
};

template <typename... Args>
void Response::Print(std::format_string<Args...> format, Args&&... args) {
  Format<Args...>(ResponsePacketType::kDebug, format,
                  std::forward<Args>(args)...);
}

template <typename... Args>
void Response::Debug(std::format_string<Args...> format, Args&&... args) {
  Format<Args...>(ResponsePacketType::kDebug, format,
                  std::forward<Args>(args)...);
}

template <typename... Args>
void Response::Format(ResponsePacketType type,
                      std::format_string<Args...> format, Args&&... args) {
  // Header byte has the type tag plus 6 bits of the size, then another byte for
  // 8 more bits of the size. The size is little-endian.
  char* header = unused_.data();
  Advance(2);
  auto [_, required_bytes] = std::format_to_n(
      unused_.data(), unused_.size(), format, std::forward<Args>(args)...);
  Advance(required_bytes);
  assert(required_bytes < (1 << (6 + 8)));
  header[0] = std::uint8_t(type) | std::uint8_t(required_bytes >> 8);
  header[1] = std::uint8_t(required_bytes);
}

}  // namespace aoc2024

#endif  // AOC2024_API_HPP_
