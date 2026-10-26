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

/// Hur ofta RTC:n läses av, i millisekunder.
constexpr uint16_t kClockPollMs = 200;

/// Antal misslyckade avläsningar i rad innan RTC:n räknas som trasig.
constexpr uint8_t kRtcFaultThreshold = 10;

// ---------------------------------------------------------------------------
// Slagschema (lokal tid)
// ---------------------------------------------------------------------------

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

}  // namespace config
