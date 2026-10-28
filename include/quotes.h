/**
 * @file quotes.h
 * @brief Citaten som visas på skärmen.
 *
 * Varje citat har två delar som visas växelvis, 10 sekunder i taget.
 * Ett nytt citat visas varje minut, i den ordning de står här.
 *
 * Texten bryts automatiskt över skärmens två rader. Får en del inte plats
 * på 2 x 16 tecken visas den i flera sidor. å ä ö Å Ä Ö fungerar.
 * Ett " i texten skrivs som \".
 */
#pragma once

#include "Display.h"

constexpr Quote kQuotes[] = {
//  {"Citat 1", "Citat 2"},
    {"Detta meddelande sponsras av:", "Advantek Group"},
    {"You will be okay", "You have no choice"},
    {"All will be fine", "You cant stop it"},
    {"You will succed", "It is inevitable"},
    {"Better days are already coming", "Its too late now"},
    {"Happiness will find you", "You cannot escape"},
    {"Strength is within you", "It cannot be removed"},
    {"Limited edition!", "Best in class!"},
    {"'Sitat'", "Oj jag stavade fel"},
    {"More than 0 sold", "Less than 2 sold"},
    {"That's no moon!", "It's a space station!"},
    {"This show was brought to you by", "Advantek Group"},
    {"Teetsuuuuoooo!", "Kaaneeeedaaaa!"},
    {"NP is not in P!", "It's probably true"},
    {"Internet enabled", "Hacking in progress"},
    {"I know a bot", "ROS2lib::Bot Anna"},
    {"BLOOD FOR THE BLOOD GOD", "SKULLS FOR THE SKULL THRONE"},
    {"Stockholmskontoret > Linköping", "Kom och gör något åt det!"},
    {"Kompetens, precision, oberoende", "Advantek Group"},
    {"Modiga, Precisa", "Genuina, Tillsammans"},
    {"if (advantek) {printf (\"<3\");}", "<3"},
    {"You have my sword! And my bow!", "And my axe!"},
    {"Cough or sneeze into your elbow!", "Stay at home if you are sick!"},
    {"Hello!", "Im glad you are here!"},
    {"Straight outta Kristiansand,", "crazy konsultfirma named Advantek"},
    {"Kontor i Kristiansand, Oslo, Arendal", "Kongsberg, Trollhättan, Göteborg..."},
    {"... Linköping, Norrköping, Umeå", "Örnsköldsvik och Stockholm!"},
    {"Cough or sneeze into your elbow!", "Stay at home if you are sick!"},
};

constexpr size_t kQuoteCount = sizeof(kQuotes) / sizeof(kQuotes[0]);
