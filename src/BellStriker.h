/**
 * @file BellStriker.h
 * @brief Styr servot som slår på klockan.
 */
#pragma once

#include <Arduino.h>
#include <Servo.h>

/**
 * @brief Icke-blockerande styrning av slagservot.
 *
 * request() startar en slagsekvens och update() driver den framåt.
 * update() måste anropas ofta från loop(). Eftersom inget delay() används
 * kan resten av programmet (seriekonsol, klocka, watchdog) köra under tiden.
 *
 * Ett slag går igenom faserna:
 *
 *   Idle -> Settling -> Striking -> Returning -> (Pausing -> Striking ...) -> Idle
 *
 * När sekvensen är klar kopplas servot loss (detach). Då skickas inga
 * pulser och servot står still utan att surra eller dra ström.
 */
class BellStriker {
 public:
  /// Pinne, pulsbredder och vinklar för servot.
  struct Geometry {
    uint8_t pin;
    uint16_t minPulseUs;
    uint16_t maxPulseUs;
    uint8_t restAngleDeg;
    uint8_t strikeAngleDeg;
  };

  /// Längd på varje fas i en slagsekvens, i millisekunder.
  struct Timing {
    uint16_t settleMs;
    uint16_t strikeMs;
    uint16_t returnMs;
    uint16_t pauseMs;
  };

  /// Övre gräns för antal slag i en sekvens, som skydd mot felaktiga värden.
  static constexpr uint8_t kMaxStrikes = 12;

  BellStriker(const Geometry& geometry, const Timing& timing);

  /// Ställer servot i viloläget vid uppstart. Anropas en gång från setup().
  void begin();

  /**
   * @brief Begär att klockan ska slås.
   * @param strikes Antal slag (1 till kMaxStrikes, större värden begränsas).
   * @return false om en sekvens redan pågår eller om strikes är 0.
   */
  bool request(uint8_t strikes);

  /// Driver tillståndsmaskinen. Anropas varje varv i loop().
  void update();

  /// true medan en sekvens pågår.
  bool isBusy() const { return state_ != State::Idle; }

 private:
  enum class State : uint8_t {
    Idle,       ///< Servot är urkopplat och vilar.
    Parking,    ///< Uppstart: servot åker till viloläget.
    Settling,   ///< Servot har kopplats in och låser i viloläget.
    Striking,   ///< Servot rör sig mot slagläget.
    Returning,  ///< Servot rör sig tillbaka till viloläget.
    Pausing,    ///< Paus mellan två slag.
  };

  /// Byter tillstånd och startar fasens timer.
  void enter(State next, uint16_t durationMs);

  /// Kopplar in servot i viloläget och startar en fas.
  void engage(State next, uint16_t durationMs);

  /// Kopplar loss servot och går till Idle.
  void release();

  /// Sätter servots position.
  void moveTo(uint8_t angleDeg);

  /// Räknar om en vinkel till pulsbredd enligt geometry_.
  uint16_t angleToPulseUs(uint8_t angleDeg) const;

  Servo servo_;
  const Geometry geometry_;
  const Timing timing_;

  State state_ = State::Idle;
  uint32_t stateStartMs_ = 0;
  uint16_t stateDurationMs_ = 0;
  uint8_t remainingStrikes_ = 0;
};
