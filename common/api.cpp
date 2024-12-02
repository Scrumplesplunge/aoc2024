#include "api.hpp"

#include <stdexcept>

namespace aoc2024 {
namespace {

using std::chrono_literals::operator""us;

char* EmitUint8(char* p, std::uint8_t x) {
  *p = x;
  return p + 1;
}

char* EmitUint16(char* p, std::uint16_t x) {
  p = EmitUint8(p, x);
  p = EmitUint8(p, x >> 8);
  return p;
}

char* EmitUint32(char* p, std::uint32_t x) {
  p = EmitUint16(p, x);
  p = EmitUint16(p, x >> 16);
  return p;
}

}  // namespace

RequestHeader RequestHeader::Decode(
    std::span<const char, RequestHeader::kNumBytes> bytes) {
  if (!(1 <= bytes[0] && bytes[0] <= 25)) {
    throw std::runtime_error("Bad request header (invalid day).");
  }
  return RequestHeader{.day = std::int8_t(bytes[0])};
}

RequestHeader RequestHeader::Decode(std::span<const char> bytes) {
  if (bytes.size() != RequestHeader::kNumBytes) {
    throw std::runtime_error(
        "Bad request header (wrong size: " + std::to_string(bytes.size()) +
        " actual vs " + std::to_string(RequestHeader::kNumBytes) +
        " expected).");
  }
  return Decode(std::span<const char, RequestHeader::kNumBytes>(bytes));
}

void RequestHeader::EncodeTo(
    std::span<char, RequestHeader::kNumBytes> bytes) const {
  bytes[0] = day;
}

void Response::RecordEvent(Event event) {
  const Time now = Clock::now();
  const std::uint32_t micros = (now - start_) / 1us;
  // Type and event byte followed by a 32-bit duration in micros.
  char* p = unused_.data();
  Advance(5);
  p = EmitUint8(p,
                std::uint8_t(ResponsePacketType::kEvent) | std::uint8_t(event));
  p = EmitUint32(p, micros);
}

std::span<char> Response::bytes() const {
  return buffer_.subspan(0, buffer_.size() - unused_.size());
}

void Response::Advance(int x) {
  if (unused_.size() < std::size_t(x)) {
    throw std::runtime_error("response too big");
  }
  unused_ = unused_.subspan(x);
}

}  // namespace aoc2024
