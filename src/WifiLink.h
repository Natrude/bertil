/**
 * @file WifiLink.h
 * @brief Håller kortet anslutet till WiFi.
 */
#pragma once

#include <Arduino.h>

/**
 * @brief Icke-blockerande WiFi-anslutning via modemet på Uno R4 WiFi.
 *
 * Modemet (en ESP32-S3) ansluter i bakgrunden. begin() startar anslutningen
 * och update() läser av läget med jämna mellanrum. Är kortet inte anslutet
 * görs ett nytt försök efter en stund.
 *
 * Alla anrop till modemet har en tidsgräns som är kortare än watchdogen,
 * så att ett modem som slutat svara inte startar om kortet.
 */
class WifiLink {
 public:
  /// Tider för anslutningen, i millisekunder.
  struct Timing {
    uint16_t pollMs;          ///< Hur ofta läget läses av.
    uint32_t retryMs;         ///< Väntetid mellan anslutningsförsök.
    uint16_t modemTimeoutMs;  ///< Längsta väntan på svar från modemet.
  };

  WifiLink(const char* ssid, const char* password, const Timing& timing);

  /// Kontrollerar modemet och börjar ansluta. Anropas en gång från setup(),
  /// innan watchdogen startas.
  void begin();

  /// Läser av läget och ansluter igen vid behov. Anropas varje varv i loop().
  void update();

  /// true när kortet är anslutet till nätverket.
  bool isConnected() const { return connected_; }

  /// Skriver ut läge, nätverk, IP-adress och signalstyrka.
  void printStatus(Print& out);

 private:
  /// Skickar ett anslutningsförsök till modemet. Väntar inte på resultatet.
  void connect();

  const char* const ssid_;
  const char* const password_;
  const Timing timing_;

  bool moduleOk_ = false;
  bool connected_ = false;
  uint8_t lastStatus_ = 0;
  uint32_t lastPollMs_ = 0;
  uint32_t lastAttemptMs_ = 0;
};
