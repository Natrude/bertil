/**
 * @file Scheduler.cpp
 * @brief Implementation av slagschemat.
 */
#include "Scheduler.h"

namespace {

/// Ett klockslag räknas bara under de första sekunderna av minuten.
/// Om kortet startas om mitt i en schemalagd minut blir det då inget
/// extra slag. Fönstret är brett nog för att tåla några missade avläsningar.
constexpr uint8_t kTriggerWindowSec = 30;

/// Värde som aldrig kan vara en riktig minut. Används som "ingen ännu".
constexpr uint32_t kNoMinute = 0xFFFFFFFFUL;

}  // namespace

Scheduler::Scheduler(const StrikeTime* entries, size_t count)
    : entries_(entries), count_(count), lastHandledMinute_(kNoMinute) {}

uint8_t Scheduler::poll(const DateTime& local) {
  if (local.second() >= kTriggerWindowSec) {
    return 0;
  }

  // unixtime() används här bara som en unik nyckel per minut.
  const uint32_t minuteKey = local.unixtime() / 60UL;
  if (minuteKey == lastHandledMinute_) {
    return 0;
  }
  lastHandledMinute_ = minuteKey;

  for (size_t i = 0; i < count_; ++i) {
    if (entries_[i].hour == local.hour() &&
        entries_[i].minute == local.minute()) {
      return entries_[i].strikes;
    }
  }
  return 0;
}
