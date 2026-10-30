/**
 * @file Config.h
 * @brief Samlad konfiguration för klockslagaren.
 *
 * Allt som kan behöva justeras när apparaten byggs ihop finns här:
 * pinnar, servovinklar, tider och slagschemat. Resten av koden ska
 * inte behöva ändras för att trimma in apparaten.
 */
#pragma once

#include <Arduino.h>

#include "Scheduler.h"

namespace config {

// ---------------------------------------------------------------------------
// Hårdvara
// ---------------------------------------------------------------------------

/// Signalpinne till servot (orange sladd på SG90).
constexpr uint8_t kServoPin = 9;

/// Skärmens pinnar. Skärmen körs i 4-bitarsläge, så bara D4-D7 kopplas.
/// R/W kopplas till GND.
constexpr uint8_t kLcdRsPin = 12;
constexpr uint8_t kLcdEnablePin = 11;
constexpr uint8_t kLcdD4Pin = 5;
constexpr uint8_t kLcdD5Pin = 4;
constexpr uint8_t kLcdD6Pin = 3;
constexpr uint8_t kLcdD7Pin = 2;

/// Inbyggd lysdiod som används som statusindikator.
constexpr uint8_t kStatusLedPin = LED_BUILTIN;

/// Servots pulsbredd vid 0 grader, i mikrosekunder.
constexpr uint16_t kServoMinPulseUs = 500;

/// Servots pulsbredd vid 180 grader, i mikrosekunder.
constexpr uint16_t kServoMaxPulseUs = 2400;

// ---------------------------------------------------------------------------
// Slagrörelse
// ---------------------------------------------------------------------------

/// Vinkel där armen vilar, en bit från klockan.
constexpr uint8_t kRestAngleDeg = 20;

/// Vinkel där servot stannar strax före klockan.
/// Pianotråden fjädrar den sista biten fram och studsar tillbaka,
/// så att klockan får klinga fritt. Justera i små steg.
constexpr uint8_t kStrikeAngleDeg = 60;

/// Väntetid efter att servot kopplats in, så att det hinner låsa i viloläget.
constexpr uint16_t kSettleMs = 60;

/// Tid som servot får på sig att nå slagläget.
constexpr uint16_t kStrikeMs = 120;

/// Tid som servot får på sig att komma tillbaka till viloläget.
constexpr uint16_t kReturnMs = 250;

/// Paus mellan slagen när flera slag ska slås i rad.
constexpr uint16_t kPauseMs = 700;

// ---------------------------------------------------------------------------
// Tid
// ---------------------------------------------------------------------------

/// Normaltidens avvikelse från UTC i timmar. Sverige: +1 (CET).
constexpr int8_t kUtcOffsetHours = 1;

/// true = följ EU:s regler för sommartid (sista söndagen i mars och oktober).
constexpr bool kUseEuDst = true;

/// Hur ofta klockan jämförs med schemat, i millisekunder.
constexpr uint16_t kClockPollMs = 200;

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------

/// Hur ofta WiFi-läget läses av, i millisekunder.
constexpr uint16_t kWifiPollMs = 1000;

/// Väntetid mellan anslutningsförsök när kortet inte är anslutet.
constexpr uint32_t kWifiRetryMs = 30000;

/// Längsta väntan på svar från WiFi-modemet. Måste vara kortare än
/// kWatchdogMs.
constexpr uint16_t kModemTimeoutMs = 3000;

// ---------------------------------------------------------------------------
// NTP
// ---------------------------------------------------------------------------

/// Server som klockan ställs från.
constexpr char kNtpServer[] = "pool.ntp.org";

/// Tid mellan synkar. Kortets klocka saknar kristall och kan gå flera
/// sekunder fel per timme, så synken görs ofta.
constexpr uint32_t kNtpSyncIntervalMs = 10UL * 60UL * 1000UL;

/// Väntetid innan nytt försök när en synk misslyckats.
constexpr uint32_t kNtpRetryMs = 30000;

/// Hur länge ett svar från servern väntas på.
constexpr uint16_t kNtpReplyTimeoutMs = 2000;

/// Efter så lång tid utan lyckad synk räknas tiden som osäker
/// (lysdioden dubbelblinkar).
constexpr uint32_t kNtpStaleMs = 60UL * 60UL * 1000UL;

// ---------------------------------------------------------------------------
// Skärm
// ---------------------------------------------------------------------------

/// Sekunder i början av varje minut då tid till nästa slag visas.
constexpr uint8_t kCountdownSeconds = 20;

/// Sekunder per citatdel. Delarna visas växelvis två gånger var under
/// resten av minuten. Citaten står i include/quotes.h.
constexpr uint8_t kQuotePartSeconds = 10;

static_assert(kCountdownSeconds + 4 * kQuotePartSeconds == 60,
              "Skärmens cykel måste vara exakt en minut.");

// ---------------------------------------------------------------------------
// Slagschema (lokal tid)
// ---------------------------------------------------------------------------

/// Dagar då klockan slår, indexerat som DateTime::dayOfTheWeek().
constexpr bool kStrikeDays[7] = {
    false,  // sön
    true,   // mån
    true,   // tis
    true,   // ons
    true,   // tor
    true,   // fre
    false,  // lör
};

/// Klockslag då klockan ska slå. Format: { timme, minut, antal slag }.
constexpr StrikeTime kSchedule[] = {
    {9, 30, 1},
    {12, 0, 3},
    {15, 45, 1},
};

constexpr size_t kScheduleSize = sizeof(kSchedule) / sizeof(kSchedule[0]);

// ---------------------------------------------------------------------------
// Övrigt
// ---------------------------------------------------------------------------

/// Hastighet på serieporten. Ska stämma med monitor_speed i platformio.ini.
constexpr uint32_t kSerialBaud = 115200;

/// Watchdogen startar om kortet om loop() fastnar längre än så här.
/// Högst 5592 ms på Uno R4.
constexpr uint32_t kWatchdogMs = 5000;

static_assert(kModemTimeoutMs < kWatchdogMs,
              "Modemets tidsgräns måste vara kortare än watchdogen.");

}  // namespace config
