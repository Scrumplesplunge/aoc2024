#ifndef AOC2024_SCHEDULE_HPP_
#define AOC2024_SCHEDULE_HPP_

#include <utility>

namespace aoc2024 {

bool SchedulerInit();

struct BackgroundTask {
  void* data;
  void (*func)(void*);
  BackgroundTask* next = nullptr;

  void Schedule();
};

template <typename F>
void Schedule(F&& f) {
  using T = std::decay_t<F>;
  struct State {
    BackgroundTask task = {};
    T func;
  };
  auto* state = new State{.func = std::forward<F>(f)};
  state->task.data = state;
  state->task.func = [](void* data) {
    auto* state = reinterpret_cast<State*>(data);
    T func = std::move(state->func);
    delete state;
    func();
  };
  state->task.Schedule();
}

}  // namespace aoc2024

#endif  // AOC2024_SCHEDULE_HPP_
