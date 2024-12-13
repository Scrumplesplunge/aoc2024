#ifndef AOC2024_SCAN_HPP_
#define AOC2024_SCAN_HPP_

#include <charconv>
#include <concepts>
#include <span>
#include <string_view>

namespace aoc2024 {

template <std::integral T>
bool ScanValue(std::string_view& input, T& value) {
  auto [i, error] =
      std::from_chars(input.data(), input.data() + input.size(), value);
  if (error != std::errc()) return false;
  input.remove_prefix(i - input.data());
  return true;
}

template <typename T>
concept Scannable = requires (std::string_view& input, T& result) {
  { ScanValue(input, result) } -> std::same_as<bool>;
};

class ArgumentParser {
 public:
  template <Scannable T>
  explicit ArgumentParser(T& value) {
    data_ = &value;
    func_ = [](std::string_view& input, void* erased_output) -> bool {
      return ScanValue(input, *reinterpret_cast<T*>(erased_output));
    };
  }

  bool operator()(std::string_view& input) const { return func_(input, data_); }

 private:
  void* data_;
  bool (*func_)(std::string_view& format, void* data);
};

bool VScanPrefix(std::string_view& input, std::string_view format,
                 std::span<const ArgumentParser> args);
bool VScan(std::string_view input, std::string_view format,
           std::span<const ArgumentParser> args);

template <Scannable... Args>
bool ScanPrefix(std::string_view& input, std::string_view format,
                Args&... args) {
  const ArgumentParser parsers[] = {ArgumentParser(args)...};
  return VScanPrefix(input, format, parsers);
}

template <Scannable... Args>
bool Scan(std::string_view input, std::string_view format, Args&... args) {
  const ArgumentParser parsers[] = {ArgumentParser(args)...};
  return VScan(input, format, parsers);
}

}  // namespace aoc2024

#endif  // AOC2024_SCAN_HPP_
