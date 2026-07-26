#pragma once
#include "Task.hpp"
#include "watchdog_bits.hpp"

class WatchdogTask : public Task {
public:
  WatchdogTask() : Task("Watchdog", 256, osPriorityRealtime) {}

  WatchdogTask(const WatchdogTask &) = delete;
  WatchdogTask &operator=(const WatchdogTask &) = delete;
  WatchdogTask(WatchdogTask &&) = delete;
  WatchdogTask &operator=(WatchdogTask &&) = delete;

  virtual ~WatchdogTask() = default;

protected:
  void run() override;

private:
  void logMissedTasks(EventBits_t bits);
};
