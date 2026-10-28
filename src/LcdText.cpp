/**
 * @file LcdText.cpp
 * @brief Implementation av textöversättningen.
 */
#include "LcdText.h"

namespace lcdtext {
namespace {

/// Egna tecken laddas i CGRAM 0-5. Skärmen visar samma tecken för koderna
/// 8-15, och de används här så att ingen nolla hamnar mitt i en sträng.
constexpr char kGlyphBase = 8;

struct Mapping {
  uint16_t codepoint;
  uint8_t glyph;
};

constexpr Mapping kMappings[kGlyphCount] = {
    {0x00E5, 0},  // å
    {0x00E4, 1},  // ä
    {0x00F6, 2},  // ö
    {0x00C5, 3},  // Å
    {0x00C4, 4},  // Ä
    {0x00D6, 5},  // Ö
};

/// Raderna ritas uppifrån. Sista raden lämnas tom, precis som för
/// tecknen i skärmens teckenrom, så att alla bokstäver står på samma linje.
constexpr uint8_t kGlyphs[kGlyphCount][8] = {
    {0b00100, 0b01010, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111, 0b00000},
    {0b01010, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111, 0b00000},
    {0b01010, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110, 0b00000},
    {0b00100, 0b01010, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001, 0b00000},
    {0b01010, 0b00000, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001, 0b00000},
    {0b01010, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110, 0b00000},
};

/// Läser ett UTF-8-tecken och flyttar fram p. Felaktiga byte ger 0xFFFF.
uint16_t decode(const char*& p) {
  const uint8_t b0 = static_cast<uint8_t>(*p++);
  if (b0 < 0x80) {
    return b0;
  }
  // Tvåbytestecken räcker för alla bokstäver som kan behövas här.
  // Längre tecken hoppas över i sin helhet.
  uint8_t extra = 0;
  if ((b0 & 0xE0) == 0xC0) {
    extra = 1;
  } else if ((b0 & 0xF0) == 0xE0) {
    extra = 2;
  } else if ((b0 & 0xF8) == 0xF0) {
    extra = 3;
  } else {
    return 0xFFFF;
  }

  uint16_t cp = b0 & 0x1F;
  for (uint8_t i = 0; i < extra; ++i) {
    const uint8_t b = static_cast<uint8_t>(*p);
    if ((b & 0xC0) != 0x80) {
      return 0xFFFF;
    }
    ++p;
    cp = (cp << 6) | (b & 0x3F);
  }
  return extra == 1 ? cp : 0xFFFF;
}

}  // namespace

const uint8_t* glyph(uint8_t index) {
  return kGlyphs[index];
}

size_t toLcd(const char* utf8, char* out, size_t cap) {
  if (cap == 0) {
    return 0;
  }

  size_t length = 0;
  const char* p = utf8;
  while (*p != '\0' && length + 1 < cap) {
    const uint16_t cp = decode(p);

    char c = '?';
    if (cp >= 0x20 && cp < 0x7F) {
      c = static_cast<char>(cp);
    } else {
      for (const Mapping& m : kMappings) {
        if (m.codepoint == cp) {
          c = static_cast<char>(kGlyphBase + m.glyph);
          break;
        }
      }
    }
    out[length++] = c;
  }
  out[length] = '\0';
  return length;
}

}  // namespace lcdtext
