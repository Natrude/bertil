/**
 * @file Display.h
 * @brief Visar information på en LCD1602 med I2C-adapter.
 */
#pragma once

#include <Arduino.h>
#include <RTClib.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "Scheduler.h"

/**
 * @brief Skärmen visar tid kvar till nästa klockslag.
 *
 * update() bygger skärmens två rader och skriver bara de rader som har
 * ändrats, så att I2C-bussen inte belastas i onödan. Saknas skärmen
 * fortsätter resten av programmet som vanligt.
 */
class Display {
 public:
  static constexpr uint8_t kCols = 16;
  static constexpr uint8_t kRows = 2;

  explicit Display(const Scheduler& scheduler);

  /// Letar upp skärmen och laddar egna tecken. Anropas en gång från setup().
  void begin();

  /**
   * @brief Uppdaterar skärmen.
   * @param local    Aktuell lokal tid, eller nullptr om klockan inte är ställd.
   * @param striking true medan servot slår.
   */
  void update(const DateTime* local, bool striking);

 private:
  /// Visar två rader. Texten är UTF-8 och fylls ut med mellanslag.
  void show(const char* row0, const char* row1);

  /// Skriver en rad om den skiljer sig från det som redan visas.
  void writeRow(uint8_t row, const char* utf8);

  const Scheduler& scheduler_;
  hd44780_I2Cexp lcd_;
  bool present_ = false;

  /// Det som visas just nu, i skärmens teckenkoder.
  char shown_[kRows][kCols + 1] = {};
};
