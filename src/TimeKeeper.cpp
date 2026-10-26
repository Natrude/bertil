/**
 * @file TimeKeeper.cpp
 * @brief Implementation av tidshållningen.
 */
#include "TimeKeeper.h"

#include <Wire.h>

namespace {

/// Maxtid för en I2C-överföring. Utan gräns kan Wire hänga för alltid
/// om en sladd glappar, och då startar watchdogen om hela kortet.
constexpr uint32_t kWireTimeoutUs = 25000;

/// Tider utanför detta intervall räknas som felaktiga avläsningar.
constexpr uint16_t kMinValidYear = 2024;
constexpr uint16_t kMaxValidYear = 2099;

}  // namespace

TimeKeeper::TimeKeeper(int8_t utcOffsetHours, bool useEuDst)
    : utcOffsetHours_(utcOffsetHours), useEuDst_(useEuDst) {}

TimeKeeper::Status TimeKeeper::begin() {
  // Timeouten sätts före rtc_.begin() så att även den första överföringen
  // har en gräns. Wire.begin() nollställer den inte.
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(kWireTimeoutUs, true);
#endif

  // rtc_.begin() startar Wire och kontrollerar att kretsen svarar på
  // sin I2C-adress (0x68).
  if (!rtc_.begin()) {
    return Status::NotFound;
  }

  // 32 kHz-utgången används inte. Stäng av den för att spara ström.
  rtc_.disable32K();

  // Flaggan sätts av kretsen om både matning och batteri har saknats.
  // Då står klockan på ett gammalt värde och måste ställas.
  if (rtc_.lostPower()) {
    return Status::LostPower;
  }

  return Status::Ok;
}

bool TimeKeeper::readLocal(DateTime& local) {
#if defined(WIRE_HAS_TIMEOUT)
  Wire.clearWireTimeoutFlag();
#endif

  const DateTime utc = rtc_.now();

#if defined(WIRE_HAS_TIMEOUT)
  if (Wire.getWireTimeoutFlag()) {
    return false;
  }
#endif

  // Om modulen inte svarar läses ofta bara ettor (0xFF), vilket ger
  // ogiltiga datum. Sådana avläsningar kastas.
  if (!utc.isValid() || utc.year() < kMinValidYear ||
      utc.year() > kMaxValidYear) {
    return false;
  }

  local = utcToLocal(utc);
  return true;
}

void TimeKeeper::setLocal(const DateTime& local) {
  // adjust() nollställer också lostPower-flaggan i kretsen.
  rtc_.adjust(localToUtc(local));
}

float TimeKeeper::temperatureC() {
  return rtc_.getTemperature();
}

DateTime TimeKeeper::utcToLocal(const DateTime& utc) const {
  const int8_t hours = utcOffsetHours_ + (isDst(utc) ? 1 : 0);
  return utc + TimeSpan(0, hours, 0, 0);
}

DateTime TimeKeeper::localToUtc(const DateTime& local) const {
  const DateTime standardUtc = local - TimeSpan(0, utcOffsetHours_, 0, 0);

  // Prova först tolkningen "lokal tid är sommartid". Stämmer den inte
  // är det normaltid. Timmen som finns två gånger i oktober tolkas
  // därmed som sommartid.
  const DateTime dstUtc = standardUtc - TimeSpan(0, 1, 0, 0);
  if (isDst(dstUtc)) {
    return dstUtc;
  }
  return standardUtc;
}

bool TimeKeeper::isDst(const DateTime& utc) const {
  if (!useEuDst_) {
    return false;
  }

  // I EU gäller sommartid från sista söndagen i mars kl 01:00 UTC
  // till sista söndagen i oktober kl 01:00 UTC, i alla tidszoner samtidigt.
  const uint16_t year = utc.year();
  const DateTime start(year, 3, lastSundayOf(year, 3), 1, 0, 0);
  const DateTime end(year, 10, lastSundayOf(year, 10), 1, 0, 0);
  return utc >= start && utc < end;
}

uint8_t TimeKeeper::lastSundayOf(uint16_t year, uint8_t month) {
  // Gäller bara månader med 31 dagar, vilket räcker för mars och oktober.
  // dayOfTheWeek() ger 0 för söndag, 1 för måndag och så vidare.
  const DateTime lastDay(year, month, 31);
  return 31 - lastDay.dayOfTheWeek();
}

void printDateTime(Print& out, const DateTime& t) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u", t.year(),
           t.month(), t.day(), t.hour(), t.minute(), t.second());
  out.print(buffer);
}
