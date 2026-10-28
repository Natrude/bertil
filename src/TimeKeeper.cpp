/**
 * @file TimeKeeper.cpp
 * @brief Implementation av tidshållningen.
 */
#include "TimeKeeper.h"

namespace {

/// Hur länge ankaret får ligga innan det flyttas fram. Måste vara mycket
/// kortare än de cirka 49 dagar det tar innan millis() slår runt.
constexpr uint32_t kReanchorMs = 24UL * 60UL * 60UL * 1000UL;

}  // namespace

TimeKeeper::TimeKeeper(int8_t utcOffsetHours, bool useEuDst)
    : utcOffsetHours_(utcOffsetHours), useEuDst_(useEuDst) {}

bool TimeKeeper::readLocal(DateTime& local) {
  if (!isSet_) {
    return false;
  }
  local = utcToLocal(nowUtc());
  return true;
}

void TimeKeeper::setLocal(const DateTime& local) {
  setUtc(localToUtc(local).unixtime(), 0);
}

int32_t TimeKeeper::setUtc(uint32_t unixSeconds, uint16_t millisPart) {
  const uint32_t nowMs = millis();

  int32_t correctionMs = 0;
  if (isSet_) {
    const int64_t oldMs =
        static_cast<int64_t>(anchorUnix_) * 1000 + (nowMs - anchorMs_);
    const int64_t newMs = static_cast<int64_t>(unixSeconds) * 1000 + millisPart;
    // Begränsa så att stora hopp (t.ex. fel år vid manuell ställning)
    // inte slår runt.
    const int64_t diffMs = newMs - oldMs;
    correctionMs = static_cast<int32_t>(
        diffMs > INT32_MAX ? INT32_MAX : (diffMs < INT32_MIN ? INT32_MIN : diffMs));
  }

  // Ankaret är det värde millis() hade när sekunden unixSeconds började.
  anchorUnix_ = unixSeconds;
  anchorMs_ = nowMs - millisPart;
  isSet_ = true;
  return correctionMs;
}

DateTime TimeKeeper::nowUtc() {
  // Subtraktionen av osignerade tal ger rätt svar även när millis() har
  // slagit runt, så länge mindre än 49 dagar gått sedan ankaret.
  uint32_t elapsedMs = millis() - anchorMs_;

  // Flytta fram ankaret med hela sekunder då och då, så att avståndet
  // till millis() aldrig blir så stort att det slår runt.
  if (elapsedMs >= kReanchorMs) {
    const uint32_t wholeSeconds = elapsedMs / 1000UL;
    anchorUnix_ += wholeSeconds;
    anchorMs_ += wholeSeconds * 1000UL;
    elapsedMs -= wholeSeconds * 1000UL;
  }

  return DateTime(anchorUnix_ + elapsedMs / 1000UL);
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
