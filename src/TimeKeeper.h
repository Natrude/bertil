/**
 * @file TimeKeeper.h
 * @brief Håller reda på tiden med en DS3231 och räknar om till lokal tid.
 */
#pragma once

#include <Arduino.h>
#include <RTClib.h>

/**
 * @brief Omslutning av DS3231-modulen.
 *
 * RTC:n lagrar alltid UTC. Lokal tid (inklusive sommartid) räknas fram
 * vid varje avläsning. Det gör att klockan byter till och från sommartid
 * av sig själv, utan att RTC:n behöver ställas om.
 */
class TimeKeeper {
 public:
  /// Resultat från begin().
  enum class Status : uint8_t {
    Ok,         ///< RTC:n hittades och tiden är giltig.
    NotFound,   ///< Ingen RTC svarar på I2C-bussen.
    LostPower,  ///< RTC:n har tappat strömmen och tiden måste ställas.
  };

  /**
   * @param utcOffsetHours Normaltidens avvikelse från UTC, t.ex. +1 för Sverige.
   * @param useEuDst       true om EU:s regler för sommartid ska användas.
   */
  TimeKeeper(int8_t utcOffsetHours, bool useEuDst);

  /// Startar I2C och kontrollerar RTC:n. Anropas en gång från setup().
  Status begin();

  /**
   * @brief Läser aktuell lokal tid.
   * @param[out] local Lokal tid. Ändras bara om avläsningen lyckas.
   * @return false om RTC:n inte svarar eller ger orimliga värden.
   */
  bool readLocal(DateTime& local);

  /// Ställer klockan. Tiden anges i lokal tid och sparas som UTC i RTC:n.
  void setLocal(const DateTime& local);

  /// Temperaturen i DS3231-kretsen i grader Celsius (bra för felsökning).
  float temperatureC();

 private:
  DateTime utcToLocal(const DateTime& utc) const;
  DateTime localToUtc(const DateTime& local) const;

  /// true om sommartid gäller vid tidpunkten utc enligt EU:s regler.
  bool isDst(const DateTime& utc) const;

  /// Datum för sista söndagen i en månad med 31 dagar.
  static uint8_t lastSundayOf(uint16_t year, uint8_t month);

  RTC_DS3231 rtc_;
  const int8_t utcOffsetHours_;
  const bool useEuDst_;
};

/// Skriver ut en tidpunkt som "ÅÅÅÅ-MM-DD hh:mm:ss".
void printDateTime(Print& out, const DateTime& t);
