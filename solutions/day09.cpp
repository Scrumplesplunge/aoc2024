#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

#include <cctype>
#include <algorithm>
#include <cstring>
#include <print>
#include <ranges>

namespace aoc2024 {

std::int64_t Part1(const std::span<const char> input) {
  int free_block_index = 1;
  int free_block_space = input[1] - '0';
  int copy_index = input.size() - 1;
  std::int64_t checksum = 0;
  int output_index = input[0] - '0';
  while (free_block_index < copy_index) {
    // Need to move the file at `copy_index`.
    const int block_id = copy_index / 2;
    int block_size = input[copy_index] - '0';
    // Loop while the file continues to fill entire free blocks.
    while (block_size > free_block_space) {
      for (int i = 0; i < free_block_space; i++) {
        checksum += output_index++ * block_id;
      }
      block_size -= free_block_space;
      free_block_index += 2;
      if (free_block_index > copy_index) break;
      const int skipped_id = (free_block_index - 1) / 2;
      const int skipped_file_size = input[free_block_index - 1] - '0';
      for (int i = 0; i < skipped_file_size; i++) {
        checksum += output_index++ * skipped_id;
      }
      free_block_space = input[free_block_index] - '0';
    }
    // There are two cases here:
    //   1. The remainder of the file fits into the current free block without
    //      completely filling it.
    //   2. The current free block ends at the start of the file which we are
    //      already copying.
    // We can handle both cases the same way. In the latter case, the outer loop
    // will end after we finish updating the checksum.
    for (int i = 0; i < block_size; i++) {
      checksum += output_index++ * block_id;
    }
    free_block_space -= block_size;
    copy_index -= 2;
  }
  return checksum;
}

std::int64_t Part2(const std::span<char> input) {
  const int n = input.size();
  // `offsets[i]` is the offset of the entry at `inputs[i]`.
  int offsets[20000];
  {
    int offset = 0;
    for (int i = 0; i < n; i++) {
      offsets[i] = offset;
      offset += input[i] - '0';
    }
  }
  // `free_block_indices[i]` is the index of the leftmost free block with
  // exactly `i` units of space. `free_block_indices[0]` is meaningless.
  int free_block_indices[10];
  for (int size = 0; size < 10; size++) {
    int index = n;
    for (int i = 1; i < n; i += 2) {
      if (input[i] == '0' + size) {
        index = i;
        break;
      }
    }
    free_block_indices[size] = index;
  }
  std::int64_t checksum = 0;
  for (int copy_index = n - 1; copy_index > 0; copy_index -= 2) {
    const int block_id = copy_index / 2;
    const int block_size = input[copy_index] - '0';
    // Find the first block which fits the file.
    int free_block_size = -1;
    int first = n;
    for (int i = block_size; i < 10; i++) {
      if (free_block_indices[i] < first) {
        free_block_size = i;
        first = free_block_indices[i];
      }
      first = std::min(first, free_block_indices[i]);
    }
    if (first > copy_index) {
      // Either no free block will fit the file (first==n) or a block which fits
      // it is further to the right than the file.
      // Block cannot be moved. Calculate its checksum in place.
      for (int i = 0; i < block_size; i++) {
        checksum += (offsets[copy_index] + i) * block_id;
      }
    } else {
      assert(free_block_size >= block_size);
      // Block can be moved.
      for (int i = 0; i < block_size; i++) {
        checksum += offsets[first]++ * block_id;
      }
      input[first] -= block_size;
      assert(input[first] == '0' + (free_block_size - block_size));
      // Remove the free block from its size list (since it changed size).
      free_block_indices[free_block_size] = n;
      for (int i = first; i < n; i += 2) {
        if (input[i] == '0' + free_block_size) {
          free_block_indices[free_block_size] = i;
          break;
        }
      }
      // Insert it into the correct size list.
      const int new_size = free_block_size - block_size;
      free_block_indices[new_size] =
          std::min(free_block_indices[new_size], first);
    }
  }
  return checksum;
}

Task<void> Day09(tcp::Socket& socket) {
  char buffer[20001];
  std::span<char> input = co_await socket.Read(buffer);
  assert(input.size() <= 20000 && input.back() == '\n');
  input = input.subspan(0, input.size() - 1);
  assert(input.size() % 2 == 1);

  const std::int64_t part1 = Part1(input);
  const std::int64_t part2 = Part2(input);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
