#include "ArduboyRem.h"

byte ArduboyRem::flicker = 0;
uint16_t ArduboyRem::nextFrameStart  = 0;
uint16_t ArduboyRem::eachFrameMicros = 0;

const uint8_t PROGMEM yMask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

ArduboyRem::ArduboyRem() : Arduboy2() {}


void ArduboyRem::drawMaskBitmap(int8_t x, int8_t y, const uint8_t *bitmap, uint8_t hflip)
{
	// Read size
	uint8_t w = pgm_read_byte(bitmap++);
	uint8_t h = pgm_read_byte(bitmap++);
	// Read hotspot
	int8_t hx = pgm_read_byte(bitmap++);
	int8_t hy = pgm_read_byte(bitmap++);

	y -= hy;
	if (hflip == 0) {
		x -= hx;
	} else {
		x -= w - hx;
	}

	// no need to draw at all if we're offscreen
	int8_t endX = x + w;
	if ((endX < 0 && x < 0)/* || x >= WIDTH*/ || int8_t(y + h) < 0 || y >= HEIGHT)
		return;

	//uint8_t yOffset = 1 << (y & 7);
	uint8_t yOffset = pgm_read_byte(yMask + (y & 7));
	const uint8_t* sourceAddress = bitmap;

	int8_t sRow = y >> 3;
	uint8_t uRow = sRow; // Same as 'sRow' but unsigned: will be used after clipping
	uint8_t rows = (h + 7) >> 3;
	// Top clipping: skip 'n' rows if outside the screen
	if (sRow < 0) {
		sourceAddress += (-sRow * w) << 1;
		rows += sRow;
		uRow = 0;
	}
	// Bottom clipping: stop as soon as we reach bottom of screen
	if (uRow + rows > (HEIGHT >> 3)) {
		rows = (HEIGHT >> 3) - uRow;
	}
	// Optimization: Perform 'x' clipping outside of loop
	uint8_t startCol = 0;
	uint8_t endCol = w;
	if (x < 0) {
		startCol = -x;
	}
	if (x + w > WIDTH) {
		endCol = WIDTH - x;
	}

	if (hflip == 0) {
		sourceAddress += (startCol << 1);
	} else {
		sourceAddress += ((w - endCol) << 1);
	}
	uint8_t sourceAddressSkip = (w - (endCol - startCol)) << 1;

	if (hflip == 0) {
		uint16_t destAddress = (uRow * WIDTH) + x + startCol;
		uint8_t destAddressSkip = (WIDTH - (endCol - startCol));

		for (uint8_t bRow = uRow; bRow < uint8_t(uRow + rows); bRow++, sourceAddress += sourceAddressSkip, destAddress += destAddressSkip) {
			for (uint8_t iCol = startCol; iCol < endCol; iCol++, destAddress ++) {
				// Draw mask first with AND and Draw color with OR:
				// The GCC compiler is doing a poor job because of its constraint to keep 'r1' always 0 (two useless clr __zero_reg__ here)
				// and also because it can't accept to keep the last 'mul' result in R1:R0, so there's a useless 'movw'. Total: 3 wasted clock cycles in this loop
				uint16_t sourceMask = ~(pgm_read_byte_and_increment(sourceAddress) * yOffset);
				uint16_t sourceByte = pgm_read_byte_and_increment(sourceAddress) * yOffset;

				sBuffer[destAddress] = (sBuffer[destAddress] & uint8_t(sourceMask)) | uint8_t(sourceByte);
				sBuffer[destAddress + WIDTH] = (sBuffer[destAddress + WIDTH] & uint8_t(sourceMask >> 8)) | uint8_t(sourceByte >> 8);
			}
		}
	}
	else {
		uint16_t destAddress = (uRow * WIDTH) + x + endCol - 1;
		uint8_t destAddressSkip = (WIDTH + (endCol - startCol));

		for (uint8_t bRow = uRow; bRow < uint8_t(uRow + rows); bRow++, sourceAddress += sourceAddressSkip, destAddress += destAddressSkip) {
			for (uint8_t iCol = startCol; iCol < endCol; iCol++, destAddress--) {
				uint16_t sourceMask = ~(pgm_read_byte_and_increment(sourceAddress) * yOffset);
				uint16_t sourceByte = pgm_read_byte_and_increment(sourceAddress) * yOffset;

				sBuffer[destAddress] = (sBuffer[destAddress] & uint8_t(sourceMask)) | uint8_t(sourceByte);
				sBuffer[destAddress + WIDTH] = (sBuffer[destAddress + WIDTH] & uint8_t(sourceMask >> 8)) | uint8_t(sourceByte >> 8);
			}
		}
	}
}

