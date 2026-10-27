/**
 * @file main.cpp
 * @brief Huvudprogram för klockslagaren.
 *
 * Hårdvara:
 *   - Arduino Uno R4 WiFi
 *   - SG90-servo på pinne 9, matat med 5 V och gemensam jord
 *
 * Programmet är uppbyggt av små moduler som var och en har en update()
 * som anropas från loop(). Ingen modul blockerar, så loop() varvar många
 * gånger per sekund och watchdogen kan matas hela tiden.
 *
 *   TimeKeeper   håller tiden och räknar om till lokal tid
 *   WifiLink     håller kortet anslutet till WiFi
 *   Scheduler    avgör om klockan ska slå just nu
 *   BellStriker  rör servot
 *   Console      kommandon över serieporten
 */
#include <Arduino.h>
#include <WDT.h>

#include "BellStriker.h"
#include "Config.h"
#include "Console.h"
#include "Scheduler.h"
#include "TimeKeeper.h"
#include "WifiLink.h"

#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#error "include/Secrets.h saknas. Kopiera include/Secrets.example.h till include/Secrets.h och fyll i WiFi-uppgifterna."
#endif

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

WifiLink wifi(secrets::kWifiSsid, secrets::kWifiPassword,
              WifiLink::Timing{
                  config::kWifiPollMs,
                  config::kWifiRetryMs,
                  config::kModemTimeoutMs,
              });

Console console(Serial, timeKeeper, striker, scheduler, wifi);

// ---------------------------------------------------------------------------
// Systemstatus
// ---------------------------------------------------------------------------

/// Övergripande läge, visas med lysdioden.
enum class Health : uint8_t {
  Ok,          ///< Allt fungerar. Lysdioden blinkar lugnt.
  TimeNotSet,  ///< Klockan är inte ställd. Lysdioden blinkar snabbt.
};

Health health = Health::TimeNotSet;
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
    health = Health::TimeNotSet;
    return;
  }
  health = Health::Ok;

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
  pinMode(config::kStatusLedPin, OUTPUT);
  Serial.begin(config::kSerialBaud);

  // Servot först, så att armen kommer till viloläget direkt.
  striker.begin();
  scheduler.begin();

  console.begin();
  Serial.println(F("Klockan är inte ställd. Använd 'set'."));

  wifi.begin();
  console.update();

  // Watchdogen startar om kortet om loop() fastnar.
  WDT.begin(config::kWatchdogMs);
}

void loop() {
  WDT.refresh();

  console.update();
  striker.update();
  wifi.update();

  if (millis() - lastClockPollMs >= config::kClockPollMs) {
    lastClockPollMs = millis();
    pollClock();
  }

  updateStatusLed();
}
