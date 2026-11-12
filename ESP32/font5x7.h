#ifndef FONT5X7_H
#define FONT5X7_H

#include <Arduino.h>

// Tiny 5x7 bitmap font used by the display driver.
// Each glyph is 7 rows; bit4 of a row = leftmost column.
struct FontGlyph {
    char c;
    uint8_t rows[7];
};

extern const FontGlyph font5x7Table[];
extern const int font5x7Count;

// Returns pointer to the 7 row bytes for `c`, or a blank glyph when unknown.
const uint8_t *font5x7Glyph(char c);

#endif
