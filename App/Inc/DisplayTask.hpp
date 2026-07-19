#pragma once

#include "SensorReading.hpp" //
#include "Task.hpp"          // Base class

class DisplayTask : public Task {
public:
  DisplayTask() : Task("Display", 384, osPriorityBelowNormal) {}

  DisplayTask(const char *name, uint32_t stackWords, osPriority_t priority)
      : Task(name, stackWords, priority) {}

  // Clean up with a defaulted virtual destructor
  virtual ~DisplayTask() = default;

  // Explicitly delete copy and move operations beacause of freertos
  DisplayTask(const DisplayTask &) = delete;
  DisplayTask &operator=(const DisplayTask &) = delete;
  DisplayTask(DisplayTask &&) = delete;
  DisplayTask &operator=(DisplayTask &&) = delete;

protected:
  // Implement the pure virtual method from the base class
  void run() override;

private:
  // Helper function to update the screen
  void renderOLED(const SensorReading_t &reading);
};
