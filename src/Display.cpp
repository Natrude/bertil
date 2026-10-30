/**
 * @file Display.cpp
 * @brief Implementation av skärmen.
 */
#include "Display.h"

Display::Display(const Pins& pins, const Scheduler& scheduler,
                 const Quote* quotes, size_t quoteCount, const Timing& timing)
    : scheduler_(scheduler),
      quotes_(quotes),
      quoteCount_(quoteCount),
      timing_(timing),
      lcd_(pins.rs, pins.enable, pins.d4, pins.d5, pins.d6, pins.d7) {}

void Display::begin() {
  // Utan R/W-pinne kan skärmen inte läsas av, så begin() märker inte om
  // den saknas. Fel kommer bara om pinnarna är ogiltiga.
  if (lcd_.begin(lcdtext::kCols, lcdtext::kRows) != 0) {
    Serial.println(F("Fel: kan inte starta skärmen."));
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

  if (striking) {
    showText("Slår nu!", "");
    return;
  }

  // Cykeln följer klockans sekunder, så att nedräkningen syns när det slår
  // på hel minut. Innan klockan är ställd används kortets egen räknare.
  uint8_t second;
  uint32_t minute;
  if (local != nullptr) {
    second = local->second();
    minute = local->unixtime() / 60UL;
  } else {
    second = (millis() / 1000UL) % 60UL;
    minute = millis() / 60000UL;
  }

  if (second < timing_.countdownSec || quoteCount_ == 0) {
    showCountdown(local);
    return;
  }

  // Delarna visas växelvis: första, andra, första, andra.
  const uint8_t quoteSecond = second - timing_.countdownSec;
  const uint8_t slot = quoteSecond / timing_.quotePartSec;
  const Quote& quote = quotes_[minute % quoteCount_];
  showQuotePart(slot % 2 == 0 ? quote.first : quote.second,
                quoteSecond % timing_.quotePartSec);
}

void Display::showCountdown(const DateTime* local) {
  if (local == nullptr) {
    showText("Väntar på tid", "från NTP");
    return;
  }

  StrikeTime entry;
  uint32_t secondsUntil = 0;
  if (!scheduler_.next(*local, entry, secondsUntil)) {
    showText("Inget schema", "");
    return;
  }

  if (secondsUntil == 0) {
    showText("Slår nu!", "");
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
  showText(row0, row1);
}

void Display::showQuotePart(const char* utf8, uint8_t secondInPart) {
  char text[kMaxQuoteLines * (lcdtext::kCols + 1) + 1];
  lcdtext::toLcd(utf8, text, sizeof(text));

  lcdtext::Line lines[kMaxQuoteLines];
  const uint8_t count = lcdtext::wrap(text, lines, kMaxQuoteLines);

  // Får texten inte plats på två rader bläddras den en rad i taget, så att
  // föregående rad står kvar som sammanhang: rad 1-2, sedan 2-3 och så vidare.
  const uint8_t views = count > lcdtext::kRows ? count - lcdtext::kRows + 1 : 1;
  const uint8_t view = secondInPart * views / timing_.quotePartSec;

  writeRow(0, lines[view].text);
  writeRow(1, view + 1 < count ? lines[view + 1].text : "");
}

void Display::showText(const char* row0, const char* row1) {
  char text[lcdtext::kCols + 1];
  lcdtext::toLcd(row0, text, sizeof(text));
  writeRow(0, text);
  lcdtext::toLcd(row1, text, sizeof(text));
  writeRow(1, text);
}

void Display::writeRow(uint8_t row, const char* lcdText) {
  char text[lcdtext::kCols + 1];
  size_t length = strnlen(lcdText, lcdtext::kCols);
  memcpy(text, lcdText, length);
  while (length < lcdtext::kCols) {
    text[length++] = ' ';
  }
  text[lcdtext::kCols] = '\0';

  if (strcmp(text, shown_[row]) == 0) {
    return;
  }

  lcd_.setCursor(0, row);
  for (uint8_t i = 0; i < lcdtext::kCols; ++i) {
    lcd_.write(static_cast<uint8_t>(text[i]));
  }
  memcpy(shown_[row], text, sizeof(text));
}
