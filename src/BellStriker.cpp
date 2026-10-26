/**
 * @file BellStriker.cpp
 * @brief Implementation av slagservots tillståndsmaskin.
 */
#include "BellStriker.h"

namespace {

/// Tid som servot får på sig att åka till viloläget vid uppstart.
constexpr uint16_t kParkMs = 500;

}  // namespace

BellStriker::BellStriker(const Geometry& geometry, const Timing& timing)
    : geometry_(geometry), timing_(timing) {}

void BellStriker::begin() {
  // Servot kan stå var som helst när strömmen slås på.
  // Kör det till viloläget och koppla sedan loss det.
  engage(State::Parking, kParkMs);
}

bool BellStriker::request(uint8_t strikes) {
  if (isBusy() || strikes == 0) {
    return false;
  }

  remainingStrikes_ = min(strikes, kMaxStrikes);
  engage(State::Settling, timing_.settleMs);
  return true;
}

void BellStriker::update() {
  if (state_ == State::Idle) {
    return;
  }

  // Vänta tills den aktuella fasen är klar. Subtraktionen av osignerade
  // tal ger rätt svar även när millis() slår runt efter cirka 49 dagar.
  if (millis() - stateStartMs_ < stateDurationMs_) {
    return;
  }

  switch (state_) {
    case State::Parking:
      release();
      break;

    case State::Settling:
    case State::Pausing:
      // Slå. Servot stannar strax före klockan och tråden fjädrar fram.
      moveTo(geometry_.strikeAngleDeg);
      enter(State::Striking, timing_.strikeMs);
      break;

    case State::Striking:
      moveTo(geometry_.restAngleDeg);
      enter(State::Returning, timing_.returnMs);
      break;

    case State::Returning:
      --remainingStrikes_;
      if (remainingStrikes_ > 0) {
        enter(State::Pausing, timing_.pauseMs);
      } else {
        release();
      }
      break;

    case State::Idle:
      break;
  }
}

void BellStriker::enter(State next, uint16_t durationMs) {
  state_ = next;
  stateStartMs_ = millis();
  stateDurationMs_ = durationMs;
}

void BellStriker::engage(State next, uint16_t durationMs) {
  // Positionen sätts före attach() så att första pulsen redan pekar på
  // viloläget. Annars börjar Servo-biblioteket på 90 grader och armen
  // rycker till. Positionen sätts igen efter attach() eftersom biblioteket
  // först då känner till våra min- och maxgränser för pulsbredden.
  moveTo(geometry_.restAngleDeg);
  servo_.attach(geometry_.pin, geometry_.minPulseUs, geometry_.maxPulseUs);
  moveTo(geometry_.restAngleDeg);
  enter(next, durationMs);
}

void BellStriker::release() {
  servo_.detach();
  remainingStrikes_ = 0;
  state_ = State::Idle;
}

void BellStriker::moveTo(uint8_t angleDeg) {
  servo_.writeMicroseconds(angleToPulseUs(angleDeg));
}

uint16_t BellStriker::angleToPulseUs(uint8_t angleDeg) const {
  // Egen omräkning i stället för Servo::write(), så att vinkeln alltid
  // räknas om med de pulsbredder som står i konfigurationen.
  const uint8_t clamped = min(angleDeg, static_cast<uint8_t>(180));
  return map(clamped, 0, 180, geometry_.minPulseUs, geometry_.maxPulseUs);
}
