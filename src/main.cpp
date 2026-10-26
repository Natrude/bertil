/**
 * @file main.cpp
 * @brief Huvudprogram för klockslagaren.
 *
 * Hårdvara:
 *   - Arduino Uno R3
 *   - SG90-servo på pinne 9, matat med 5 V och gemensam jord
 *   - Adafruit DS3231 RTC på I2C (SDA = A4, SCL = A5)
 *
 * Programmet är uppbyggt av små moduler som var och en har en update()
 * som anropas från loop(). Ingen modul blockerar, så loop() varvar många
 * gånger per sekund och watchdogen kan matas hela tiden.
 *
 *   TimeKeeper   läser RTC:n och räknar om till lokal tid
 *   Scheduler    avgör om klockan ska slå just nu
 *   BellStriker  rör servot
 *   Console      kommandon över serieporten
 */
#include <Arduino.h>
#include <avr/wdt.h>

#include "BellStriker.h"
#include "Config.h"
#include "Console.h"
#include "Scheduler.h"
#include "TimeKeeper.h"

namespace {

// ---------------------------------------------------------------------------
// Moduler
// ---------------------------------------------------------------------------

TimeKeeper timeKeeper(config::kUtcOffsetHours, config::kUseEuDst);

BellStriker striker(
    BellStriker::Geometry{
        config::kServoPin,
        config::kServoMinPulseUs,
        config::kServoMaxPulseUs,
        config::kRestAngleDeg,
        config::kStrikeAngleDeg,
    },
    BellStriker::Timing{
        config::kSettleMs,
        config::kStrikeMs,
        config::kReturnMs,
        config::kPauseMs,
    });

Scheduler scheduler(config::kSchedule, config::kScheduleSize);

Console console(Serial, timeKeeper, striker, scheduler);

// ---------------------------------------------------------------------------
// Systemstatus
// ---------------------------------------------------------------------------

/// Övergripande läge, visas med lysdioden.
enum class Health : uint8_t {
  Ok,        ///< Allt fungerar. Lysdioden blinkar lugnt.
  RtcFault,  ///< RTC:n svarar inte. Lysdioden blinkar snabbt.
};

Health health = Health::Ok;
uint8_t consecutiveRtcFailures = 0;
uint32_t lastClockPollMs = 0;

// ---------------------------------------------------------------------------
// Hjälpfunktioner
// ---------------------------------------------------------------------------

/// Blinkar statuslysdioden utan att blockera.
void updateStatusLed() {
  // Lugn blink (1 Hz) när allt är bra, snabb blink (5 Hz) vid fel.
  const uint16_t halfPeriodMs = (health == Health::Ok) ? 500 : 100;
  const bool on = (millis() / halfPeriodMs) % 2 == 0;
  digitalWrite(config::kStatusLedPin, on ? HIGH : LOW);
}

/// Läser klockan, uppdaterar hälsoläget och startar slag enligt schemat.
void pollClock() {
  DateTime now;

  if (!timeKeeper.readLocal(now)) {
    // Enstaka missar kan bero på störningar. Först när många avläsningar
    // i rad misslyckats räknas RTC:n som trasig.
    if (consecutiveRtcFailures < config::kRtcFaultThreshold) {
      ++consecutiveRtcFailures;
    } else if (health != Health::RtcFault) {
      health = Health::RtcFault;
      Serial.println(F("Fel: RTC svarar inte."));
    }
    return;
  }

  consecutiveRtcFailures = 0;
  if (health == Health::RtcFault) {
    health = Health::Ok;
    Serial.println(F("RTC svarar igen."));
  }

  // Medan servot parkerar vid uppstart eller slår manuellt väntar schemat.
  // Klockslaget tas då när servot är ledigt, så länge det är inom fönstret.
  if (striker.isBusy()) {
    return;
  }

  const uint8_t strikes = scheduler.poll(now);
  if (strikes == 0) {
    return;
  }

  striker.request(strikes);
  printDateTime(Serial, now);
  Serial.print(F("  slår "));
  Serial.print(strikes);
  Serial.println(F(" gång(er)."));
}

}  // namespace

// ---------------------------------------------------------------------------
// Arduino-ingångar
// ---------------------------------------------------------------------------

void setup() {
  // Om kortet startats om av watchdogen kan den fortfarande vara aktiv.
  // Stäng av den medan uppstarten pågår.
  wdt_disable();

  pinMode(config::kStatusLedPin, OUTPUT);
  Serial.begin(config::kSerialBaud);

  // Servot först, så att armen kommer till viloläget direkt.
  striker.begin();
  scheduler.begin();

  switch (timeKeeper.begin()) {
    case TimeKeeper::Status::Ok:
      break;

    case TimeKeeper::Status::NotFound:
      health = Health::RtcFault;
      Serial.println(F("Fel: hittar ingen RTC. Kontrollera SDA/SCL och matning."));
      break;

    case TimeKeeper::Status::LostPower:
      // Tiden i RTC:n är ogiltig. Använd tiden då programmet kompilerades
      // som startvärde. Den ligger några sekunder efter, så ställ klockan
      // exakt med kommandot "set" efteråt.
      timeKeeper.setLocal(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println(F("Varning: RTC hade tappat strömmen. Tiden är satt till"));
      Serial.println(F("kompileringstiden. Använd 'set' för att ställa exakt."));
      break;
  }

  console.begin();
  console.update();

  // Watchdogen startar om kortet om loop() fastnar i mer än 2 sekunder.
  wdt_enable(WDTO_2S);
}

void loop() {
  wdt_reset();

  console.update();
  striker.update();

  if (millis() - lastClockPollMs >= config::kClockPollMs) {
    lastClockPollMs = millis();
    pollClock();
  }

  updateStatusLed();
}
