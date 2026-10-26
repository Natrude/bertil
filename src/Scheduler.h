/**
 * @file Scheduler.h
 * @brief Avgör när klockan ska slå utifrån ett schema.
 */
#pragma once

#include <Arduino.h>
#include <RTClib.h>

/// En post i slagschemat.
struct StrikeTime {
  uint8_t hour;     ///< 0-23, lokal tid.
  uint8_t minute;   ///< 0-59.
  uint8_t strikes;  ///< Antal slag.
};

/**
 * @brief Jämför aktuell tid mot schemat.
 *
 * poll() anropas regelbundet med lokal tid och returnerar antal slag
 * när ett schemalagt klockslag inträffar. Varje minut hanteras högst
 * en gång, så klockan slår inte flera gånger under samma minut.
 */
class Scheduler {
 public:
  /**
   * @param entries Pekare till schemat. Måste finnas kvar hela programmets
   *                livstid (i praktiken en konstant i Config.h).
   * @param count   Antal poster i schemat.
   */
  Scheduler(const StrikeTime* entries, size_t count);

  /**
   * @brief Kontrollerar om ett klockslag ska slås nu.
   * @param local Aktuell lokal tid.
   * @return Antal slag som ska slås, eller 0.
   */
  uint8_t poll(const DateTime& local);

  size_t size() const { return count_; }
  const StrikeTime& entry(size_t index) const { return entries_[index]; }

 private:
  const StrikeTime* const entries_;
  const size_t count_;

  /// Senast hanterade minut, räknat som minuter sedan 1970.
  uint32_t lastHandledMinute_;
};
