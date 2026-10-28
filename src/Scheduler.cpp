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

Scheduler::Scheduler(const StrikeTime* entries, size_t count)
    : entries_(entries), count_(count), lastHandledMinute_(kNoMinute) {}

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

  bool found = false;
  for (size_t i = 0; i < count_; ++i) {
    const uint32_t entrySec =
        entries_[i].hour * 3600UL + entries_[i].minute * 60UL;
    // Poster som redan passerat i dag räknas till samma tid i morgon.
    // Dagar med sommartidsbyte är en timme kortare eller längre, vilket
    // bara påverkar nedräkningen under natten till bytet.
    const uint32_t until = (entrySec + kSecondsPerDay - nowSec) % kSecondsPerDay;
    if (!found || until < secondsUntil) {
      entry = entries_[i];
      secondsUntil = until;
      found = true;
    }
  }
  return found;
}
