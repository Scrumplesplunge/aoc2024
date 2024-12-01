#ifndef AOC2024_CORO_HPP_
#define AOC2024_CORO_HPP_

#include <cassert>
#include <coroutine>
#include <exception>
#include <functional>
#include <optional>
#include <utility>

namespace aoc2024 {

// A Task is a coroutine type for asynchronous work which can either be awaited
// by another coroutine or can be started with a callback for completion.
template <typename T>
class [[nodiscard]] Task {
 public:
  struct promise_type;

  ~Task() { if (handle_) handle_.destroy(); }

  // Movable.
  Task(Task&& other) noexcept
      : handle_(std::exchange(other.handle_, nullptr)) {}
  Task& operator=(Task&& other) noexcept {
    if (handle_) handle_.destroy();
    handle_ = std::exchange(other.handle_, nullptr);
    return *this;
  }

  // Not copyable.
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  // Awaitable.
  bool await_ready() const;
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiter);
  T await_resume();

  // Synchronously start the task. If the task completes immediately, `done`
  // will be invoked synchronously. Otherwise, `done` will be invoked
  // asynchronously once the task completes. It is the caller's responsibility
  // to ensure that the lifetime of the `Task` object lasts until after `done`
  // has been invoked.
  template <std::invocable<T> F>
  requires (!std::is_same_v<T, void>)
  void Start(F&& done);

  template <std::invocable<> F>
  requires std::is_same_v<T, void>
  void Start(F&& done);

 private:
  explicit Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

  std::coroutine_handle<promise_type> handle_;
};

// An Event is a single-producer, single-consumer awaitable which can be
// resolved by a callback:
//
//     Event event;
//     RunWithCallback([](int x) { event(x); });
//     int x = co_await event;
//
// This type is not currently thread-safe: resolving the event from another
// thread without synchronisation will result in undefined behaviour.
template <typename T>
class [[nodiscard]] Event {
 public:
  bool await_ready() const;
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);
  T await_resume();

  void Trigger(T value);

 private:
  std::coroutine_handle<> awaiter_;
  std::optional<T> value_;
};

struct promise_base {
  enum State : char {
    kNotStarted,  // Has not been started.
    kStarted,     // Has started but has not finished.
    kReady,       // Has finished successfully but has not been consumed.
    kFailed,      // Has finished unsuccessfully but has not been consumed.
    kDone,        // Has finished and has been consumed.
  };

  ~promise_base() {
    assert(state == kNotStarted || state == kDone);
  }

  std::suspend_always initial_suspend() noexcept { return {}; }

  class Invoke {
   public:
    explicit Invoke(std::function<void()> done) : done_(std::move(done)) {}

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) noexcept { done_(); }
    void await_resume() noexcept {}

   private:
    std::function<void()> done_;
  };

  Invoke final_suspend() noexcept { return Invoke(std::move(done)); }

  State state = State::kNotStarted;
  std::function<void()> done;
};

template <typename T>
struct Task<T>::promise_type : promise_base {
  promise_type() {}
  ~promise_type() {}

  Task<T> get_return_object() {
    return Task<T>(std::coroutine_handle<promise_type>::from_promise(*this));
  }

  void return_value(T value) {
    assert(state == kStarted);
    new(&result) T(std::move(value));
    state = kReady;
  }

  void unhandled_exception() {
    assert(state == kStarted);
    new(&error) std::exception_ptr(std::current_exception());
    state = kFailed;
  }

  T consume() {
    assert(state == kReady || state == kFailed);
    const bool failed = state == kFailed;
    state = kDone;
    if (failed) {
      std::exception_ptr p = std::move(error);
      error.~exception_ptr();
      std::rethrow_exception(std::move(p));
    } else {
      struct Cleanup {
        T& value;
        ~Cleanup() { value.~T(); }
      } cleanup{result};
      return result;
    }
  }

  union {
    T result;
    std::exception_ptr error;
  };
};

template <>
struct Task<void>::promise_type : promise_base {
  Task<void> get_return_object() {
    return Task<void>(std::coroutine_handle<promise_type>::from_promise(*this));
  }

  void return_void() {
    assert(state == kStarted);
    state = kReady;
  }

  void unhandled_exception() {
    assert(state == kStarted);
    error = std::current_exception();
    state = kFailed;
  }

  void consume() {
    // For some reason, if I stack up three pending ncats, this assertion fails
    // when the second one starts. A task gets consumed after it has been
    // initiated but has not yet finished (successfully or unsuccessfully).
    assert(state == kReady || state == kFailed);
    const bool failed = state == kFailed;
    state = kDone;
    if (failed) std::rethrow_exception(std::move(error));
  }

  std::exception_ptr error;
};

template <typename T>
bool Task<T>::await_ready() const { return false; }

template <typename T>
std::coroutine_handle<> Task<T>::await_suspend(
    std::coroutine_handle<> awaiter) {
  handle_.promise().done = awaiter;
  handle_.promise().state = promise_type::kStarted;
  return handle_;
}

template <typename T>
T Task<T>::await_resume() { return handle_.promise().consume(); }

template <typename T>
template <std::invocable<T> F>
requires (!std::is_same_v<T, void>)
void Task<T>::Start(F&& done) {
  assert(handle_.promise().state == promise_type::kNotStarted);
  handle_.promise().state = promise_type::kStarted;
  handle_.promise().done = [handle = handle_, done = std::forward<F>(done)] {
    done(handle.promise().consume());
  };
  handle_.resume();
}

template <typename T>
template <std::invocable<> F>
requires std::is_same_v<T, void>
void Task<T>::Start(F&& done) {
  assert(handle_.promise().state == promise_type::kNotStarted);
  handle_.promise().state = promise_type::kStarted;
  handle_.promise().done = [handle = handle_, done = std::forward<F>(done)] {
    handle.promise().consume();
    done();
  };
  handle_.resume();
}

template <typename T>
bool Event<T>::await_ready() const {
  assert(!awaiter_);
  return value_.has_value();
}

template <typename T>
std::coroutine_handle<> Event<T>::await_suspend(
    std::coroutine_handle<> awaiter) {
  assert(!awaiter_);
  if (value_) {
    return awaiter;
  } else {
    awaiter_ = awaiter;
    return std::noop_coroutine();
  }
}

template <typename T>
T Event<T>::await_resume() {
  assert(value_);
  return std::move(*value_);
}

template <typename T>
void Event<T>::Trigger(T value) {
  assert(!value_);
  value_.emplace(std::move(value));
  if (awaiter_) awaiter_.resume();
}

}  // namespace aoc2024

#endif  // AOC2024_CORO_HPP_
