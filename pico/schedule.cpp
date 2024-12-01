#include "schedule.hpp"

#include <pico/cyw43_arch.h>

namespace aoc2024 {
namespace {

// Set by Init(), after which point it is unchanged.
async_when_pending_worker worker;

BackgroundTask* head;
BackgroundTask* tail;

void Run() {
  while (head) {
    BackgroundTask* task = head;
    head = head->next;
    if (!head) tail = nullptr;
    task->func(task->data);
  }
}

}  // namespace

bool SchedulerInit() {
  assert(!worker.do_work);
  worker.do_work = [](async_context*, async_when_pending_worker*) { Run(); };
  async_context_add_when_pending_worker(cyw43_arch_async_context(), &worker);
  return true;
}

void BackgroundTask::Schedule() {
  if (tail) {
    tail->next = this;
  } else {
    head = tail = this;
    async_context_set_work_pending(cyw43_arch_async_context(), &worker);
  }
}

}  // namespace aoc2024
