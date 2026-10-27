/**
 * @file WifiLink.cpp
 * @brief Implementation av WiFi-anslutningen.
 */
#include "WifiLink.h"

#include <Modem.h>
#include <WiFiS3.h>

namespace {

/// Svaret från firmwareVersion() när modemet inte svarar.
constexpr char kNoFirmware[] = "99.99.99";

/// Text för ett läge från WiFi.status().
const __FlashStringHelper* statusText(uint8_t status) {
  switch (status) {
    case WL_CONNECTED:
      return F("ansluten");
    case WL_NO_SSID_AVAIL:
      return F("hittar inte nätverket");
    case WL_CONNECT_FAILED:
      return F("anslutningen misslyckades (fel lösenord?)");
    case WL_CONNECTION_LOST:
      return F("anslutningen tappad");
    default:
      return F("inte ansluten");
  }
}

}  // namespace

WifiLink::WifiLink(const char* ssid, const char* password, const Timing& timing)
    : ssid_(ssid), password_(password), timing_(timing) {}

void WifiLink::begin() {
  // Första anropet startar modemet. Det kan ta en stund och görs därför
  // innan watchdogen är igång.
  const char* firmware = WiFi.firmwareVersion();
  if (strcmp(firmware, kNoFirmware) == 0) {
    // Utan svar från modemet skulle varje senare anrop vänta tills
    // tidsgränsen löper ut. Låt bli modemet helt i stället.
    Serial.println(F("Fel: WiFi-modemet svarar inte."));
    return;
  }
  moduleOk_ = true;

  Serial.print(F("WiFi-modemets firmware: "));
  Serial.println(firmware);
  if (strcmp(firmware, WIFI_FIRMWARE_LATEST_VERSION) != 0) {
    Serial.print(F("Varning: biblioteket är gjort för firmware "));
    Serial.print(WIFI_FIRMWARE_LATEST_VERSION);
    Serial.println(F(". Uppdatera modemet om WiFi krånglar."));
  }

  // Biblioteket väntar annars i upp till 10 s på varje svar, längre än
  // watchdogen tillåter. Tidsgränsen måste sättas efter att modemet startats,
  // eftersom starten återställer den.
  modem.timeout(timing_.modemTimeoutMs);

  // Med tidsgränsen 0 skickar WiFi.begin() bara uppgifterna till modemet och
  // returnerar direkt. Modemet ansluter sedan i bakgrunden.
  WiFi.setTimeout(0);

  connect();
}

void WifiLink::update() {
  if (!moduleOk_ || millis() - lastPollMs_ < timing_.pollMs) {
    return;
  }
  lastPollMs_ = millis();

  lastStatus_ = WiFi.status();
  const bool connected = lastStatus_ == WL_CONNECTED;

  if (connected && !connected_) {
    Serial.print(F("WiFi ansluten till "));
    Serial.print(ssid_);
    Serial.print(F(", IP "));
    Serial.println(WiFi.localIP());
  } else if (!connected && connected_) {
    Serial.println(F("WiFi tappad."));
  }
  connected_ = connected;

  if (!connected_ && millis() - lastAttemptMs_ >= timing_.retryMs) {
    Serial.print(F("WiFi: "));
    Serial.print(statusText(lastStatus_));
    Serial.println(F(". Försöker igen."));
    connect();
  }
}

void WifiLink::printStatus(Print& out) {
  if (!moduleOk_) {
    out.println(F("WiFi: modemet svarar inte."));
    return;
  }

  out.print(F("WiFi: "));
  out.println(statusText(lastStatus_));
  out.print(F("  nätverk: "));
  out.println(ssid_);
  if (connected_) {
    out.print(F("  IP:      "));
    out.println(WiFi.localIP());
    out.print(F("  signal:  "));
    out.print(WiFi.RSSI());
    out.println(F(" dBm"));
  }
}

void WifiLink::connect() {
  lastAttemptMs_ = millis();
  WiFi.begin(ssid_, password_);
}
