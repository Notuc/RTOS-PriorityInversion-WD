#pragma once
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"

class Task {
public:
  Task(const char *name, uint32_t stackWords, osPriority_t priority);
  virtual ~Task() = default;

  TaskHandle_t handle() const { return handle_; }

protected:
  virtual void run() = 0; // derived class implements this

private:
  TaskHandle_t handle_ = nullptr;

  // FreeRTOS needs a plain C function pointer.
  // This static method IS that function — it casts
  // void* back to Task* and calls run().
  static void trampoline(void *param);
};
