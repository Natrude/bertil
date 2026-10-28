/**
 * @file NtpSync.cpp
 * @brief Implementation av NTP-synken.
 */
#include "NtpSync.h"

namespace {

constexpr uint16_t kNtpPort = 123;

/// Lokal UDP-port för svaren.
constexpr uint16_t kLocalPort = 2390;

constexpr size_t kPacketSize = 48;

/// Hur ofta modemet tillfrågas om ett svar har kommit. Varje fråga är ett
/// kommando till modemet, så det görs inte varje varv i loop().
constexpr uint16_t kReplyPollMs = 20;

/// Sekunder från 1900 (NTP) till 1970 (unix).
constexpr uint32_t kNtpToUnix = 2208988800UL;

/// Svar före 2024 är uppenbart fel.
constexpr uint32_t kMinValidUnix = 1704067200UL;

uint32_t readUint32(const uint8_t* p) {
  return (static_cast<uint32_t>(p[0]) << 24) |
         (static_cast<uint32_t>(p[1]) << 16) |
         (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

void writeUint32(uint8_t* p, uint32_t v) {
  p[0] = v >> 24;
  p[1] = v >> 16;
  p[2] = v >> 8;
  p[3] = v;
}

}  // namespace

NtpSync::NtpSync(const char* server, TimeKeeper& clock, const WifiLink& wifi,
                 const Timing& timing)
    : server_(server), clock_(clock), wifi_(wifi), timing_(timing) {}

void NtpSync::update() {
  if (!wifi_.isConnected()) {
    // Socketen i modemet överlever inte alltid en ny anslutning.
    // Öppna en ny när WiFi är tillbaka.
    if (udpOpen_) {
      udp_.stop();
      udpOpen_ = false;
    }
    waiting_ = false;
    return;
  }

  if (!udpOpen_) {
    if (!udp_.begin(kLocalPort)) {
      return;
    }
    udpOpen_ = true;
  }

  if (!waiting_) {
    if (isDue()) {
      syncRequested_ = false;
      if (sendRequest()) {
        waiting_ = true;
      } else {
        Serial.println(F("NTP: kunde inte skicka förfrågan."));
        finishAttempt(false);
      }
    }
    return;
  }

  if (millis() - lastPollMs_ < kReplyPollMs) {
    return;
  }
  lastPollMs_ = millis();

  if (handleReply()) {
    finishAttempt(true);
  } else if (millis() - sentMs_ >= timing_.replyTimeoutMs) {
    Serial.println(F("NTP: inget svar."));
    finishAttempt(false);
  }
}

bool NtpSync::isFresh() const {
  return everSynced_ && millis() - lastSyncMs_ < timing_.staleMs;
}

void NtpSync::printStatus(Print& out) const {
  out.print(F("NTP: "));
  out.println(server_);
  if (!everSynced_) {
    out.println(F("  ingen synk ännu"));
    return;
  }
  out.print(F("  senaste synk: för "));
  out.print((millis() - lastSyncMs_) / 1000UL);
  out.println(F(" s sedan"));
  out.print(F("  korrigering:  "));
  out.print(lastCorrectionMs_);
  out.println(F(" ms"));
}

bool NtpSync::isDue() const {
  if (syncRequested_ || !attempted_) {
    return true;
  }
  const uint32_t wait = lastAttemptOk_ ? timing_.intervalMs : timing_.retryMs;
  return millis() - lastAttemptMs_ >= wait;
}

bool NtpSync::sendRequest() {
  uint8_t packet[kPacketSize] = {};
  // LI = 0, version 4, läge 3 (klient).
  packet[0] = 0x23;

  // Servern kopierar sändtiden (byte 40-47) till svaret (byte 24-31).
  // Här läggs ett eget löpnummer i stället för en tid, så att svaret kan
  // knytas till just den här förfrågan.
  ++nonce_;
  writeUint32(&packet[40], nonce_);
  writeUint32(&packet[44], millis());

  if (!udp_.beginPacket(server_, kNtpPort)) {
    return false;
  }
  udp_.write(packet, kPacketSize);
  sentMs_ = millis();
  lastPollMs_ = sentMs_;
  return udp_.endPacket() == 1;
}

bool NtpSync::handleReply() {
  // Läs alla väntande paket. Gamla svar på tidigare förfrågningar kastas.
  while (udp_.parsePacket() > 0) {
    const uint32_t receivedMs = millis();

    uint8_t packet[kPacketSize];
    if (udp_.read(packet, kPacketSize) != static_cast<int>(kPacketSize)) {
      continue;
    }

    const uint8_t leap = packet[0] >> 6;
    const uint8_t mode = packet[0] & 0x07;
    const uint8_t stratum = packet[1];
    if (readUint32(&packet[24]) != nonce_ || mode != 4 || leap == 3 ||
        stratum == 0 || stratum > 15) {
      continue;
    }

    // Serverns sändtid. Noll betyder att servern inte har någon tid, och
    // måste kastas här eftersom den annars blir ett giltigt datum år 2036.
    const uint32_t ntpSeconds = readUint32(&packet[40]);
    const uint32_t fraction = readUint32(&packet[44]);
    if (ntpSeconds == 0 && fraction == 0) {
      continue;
    }

    // Osignerad aritmetik ger rätt unixtid även efter att NTP-sekunderna
    // slår runt år 2036.
    const uint32_t seconds = ntpSeconds - kNtpToUnix;
    if (seconds < kMinValidUnix) {
      continue;
    }

    // Svaret var på väg ungefär halva rundturen. Räkna fram tiden nu.
    const uint32_t halfRoundTripMs = (receivedMs - sentMs_) / 2;
    const uint32_t totalMs = static_cast<uint32_t>(
        (static_cast<uint64_t>(fraction) * 1000) >> 32) + halfRoundTripMs;

    const bool wasSet = clock_.isSet();
    lastCorrectionMs_ =
        clock_.setUtc(seconds + totalMs / 1000, totalMs % 1000);

    DateTime local;
    clock_.readLocal(local);
    Serial.print(F("NTP: "));
    printDateTime(Serial, local);
    if (wasSet) {
      Serial.print(F("  korrigering "));
      Serial.print(lastCorrectionMs_);
      Serial.print(F(" ms"));
    }
    Serial.print(F("  rundtur "));
    Serial.print(receivedMs - sentMs_);
    Serial.println(F(" ms"));
    return true;
  }
  return false;
}

void NtpSync::finishAttempt(bool success) {
  waiting_ = false;
  attempted_ = true;
  lastAttemptOk_ = success;
  lastAttemptMs_ = millis();
  if (success) {
    everSynced_ = true;
    lastSyncMs_ = lastAttemptMs_;
  }
}
