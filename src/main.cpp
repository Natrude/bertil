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
 *   NtpSync      ställer klockan från en NTP-server
 *   Display      visar tid till nästa slag och citat på en LCD1602
 *   Scheduler    avgör om klockan ska slå just nu
 *   BellStriker  rör servot
 *   Console      kommandon över serieporten
 */
#include <Arduino.h>
#include <WDT.h>

#include "BellStriker.h"
#include "Config.h"
#include "Console.h"
#include "Display.h"
#include "NtpSync.h"
#include "Scheduler.h"
#include "TimeKeeper.h"
#include "WifiLink.h"
#include "quotes.h"

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

NtpSync ntp(config::kNtpServer, timeKeeper, wifi,
            NtpSync::Timing{
                config::kNtpSyncIntervalMs,
                config::kNtpRetryMs,
                config::kNtpReplyTimeoutMs,
                config::kNtpStaleMs,
            });

Display display(scheduler, kQuotes, kQuoteCount,
                Display::Timing{
                    config::kCountdownSeconds,
                    config::kQuotePartSeconds,
                });

Console console(Serial, timeKeeper, striker, scheduler, wifi, ntp);

// ---------------------------------------------------------------------------
// Systemstatus
// ---------------------------------------------------------------------------

/// Övergripande läge, visas med lysdioden.
enum class Health : uint8_t {
  Ok,          ///< Allt fungerar. Lysdioden blinkar lugnt.
  TimeStale,   ///< Tiden går, men har inte synkats på länge.
               ///< Lysdioden dubbelblinkar.
  TimeNotSet,  ///< Klockan är inte ställd. Lysdioden blinkar snabbt.
};

Health health = Health::TimeNotSet;
uint32_t lastClockPollMs = 0;

// ---------------------------------------------------------------------------
// Hjälpfunktioner
// ---------------------------------------------------------------------------

/// Blinkar statuslysdioden utan att blockera.
void updateStatusLed() {
  const uint32_t now = millis();
  bool on = false;

  switch (health) {
    case Health::Ok:
      // Lugn blink, 1 Hz.
      on = (now / 500) % 2 == 0;
      break;

    case Health::TimeStale: {
      // Två korta blinkar per sekund.
      const uint32_t phase = now % 1000;
      on = phase < 100 || (phase >= 200 && phase < 300);
      break;
    }

    case Health::TimeNotSet:
      // Snabb blink, 5 Hz.
      on = (now / 100) % 2 == 0;
      break;
  }

  digitalWrite(config::kStatusLedPin, on ? HIGH : LOW);
}

/// Visar aktuellt läge på skärmen.
void updateDisplay() {
  DateTime now;
  const bool hasTime = timeKeeper.readLocal(now);
  display.update(hasTime ? &now : nullptr, striker.isBusy());
}

/// Läser klockan, uppdaterar hälsoläget och startar slag enligt schemat.
void pollClock() {
  DateTime now;

  if (!timeKeeper.readLocal(now)) {
    health = Health::TimeNotSet;
    return;
  }
  health = ntp.isFresh() ? Health::Ok : Health::TimeStale;

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
  Serial.println(F("Väntar på tid från NTP."));

  display.begin();
  display.update(nullptr, false);
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
  ntp.update();

  if (millis() - lastClockPollMs >= config::kClockPollMs) {
    lastClockPollMs = millis();
    pollClock();
    updateDisplay();
  }

  updateStatusLed();
}
