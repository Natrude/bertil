/**
 * @file Console.h
 * @brief Enkel textkonsol över serieporten för att ställa och testa apparaten.
 */
#pragma once

#include <Arduino.h>

#include "BellStriker.h"
#include "Scheduler.h"
#include "TimeKeeper.h"
#include "WifiLink.h"

/**
 * @brief Läser kommandon rad för rad från serieporten.
 *
 * Kommandon:
 *   help                          Visa hjälp
 *   time                          Visa aktuell lokal tid
 *   set ÅÅÅÅ-MM-DD hh:mm:ss       Ställ klockan (lokal tid)
 *   strike [n]                    Slå n gånger direkt (standard 1)
 *   schedule                      Visa slagschemat
 *   wifi                          Visa WiFi-läget
 *
 * Läsningen är icke-blockerande: update() tar bara de tecken som redan
 * har kommit in och returnerar direkt.
 */
class Console {
 public:
  Console(Stream& io, TimeKeeper& clock, BellStriker& striker,
          const Scheduler& scheduler, WifiLink& wifi);

  /// Skriver ut välkomsttext. Anropas en gång från setup().
  void begin();

  /// Läser in tecken och kör färdiga rader. Anropas varje varv i loop().
  void update();

 private:
  void execute(char* line);
  void printHelp();
  void printTime();
  void printSchedule();
  void handleSet(const char* line);
  void handleStrike(const char* line);

  /// Längsta rad som tas emot, inklusive avslutande nolla.
  static constexpr uint8_t kLineCapacity = 40;

  Stream& io_;
  TimeKeeper& clock_;
  BellStriker& striker_;
  const Scheduler& scheduler_;
  WifiLink& wifi_;

  char line_[kLineCapacity];
  uint8_t length_ = 0;
  bool overflow_ = false;
};
