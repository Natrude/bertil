/**
 * @file NtpSync.h
 * @brief Ställer klockan från en NTP-server.
 */
#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>

#include "TimeKeeper.h"
#include "WifiLink.h"

/**
 * @brief Icke-blockerande SNTP-klient.
 *
 * update() skickar en förfrågan när det är dags och tittar sedan efter
 * svaret vid senare anrop, utan att vänta. Svaret ställer TimeKeeper.
 * Avvikelsen mot den egna klockan loggas vid varje synk, eftersom kortet
 * saknar kristall för klockan och går märkbart fel mellan synkarna.
 */
class NtpSync {
 public:
  /// Tider för synken, i millisekunder.
  struct Timing {
    uint32_t intervalMs;      ///< Tid mellan lyckade synkar.
    uint32_t retryMs;         ///< Väntetid efter en misslyckad synk.
    uint16_t replyTimeoutMs;  ///< Hur länge ett svar väntas på.
    uint32_t staleMs;         ///< Efter så lång tid utan synk räknas tiden som osäker.
  };

  NtpSync(const char* server, TimeKeeper& clock, const WifiLink& wifi,
          const Timing& timing);

  /// Skickar förfrågningar och tar emot svar. Anropas varje varv i loop().
  void update();

  /// Begär en synk så snart som möjligt.
  void requestSync() { syncRequested_ = true; }

  /// true om klockan har synkats inom staleMs.
  bool isFresh() const;

  /// Skriver ut server, senaste synk och senaste avvikelse.
  void printStatus(Print& out) const;

 private:
  /// true om det är dags att skicka en ny förfrågan.
  bool isDue() const;

  /// Skickar en förfrågan. Returnerar false om den inte kunde skickas.
  bool sendRequest();

  /// Läser ett inkommet svar. Returnerar true om klockan ställdes.
  bool handleReply();

  /// Avslutar ett försök och planerar nästa.
  void finishAttempt(bool success);

  const char* const server_;
  TimeKeeper& clock_;
  const WifiLink& wifi_;
  const Timing timing_;

  WiFiUDP udp_;
  bool udpOpen_ = false;

  bool waiting_ = false;        ///< En förfrågan är skickad och väntar på svar.
  bool syncRequested_ = false;  ///< Synk begärd från konsolen.
  bool attempted_ = false;      ///< Minst ett försök har gjorts.
  bool lastAttemptOk_ = false;
  bool everSynced_ = false;

  uint32_t sentMs_ = 0;         ///< millis() när förfrågan skickades.
  uint32_t lastPollMs_ = 0;     ///< millis() när svar senast efterfrågades.
  uint32_t lastAttemptMs_ = 0;  ///< millis() när senaste försöket avslutades.
  uint32_t lastSyncMs_ = 0;     ///< millis() vid senaste lyckade synk.
  int32_t lastCorrectionMs_ = 0;
  uint32_t nonce_ = 0;          ///< Känns igen i svaret, så gamla svar kastas.
};
