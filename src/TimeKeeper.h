/**
 * @file TimeKeeper.h
 * @brief Håller reda på tiden med kortets egen klocka och räknar om till
 *        lokal tid.
 */
#pragma once

#include <Arduino.h>
#include <RTClib.h>

/**
 * @brief Mjukvaruklocka byggd på millis().
 *
 * Klockan lagrar UTC som en tidpunkt (ankaret) och det värde millis() hade
 * då. Aktuell tid räknas fram från hur lång tid som gått sedan ankaret.
 * Lokal tid (inklusive sommartid) räknas fram vid varje avläsning, så
 * klockan byter till och från sommartid av sig själv.
 *
 * Tiden finns bara i minnet och måste ställas igen efter varje omstart.
 */
class TimeKeeper {
 public:
  /**
   * @param utcOffsetHours Normaltidens avvikelse från UTC, t.ex. +1 för Sverige.
   * @param useEuDst       true om EU:s regler för sommartid ska användas.
   */
  TimeKeeper(int8_t utcOffsetHours, bool useEuDst);

  /**
   * @brief Läser aktuell lokal tid.
   * @param[out] local Lokal tid. Ändras bara om klockan är ställd.
   * @return false om klockan inte har ställts sedan start.
   */
  bool readLocal(DateTime& local);

  /// Ställer klockan. Tiden anges i lokal tid och sparas som UTC.
  void setLocal(const DateTime& local);

 private:
  /// Aktuell UTC-tid. Får bara anropas när klockan är ställd.
  DateTime nowUtc();

  DateTime utcToLocal(const DateTime& utc) const;
  DateTime localToUtc(const DateTime& local) const;

  /// true om sommartid gäller vid tidpunkten utc enligt EU:s regler.
  bool isDst(const DateTime& utc) const;

  /// Datum för sista söndagen i en månad med 31 dagar.
  static uint8_t lastSundayOf(uint16_t year, uint8_t month);

  const int8_t utcOffsetHours_;
  const bool useEuDst_;

  bool isSet_ = false;
  uint32_t anchorUnix_ = 0;  ///< UTC vid ankaret, sekunder sedan 1970.
  uint32_t anchorMs_ = 0;    ///< millis() vid ankaret.
};

/// Skriver ut en tidpunkt som "ÅÅÅÅ-MM-DD hh:mm:ss".
void printDateTime(Print& out, const DateTime& t);
