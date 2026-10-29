/**
 * @file Scheduler.cpp
 * @brief Implementation av slagschemat.
 */
#include "Scheduler.h"

#include <EEPROM.h>

namespace {

/// Ett klockslag räknas bara under de första sekunderna av minuten, så att
/// det inte slår långt efter utsatt tid. Fönstret är brett nog för att tåla
/// några missade avläsningar och ett manuellt slag som pågår.
constexpr uint8_t kTriggerWindowSec = 30;

/// Adress i EEPROM där senast slagna minut sparas. Skrivs bara när klockan
/// slår enligt schemat, så slitaget blir försumbart.
constexpr int kLastStrikeEepromAddr = 0;

/// Värde som aldrig kan vara en riktig minut. Används som "ingen ännu".
constexpr uint32_t kNoMinute = 0xFFFFFFFFUL;

}  // namespace

Scheduler::Scheduler(const StrikeTime* entries, size_t count,
                     const bool* days)
    : entries_(entries),
      count_(count),
      days_(days),
      lastHandledMinute_(kNoMinute) {}

void Scheduler::begin() {
  // Om kortet startas om under en minut då klockan redan har slagit
  // (t.ex. när seriemonitorn öppnas) blir det inget extra slag.
  // Ett oskrivet EEPROM innehåller 0xFF i alla byte, vilket är kNoMinute.
  EEPROM.get(kLastStrikeEepromAddr, lastHandledMinute_);
}

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

  if (!days_[local.dayOfTheWeek()]) {
    return 0;
  }

  for (size_t i = 0; i < count_; ++i) {
    if (entries_[i].hour == local.hour() &&
        entries_[i].minute == local.minute()) {
      EEPROM.put(kLastStrikeEepromAddr, minuteKey);
      return entries_[i].strikes;
    }
  }
  return 0;
}

bool Scheduler::next(const DateTime& local, StrikeTime& entry,
                     uint32_t& secondsUntil) const {
  constexpr uint32_t kSecondsPerDay = 24UL * 60UL * 60UL;
  const uint32_t nowSec = local.hour() * 3600UL + local.minute() * 60UL +
                          local.second();

  // Dagarna gås igenom framåt och första dagen med ett kommande slag vinner.
  // Dag 7 är samma veckodag som i dag och behövs när bara den dagen är
  // markerad och dagens slag redan passerat. Dagar med sommartidsbyte är en
  // timme kortare eller längre, vilket bara påverkar nedräkningen över bytet.
  for (uint8_t day = 0; day <= 7; ++day) {
    if (!days_[(local.dayOfTheWeek() + day) % 7]) {
      continue;
    }
    bool found = false;
    for (size_t i = 0; i < count_; ++i) {
      const uint32_t entrySec =
          entries_[i].hour * 3600UL + entries_[i].minute * 60UL;
      if (day == 0 && entrySec < nowSec) {
        continue;
      }
      const uint32_t until = day * kSecondsPerDay + entrySec - nowSec;
      if (!found || until < secondsUntil) {
        entry = entries_[i];
        secondsUntil = until;
        found = true;
      }
    }
    if (found) {
      return true;
    }
  }
  return false;
}
