/**
 * @file Display.h
 * @brief Visar information på en LCD1602 med I2C-adapter.
 */
#pragma once

#include <Arduino.h>
#include <RTClib.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "LcdText.h"
#include "Scheduler.h"

/// Ett tvådelat citat. Texten är UTF-8.
struct Quote {
  const char* first;
  const char* second;
};

/**
 * @brief Skärmen visar tid kvar till nästa klockslag och citat.
 *
 * Skärmen går i en cykel som börjar om varje hel minut: först nedräkningen
 * till nästa klockslag, sedan citatets två delar växelvis. Varje minut
 * visas nästa citat i listan. Är en del för lång för skärmen bläddras den
 * en rad i taget under delens tid.
 *
 * update() skriver bara de rader som har ändrats, så att I2C-bussen inte
 * belastas i onödan. Saknas skärmen fortsätter resten av programmet som
 * vanligt.
 */
class Display {
 public:
  /// Cykelns längder i sekunder. countdownSec + 4 * quotePartSec = 60.
  struct Timing {
    uint8_t countdownSec;  ///< Nedräkningen visas först i varje minut.
    uint8_t quotePartSec;  ///< Varje citatdel visas så här länge, två gånger.
  };

  Display(const Scheduler& scheduler, const Quote* quotes, size_t quoteCount,
          const Timing& timing);

  /// Letar upp skärmen och laddar egna tecken. Anropas en gång från setup().
  void begin();

  /**
   * @brief Uppdaterar skärmen.
   * @param local    Aktuell lokal tid, eller nullptr om klockan inte är ställd.
   *                 Utan tid går cykeln på kortets egen räknare.
   * @param striking true medan servot slår.
   */
  void update(const DateTime* local, bool striking);

 private:
  /// Nedräkning till nästa klockslag, eller väntetext utan tid.
  void showCountdown(const DateTime* local);

  /// En del av ett citat. secondInPart väljer rad om delen är lång.
  void showQuotePart(const char* utf8, uint8_t secondInPart);

  /// Visar två rader UTF-8-text.
  void showText(const char* row0, const char* row1);

  /// Skriver en rad i skärmens teckenkoder om den skiljer sig från det som
  /// redan visas. Kortare text fylls ut med mellanslag.
  void writeRow(uint8_t row, const char* lcdText);

  /// Rader som en citatdel högst får. Längre text tappas.
  static constexpr uint8_t kMaxQuoteLines = 4;

  const Scheduler& scheduler_;
  const Quote* const quotes_;
  const size_t quoteCount_;
  const Timing timing_;

  hd44780_I2Cexp lcd_;
  bool present_ = false;

  /// Det som visas just nu, i skärmens teckenkoder.
  char shown_[lcdtext::kRows][lcdtext::kCols + 1] = {};
};
