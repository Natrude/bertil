/**
 * @file Display.cpp
 * @brief Implementation av skärmen.
 */
#include "Display.h"

#include "LcdText.h"

Display::Display(const Scheduler& scheduler) : scheduler_(scheduler) {}

void Display::begin() {
  // begin() letar själv upp adapterns adress och hur den är kopplad.
  if (lcd_.begin(kCols, kRows) != 0) {
    Serial.println(F("Fel: hittar ingen skärm."));
    return;
  }
  present_ = true;

  for (uint8_t i = 0; i < lcdtext::kGlyphCount; ++i) {
    lcd_.createChar(i, lcdtext::glyph(i));
  }
  // createChar() lämnar markören i teckenminnet. clear() flyttar tillbaka
  // den till skärmen.
  lcd_.clear();
}

void Display::update(const DateTime* local, bool striking) {
  if (!present_) {
    return;
  }

  if (local == nullptr) {
    show("Väntar på tid", "från NTP");
    return;
  }

  StrikeTime entry;
  uint32_t secondsUntil = 0;
  if (!scheduler_.next(*local, entry, secondsUntil)) {
    show("Inget schema", "");
    return;
  }

  if (striking || secondsUntil == 0) {
    show("Slår nu!", "");
    return;
  }

  char row0[24];
  char row1[24];
  snprintf(row0, sizeof(row0), "Nästa slag %02u:%02u", entry.hour,
           entry.minute);
  snprintf(row1, sizeof(row1), "om %02lu:%02lu:%02lu",
           static_cast<unsigned long>(secondsUntil / 3600),
           static_cast<unsigned long>(secondsUntil / 60 % 60),
           static_cast<unsigned long>(secondsUntil % 60));
  show(row0, row1);
}

void Display::show(const char* row0, const char* row1) {
  writeRow(0, row0);
  writeRow(1, row1);
}

void Display::writeRow(uint8_t row, const char* utf8) {
  char text[kCols + 1];
  size_t length = lcdtext::toLcd(utf8, text, sizeof(text));
  while (length < kCols) {
    text[length++] = ' ';
  }
  text[kCols] = '\0';

  if (strcmp(text, shown_[row]) == 0) {
    return;
  }

  lcd_.setCursor(0, row);
  for (uint8_t i = 0; i < kCols; ++i) {
    lcd_.write(static_cast<uint8_t>(text[i]));
  }
  memcpy(shown_[row], text, sizeof(text));
}
