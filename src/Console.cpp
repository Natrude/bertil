/**
 * @file Console.cpp
 * @brief Implementation av seriekonsolen.
 */
#include "Console.h"

#include <stdio.h>
#include <string.h>

Console::Console(Stream& io, TimeKeeper& clock, BellStriker& striker,
                 const Scheduler& scheduler, WifiLink& wifi, NtpSync& ntp)
    : io_(io),
      clock_(clock),
      striker_(striker),
      scheduler_(scheduler),
      wifi_(wifi),
      ntp_(ntp) {}

void Console::begin() {
  io_.println(F("Klockslagare redo. Skriv 'help' för kommandon."));
}

void Console::update() {
  while (io_.available() > 0) {
    const char c = static_cast<char>(io_.read());

    // Både CR och LF avslutar en rad, så det spelar ingen roll
    // vilket radslut terminalen skickar.
    if (c == '\r' || c == '\n') {
      if (overflow_) {
        io_.println(F("Fel: raden är för lång."));
      } else if (length_ > 0) {
        line_[length_] = '\0';
        execute(line_);
      }
      length_ = 0;
      overflow_ = false;
      continue;
    }

    // Spara tecknet om det får plats. Annars markera raden som för lång
    // och släng resten fram till nästa radslut.
    if (length_ < kLineCapacity - 1) {
      line_[length_++] = c;
    } else {
      overflow_ = true;
    }
  }
}

void Console::execute(char* line) {
  if (strcmp(line, "help") == 0 || strcmp(line, "?") == 0) {
    printHelp();
  } else if (strcmp(line, "time") == 0) {
    printTime();
  } else if (strncmp(line, "set ", 4) == 0) {
    handleSet(line);
  } else if (strcmp(line, "strike") == 0 || strncmp(line, "strike ", 7) == 0) {
    handleStrike(line);
  } else if (strcmp(line, "schedule") == 0) {
    printSchedule();
  } else if (strcmp(line, "wifi") == 0) {
    wifi_.printStatus(io_);
    ntp_.printStatus(io_);
  } else if (strcmp(line, "sync") == 0) {
    handleSync();
  } else {
    io_.print(F("Okänt kommando: "));
    io_.println(line);
  }
}

void Console::printHelp() {
  io_.println(F("Kommandon:"));
  io_.println(F("  time                     visa lokal tid"));
  io_.println(F("  set YYYY-MM-DD hh:mm:ss  ställ klockan (lokal tid)"));
  io_.println(F("  strike [n]               slå n gånger nu"));
  io_.println(F("  schedule                 visa slagschemat"));
  io_.println(F("  wifi                     visa WiFi- och NTP-läget"));
  io_.println(F("  sync                     hämta tiden från NTP nu"));
}

void Console::printTime() {
  DateTime now;
  if (!clock_.readLocal(now)) {
    io_.println(F("Fel: klockan är inte ställd. Använd 'set'."));
    return;
  }
  printDateTime(io_, now);
  io_.println();
}

void Console::printSchedule() {
  static const char* const kDayNames[7] = {"sön", "mån", "tis", "ons",
                                           "tor", "fre", "lör"};
  io_.print(F("  Dagar:"));
  // Måndag först, söndag sist.
  for (uint8_t i = 1; i <= 7; ++i) {
    const uint8_t day = i % 7;
    if (scheduler_.strikesOn(day)) {
      io_.print(' ');
      io_.print(kDayNames[day]);
    }
  }
  io_.println();

  char buffer[24];
  for (size_t i = 0; i < scheduler_.size(); ++i) {
    const StrikeTime& e = scheduler_.entry(i);
    snprintf(buffer, sizeof(buffer), "  %02u:%02u  %u slag", e.hour, e.minute,
             e.strikes);
    io_.println(buffer);
  }
}

void Console::handleSet(const char* line) {
  // sscanf på AVR saknar %hhu, så värdena läses som int och kontrolleras.
  int year, month, day, hour, minute, second;
  const int parsed = sscanf(line, "set %d-%d-%d %d:%d:%d", &year, &month, &day,
                            &hour, &minute, &second);

  if (parsed != 6 || year < 2024 || year > 2099 || hour < 0 || hour > 23 ||
      minute < 0 || minute > 59 || second < 0 || second > 59) {
    io_.println(F("Fel: använd formatet set YYYY-MM-DD hh:mm:ss"));
    return;
  }

  const DateTime local(year, month, day, hour, minute, second);

  // isValid() fångar datum som inte finns, t.ex. 2026-02-30.
  if (!local.isValid()) {
    io_.println(F("Fel: ogiltigt datum."));
    return;
  }

  clock_.setLocal(local);
  io_.print(F("Klockan ställd: "));
  printTime();
}

void Console::handleSync() {
  if (!wifi_.isConnected()) {
    io_.println(F("Fel: WiFi är inte anslutet."));
    return;
  }
  ntp_.requestSync();
  io_.println(F("Synkar."));
}

void Console::handleStrike(const char* line) {
  int count = 1;
  if (strlen(line) > 6 && sscanf(line + 6, "%d", &count) != 1) {
    io_.println(F("Fel: använd strike eller strike n"));
    return;
  }

  if (count < 1 || count > BellStriker::kMaxStrikes) {
    io_.print(F("Fel: antal slag ska vara 1 till "));
    io_.println(BellStriker::kMaxStrikes);
    return;
  }

  if (striker_.request(static_cast<uint8_t>(count))) {
    io_.println(F("Slår."));
  } else {
    io_.println(F("Upptagen, försök igen."));
  }
}
