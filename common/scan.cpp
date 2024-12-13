#include "scan.hpp"

#include <cassert>
#include <stdexcept>

namespace aoc2024 {
namespace {

bool ConsumePrefix(std::string_view& input, std::string_view prefix) {
  if (!input.starts_with(prefix)) return false;
  input.remove_prefix(prefix.size());
  return true;
}

}  // namespace

bool VScan(std::string_view input, std::string_view format,
           std::span<const ArgumentParser> args) {
  int next_arg = 0;
  const int num_args = args.size();
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
      if (!args[next_arg++](input)) return false;
      format.remove_prefix(2);
    } else {
      throw std::logic_error("unescaped '}'");
    }
  }
}

}  // namespace aoc2024
