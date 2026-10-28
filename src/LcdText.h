/**
 * @file LcdText.h
 * @brief Översätter text till teckenkoder för en HD44780-skärm.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace lcdtext {

/// Skärmens storlek i tecken.
constexpr uint8_t kCols = 16;
constexpr uint8_t kRows = 2;

/// En skärmrad i skärmens teckenkoder, utfylld med mellanslag.
struct Line {
  char text[kCols + 1];
};

/// Antal egna tecken som används (å ä ö Å Ä Ö).
constexpr uint8_t kGlyphCount = 6;

/// Bitmapp för egna tecknet med index i, 8 rader à 5 punkter.
const uint8_t* glyph(uint8_t index);

/**
 * @brief Översätter UTF-8 till skärmens teckenkoder.
 *
 * Vanliga ASCII-tecken behålls. å ä ö Å Ä Ö blir egna tecken, eftersom
 * skärmarnas teckenrom skiljer sig åt och ofta saknar dem. Övriga tecken
 * blir '?'. Resultatet avslutas alltid med en nolla.
 *
 * @param utf8 Texten som ska översättas.
 * @param out  Buffert för resultatet.
 * @param cap  Buffertens storlek, inklusive avslutande nolla.
 * @return Antal tecken i resultatet.
 */
size_t toLcd(const char* utf8, char* out, size_t cap);

/**
 * @brief Bryter en text till skärmrader.
 *
 * Texten bryts vid mellanslag så att varje rad får högst kCols tecken.
 * Ord som är längre än en rad delas med ett bindestreck.
 *
 * @param lcdText  Text i skärmens teckenkoder (från toLcd()), så att varje
 *                 byte är ett tecken på skärmen.
 * @param lines    Utrymme för raderna.
 * @param maxLines Antal rader som får plats. Text som inte ryms tappas.
 * @return Antal rader, minst 1 (om maxLines > 0).
 */
uint8_t wrap(const char* lcdText, Line* lines, uint8_t maxLines);

}  // namespace lcdtext
