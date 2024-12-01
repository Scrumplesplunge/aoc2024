// A trivial utility for defining deleters with unique types.
//
// You can use a custom deleter with `unique_ptr` to support different forms of
// cleanup, such as a `malloc_ptr` which deletes with `free`. If you do this by
// the path of least resistance, you end up with:
//
//     // Type does not say which deleter will be used.
//     std::unique_ptr<char[], void (*)(void*)> data(
//         (char*)std::malloc(42),
//         &std::free);  // Every construction site must re-specify it.
//     // The `unique_ptr` is now bigger since it has to store the deleter too.
//     static_assert(sizeof(data) == sizeof(char*) + sizeof(void(*)(void*)));
//
// Using `DeleteWith`, the deleter becomes part of the type instead:
//
//     // Unique type per deleter.
//     template <typename T>
//     using malloc_ptr =
//         std::unique_ptr<T, DeleteWith<[](void* p) { std::free(p); }>>;
//     // The `unique_ptr` has no size overhead versus a `T*`.
//     static_assert(sizeof(malloc_ptr<char*>) == sizeof(char*));

#ifndef AOC2024_DELETE_WITH_HPP_
#define AOC2024_DELETE_WITH_HPP_

namespace aoc2024 {

template <auto F>
struct DeleteWith {
  void operator()(auto* p) { F(p); }
};

}  // namespace aoc2024

#endif  // AOC2024_DELETE_WITH_HPP_
