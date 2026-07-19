#include "Task.hpp"

Task::Task(const char *name, uint32_t stackWords, osPriority_t priority) {
  xTaskCreate(trampoline, // static C-compatible function
              name, stackWords,
              this, // passed as void* — trampoline casts it back
              priority, &handle_);
  configASSERT(handle_);
}

void Task::trampoline(void *param) {
  // Cast the void* back to the Task that created this thread
  static_cast<Task *>(param)->run();
  // run() should never return on bare metal,
  // but if it does, delete the task cleanly
  vTaskDelete(nullptr);
}
