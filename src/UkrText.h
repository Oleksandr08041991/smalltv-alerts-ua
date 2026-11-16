// UkrText.h — small uppercase Ukrainian 5x7 bitmap font for the 240x240 display.
// Font shapes authored for this project; built-in ASCII still uses Arduino_GFX.
// UTF-8 lowercase Ukrainian is mapped to uppercase. Glyph lookup tables live
// in DRAM (intentionally NOT PROGMEM): the ESP8266 crash at EPC 0x402187c8
// was in drawAt during a non-32-bit access to flash at 0x402a9b66.
// The 306-byte RAM tradeoff avoids any alignment-sensitive flash access.
// Only Cyrillic capitals needed
// for alert labels are supported; unknown characters show a blank glyph.
#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "Gfx.h"

namespace ukrtext {
static uint16_t kCodepoints[] = {0x410, 0x411, 0x412, 0x413, 0x490, 0x414, 0x415, 0x404, 0x416, 0x417, 0x418, 0x406, 0x407, 0x419, 0x41A, 0x41B, 0x41C, 0x41D, 0x41E, 0x41F, 0x420, 0x421, 0x422, 0x423, 0x424, 0x425, 0x426, 0x427, 0x428, 0x429, 0x42C, 0x42E, 0x42F, 0x27};
static uint8_t kRows[][7] = {
  {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001},
  {0b11111, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b11110},
  {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110},
  {0b11111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000},
  {0b00011, 0b11110, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000},
  {0b00110, 0b01010, 0b01010, 0b01010, 0b01010, 0b11111, 0b10001},
  {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111},
  {0b01111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b01111},
  {0b10101, 0b10101, 0b01110, 0b00100, 0b01110, 0b10101, 0b10101},
  {0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110},
  {0b10001, 0b10011, 0b10101, 0b10101, 0b11001, 0b10001, 0b10001},
  {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
  {0b01010, 0b00000, 0b01110, 0b00100, 0b00100, 0b00100, 0b01110},
  {0b10101, 0b00000, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001},
  {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001},
  {0b00111, 0b01001, 0b01001, 0b01001, 0b10001, 0b10001, 0b10001},
  {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001},
  {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001},
  {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110},
  {0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001},
  {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000},
  {0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111},
  {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100},
  {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b01000, 0b10000},
  {0b00100, 0b01110, 0b10101, 0b10101, 0b01110, 0b00100, 0b00100},
  {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001},
  {0b10010, 0b10010, 0b10010, 0b10010, 0b10010, 0b11111, 0b00001},
  {0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00001},
  {0b10101, 0b10101, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111},
  {0b10101, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111, 0b00001},
  {0b10000, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b11110},
  {0b10010, 0b10101, 0b10101, 0b11101, 0b10101, 0b10101, 0b10010},
  {0b01111, 0b10001, 0b10001, 0b01111, 0b00101, 0b01001, 0b10001},
  {0b00100, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}
};
constexpr uint8_t kCount = 34;

inline uint16_t nextCodepoint(const char*& p) {
  uint8_t a = static_cast<uint8_t>(*p++);
  if (a < 128) return a;
  uint8_t b = static_cast<uint8_t>(*p);
  if ((a & 0xE0) == 0xC0 && (b & 0xC0) == 0x80) {
    ++p; return (uint16_t(a & 31) << 6) | (b & 63);
  }
  return '?';
}
inline uint16_t upper(uint16_t cp) {
  if (cp >= 0x430 && cp <= 0x44F) return cp - 32;
  if (cp == 0x491) return 0x490;
  if (cp == 0x454) return 0x404;
  if (cp == 0x456) return 0x406;
  if (cp == 0x457) return 0x407;
  return cp;
}
inline int glyphIndex(uint16_t cp) {
  for (uint8_t i=0;i<kCount;i++)
    if (kCodepoints[i] == cp) return i;
  return -1;
}
inline int textWidth(const char* s, uint8_t scale=1) {
  int w=0;
  if (!s) return 0;
  while (*s) { nextCodepoint(s); w += 6*scale; }
  return w;
}
inline void drawAt(Arduino_GFX* gfx, const char* s, int x, int y,
                   uint8_t scale, uint16_t color) {
  if (!gfx || !s) return;
  while (*s) {
    uint16_t cp=upper(nextCodepoint(s));
    int idx=glyphIndex(cp);
    if (idx >= 0) {
      for (int ry=0;ry<7;ry++) {
        uint8_t mask=kRows[idx][ry];
        for (int col=0;col<5;col++)
          if (mask & (1u << (4-col)))
            gfx->fillRect(x+col*scale,y+ry*scale,scale,scale,color);
      }
    } else if (cp >= 32 && cp < 127) {
      char ch[2]={(char)cp,0};
      gfx->setTextSize(scale);
      gfx->setTextColor(color);
      gfx->setCursor(x,y);
      gfx->print(ch);
    }
    x+=6*scale;
  }
}
inline void centered(Arduino_GFX* gfx,const char* s,int y,uint8_t scale,uint16_t color) {
  drawAt(gfx,s,(TFT_WIDTH-textWidth(s,scale))/2,y,scale,color);
}
} // namespace ukrtext