size_t ArduboyRem::write(uint8_t c)
{
	if (c == '\n')
	{
		cursor_y += textSize * 8;
		cursor_x = 0;
	}
	else if (c == '\r')
	{
		// skip em
	}
	else
	{
		drawChar(cursor_x, cursor_y, c);
		cursor_x += textSize * 6;
		if (textWrap && (cursor_x > (WIDTH - textSize * 6)))
		{
			// calling ourselves recursively for 'newline' is
			// 12 bytes smaller than doing the same math here
			write('\n');
		}
	}
	return 1;
}

void ArduboyRem::drawChar(uint8_t x, uint8_t y, unsigned char c)
{
	// RV optimization: get rid of 'size' scaling
	// Also remove transparent check
	// Force alignment on full byte in 'y'

	if ((x >= WIDTH) || (y >= HEIGHT))
	{
		return;
	}

	y >>= 3;

	for (uint8_t i = 0; i < 6; i++)
	{
		// use '~' to draw black on white text if font was created white on black
		// Note: Font has been changed to 6x8, at starts at character 32 instead of 0, and ends a char 127 (instead of 255)
		// so it occupies 6 bytes x 96 chars = 576 bytes (instead of the original arduboy font of 5 bytes x 256 chars = 1280 bytes)
		drawByte(x + i, y, pgm_read_byte(font + ((c - 32) * 6) + i));
	}
	//drawByte(x + 5, y, 0);
}

void ArduboyRem::printBytePadded(uint8_t x, uint8_t y, byte num)
{
	int ten = num / 10;
	int unit = num % 10;
	drawChar(x, y, '0' + ten);
	drawChar(x + 6, y, '0' + unit);
}

void ArduboyRem::setFrameRate(uint16_t rate)
{
	eachFrameMicros = rate;
}

void ArduboyRem::nextFrame()
{
	do {
		uint16_t now = micros();

		// if it's not time for the next frame yet
		int16_t diff = nextFrameStart - now;
		if (diff < 0) {
			// if we have more than 1ms to spare, lets sleep
			// we should be woken up by timer0 every 1ms, so this should be ok
			if (diff >= 1000)
				idle();
			continue;
		}

		nextFrameStart += eachFrameMicros;
		flicker++;
		return;
	} while (true);
}

void ArduboyRem::drawFastHLine(int16_t x, uint8_t y, int16_t w, uint8_t color)
{
	// Do y bounds checks
	if (y >= HEIGHT)
		return;

	int16_t xEnd = x + w; // last x point + 1

	// Check if the entire line is not on the display
	if (xEnd <= 0 || x >= WIDTH || w <= 0)
		return;

	// clip left edge
	if (x < 0)
		x = 0;

	// clip right edge
	if (xEnd > WIDTH)
		xEnd = WIDTH;

	// calculate final clipped width (even if unchanged)
	uint8_t clippedW = xEnd - x;

	// buffer pointer plus row offset + x offset
	// (y >> 3) * WIDTH, optimized to: ((y * 16) & 0xFF80) because gcc O3 is not smart enough to do it.
	uint8_t *pBuf = sBuffer + ((y * 16) & 0xFF80) + uint8_t(x);

	// pixel mask
	//uint8_t mask = 1 << (y & 7);
	uint8_t mask = pgm_read_byte(yMask + (y & 7));

	switch (color)
	{
	case WHITE:
		do
		{
			*pBuf++ |= mask;
		} while (--clippedW);
		break;

	case BLACK:
		mask = ~mask;
		do
		{
			*pBuf++ &= mask;
		} while (--clippedW);
		break;
	/* // GRAY is disabled because unused to remove useless code:
	case GRAY:
	{
		// TOOD: This could be optimized further, but unneeded since we clear display in gray now
		uint8_t alternate = (x + y + flicker) & 1;
		while (clippedW--)
		{
			if (alternate) {
				*pBuf |= mask;
			}
			pBuf++;
			alternate ^= 1;
		}
		break;
	}
	*/
	}
}

