#pragma once

#include "Arduboy2.h"
#include "pics/font.h"

#define pgm_read_byte_and_increment(addr)   \
(__extension__({                \
    uint16_t __addr16 = (uint16_t)(addr); \
    uint8_t __result;           \
    __asm__ __volatile__        \
    (                           \
        "lpm %0, Z+" "\n\t"            \
        : "=r" (__result)       \
        : "z" (__addr16)        \
        : "r0"                  \
    );                          \
    __result;                   \
}))

class ArduboyRem : public Arduboy2
{
  public:
    static byte flicker;
    static uint16_t nextFrameStart;
    static uint16_t eachFrameMicros;

    ArduboyRem();
    
    void drawMaskBitmap(int8_t x, int8_t y, const uint8_t *bitmap, uint8_t hflip = 0);
    virtual size_t write(uint8_t);
    void ArduboyRem::drawChar(uint8_t x, uint8_t y, unsigned char c);
    void ArduboyRem::printBytePadded(uint8_t x, uint8_t y, byte num);
    void inline drawByte(uint8_t x, uint8_t y, uint8_t color)
    {
      sBuffer[(y*WIDTH) + x] = color;
    }
    void ArduboyRem::setFrameRate(uint16_t rate);
    void ArduboyRem::nextFrame();
    void ArduboyRem::drawFastHLine(int16_t x, uint8_t y, int16_t w, uint8_t color);
};