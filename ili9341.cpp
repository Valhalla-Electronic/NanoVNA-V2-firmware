/*
 * Copyright (c) 2014-2015, TAKAHASHI Tomohiro (TTRFTECH) edy555@gmail.com
 * All rights reserved.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * The software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include "ili9341.hpp"
#include "Font5x7.h"
#include "numfont20x22.h"

#define RESET_ASSERT	;
#define RESET_NEGATE	;
#define CS_LOW			digitalWrite(ili9341_conf_cs, LOW)
#define CS_HIGH			digitalWrite(ili9341_conf_cs, HIGH)
#define DC_CMD			digitalWrite(ili9341_conf_dc, LOW)
#define DC_DATA			digitalWrite(ili9341_conf_dc, HIGH)




uint16_t ili9341_spi_buffers[ili9341_bufferSize * 2];

uint16_t* ili9341_spi_bufferA = ili9341_spi_buffers;
uint16_t* ili9341_spi_bufferB = &ili9341_spi_buffers[ili9341_bufferSize];

uint16_t* ili9341_spi_buffer = ili9341_spi_bufferA;

Pad ili9341_conf_cs;
Pad ili9341_conf_dc;
small_function<uint32_t(uint32_t sdi, int bits)> ili9341_spi_transfer;
small_function<void(uint32_t words)> ili9341_spi_transfer_bulk;
small_function<void()> ili9341_spi_wait_bulk;

void ssp_wait(void)
{
}


static inline void ssp_senddata(uint8_t x)
{
  ili9341_spi_transfer(x, 8);
}

static inline uint8_t ssp_sendrecvdata(uint8_t x)
{
	return (uint8_t) ili9341_spi_transfer(x, 8);
}

static inline void ssp_senddata16(uint16_t x)
{
  ili9341_spi_transfer(x, 16);
}

uint32_t txdmamode;


void
send_command(uint8_t cmd, int len, const uint8_t *data)
{
	CS_LOW;
	DC_CMD;
    delayMicroseconds(1);
	ssp_senddata(cmd);
	DC_DATA;
    delayMicroseconds(1);
	while (len-- > 0) {
	  ssp_senddata(*data++);
	}
	//CS_HIGH;
}

void
send_command16(uint8_t cmd, int data)
{
	CS_LOW;
	DC_CMD;
    delayMicroseconds(1);
	ssp_senddata(cmd);
	DC_DATA;
    delayMicroseconds(1);
	ssp_senddata16(byteReverse16(data));
	CS_HIGH;
}

const uint8_t ili9341_init_seq[] = {
		// cmd, len, data...,
		// Power control B
		0xCF, 3, 0x00, 0x83, 0x30,
		// Power on sequence control
		0xED, 4, 0x64, 0x03, 0x12, 0x81,
		//0xED, 4, 0x55, 0x01, 0x23, 0x01,
		// Driver timing control A
		0xE8, 3, 0x85, 0x01, 0x79,
		//0xE8, 3, 0x84, 0x11, 0x7a,
		// Power control A
		0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
		// Pump ratio control
		0xF7, 1, 0x20,
		// Driver timing control B
		0xEA, 2, 0x00, 0x00,
		// POWER_CONTROL_1
		0xC0, 1, 0x26,
		// POWER_CONTROL_2
		0xC1, 1, 0x11,
		// VCOM_CONTROL_1
		0xC5, 2, 0x35, 0x3E,
		// VCOM_CONTROL_2
		0xC7, 1, 0xBE,
		// MEMORY_ACCESS_CONTROL
		//0x36, 1, 0x48, // portlait
		0x36, 1, 0b00101000, // landscape
		// COLMOD_PIXEL_FORMAT_SET : 16 bit pixel
		0x3A, 1, 0x55,
		// Frame Rate
		0xB1, 2, 0x00, 0x1B,
		// Gamma Function Disable
		0xF2, 1, 0x08,
		// gamma set for curve 01/2/04/08
		0x26, 1, 0x01,
		// positive gamma correction
		0xE0, 15, 0x1F,  0x1A,  0x18,  0x0A,  0x0F,  0x06,  0x45,  0x87,  0x32,  0x0A,  0x07,  0x02,  0x07, 0x05,  0x00,
		// negativ gamma correction
		0xE1, 15, 0x00,  0x25,  0x27,  0x05,  0x10,  0x09,  0x3A,  0x78,  0x4D,  0x05,  0x18,  0x0D,  0x38, 0x3A,  0x1F,

		// Column Address Set
		0x2A, 4, 0x00, 0x00, 0x01, 0x3f, // width 320
		// Page Address Set
		0x2B, 4, 0x00, 0x00, 0x00, 0xef, // height 240

		// entry mode
		0xB7, 1, 0x06,
		// display function control
		0xB6, 4, 0x0A, 0x82, 0x27, 0x00,

		// control display
		//0x53, 1, 0x0c,
		// diaplay brightness
		//0x51, 1, 0xff,

		// sleep out
		0x11, 0,
		0 // sentinel
};

void
ili9341_init(void)
{
  DC_DATA;
  RESET_ASSERT;
  delay(10);
  RESET_NEGATE;

  ili9341_spi_wait_bulk();

  send_command(0x01, 0, NULL); // SW reset
  delay(5);
  send_command(0x28, 0, NULL); // display off

  const uint8_t *p;
  for (p = ili9341_init_seq; *p; ) {
	send_command(p[0], p[1], &p[2]);
	p += 2 + p[1];
	delay(5);
  }

  delay(100);
  send_command(0x29, 0, NULL); // display on
}

void ili9341_pixel(int x, int y, int color)
{
	uint8_t xx[4] = { x >> 8, x, (x+1) >> 8, (x+1) };
	uint8_t yy[4] = { y >> 8, y, (y+1) >> 8, (y+1) };
	uint8_t cc[2] = { color >> 8, color };
	ili9341_spi_wait_bulk();
	send_command(0x2A, 4, xx);
	send_command(0x2B, 4, yy);
	send_command(0x2C, 2, cc);
	//send_command16(0x2C, color);
}



void ili9341_fill(int x, int y, int w, int h, int color)
{
	uint8_t xx[4] = { x >> 8, x, (x+w-1) >> 8, (x+w-1) };
	uint8_t yy[4] = { y >> 8, y, (y+h-1) >> 8, (y+h-1) };
	int len = w * h;
	ili9341_spi_wait_bulk();
	send_command(0x2A, 4, xx);
	send_command(0x2B, 4, yy);
	send_command(0x2C, 0, NULL);

	constexpr int chunkSize = 512;
	static_assert(chunkSize <= ili9341_bufferSize);

	for(int i=0; i<chunkSize; i++)
		ili9341_spi_buffer[i] = color;

	while(len > chunkSize) {
		ili9341_spi_transfer_bulk(chunkSize);
		len -= chunkSize;
	}
	while (len-- > 0) 
	  ssp_senddata16(color);
}

void ili9341_bulk(int x, int y, int w, int h)
{
	uint8_t xx[4] = { x >> 8, x, (x+w-1) >> 8, (x+w-1) };
	uint8_t yy[4] = { y >> 8, y, (y+h-1) >> 8, (y+h-1) };
	int len = w * h;

	ili9341_spi_wait_bulk();

    delayMicroseconds(10);

	send_command(0x2A, 4, xx);
	send_command(0x2B, 4, yy);
	send_command(0x2C, 0, NULL);

	ili9341_spi_transfer_bulk(len);
	
	// switch buffers so that the user can continue to render while
	// the bulk transfer is happening.
	if(ili9341_spi_buffer == ili9341_spi_bufferA)
		ili9341_spi_buffer = ili9341_spi_bufferB;
	else ili9341_spi_buffer = ili9341_spi_bufferA;
}

void
ili9341_read_memory_raw(uint8_t cmd, int len, uint16_t* out)
{
	uint8_t r, g, b;
	ili9341_spi_wait_bulk();
	send_command(cmd, 0, NULL);

	// require 8bit dummy clock
	r = ssp_sendrecvdata(0);

	while (len-- > 0) {
		// read data is always 18bit
		r = ssp_sendrecvdata(0);
		g = ssp_sendrecvdata(0);
		b = ssp_sendrecvdata(0);
		*out++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
	}

	CS_HIGH;
}

void
ili9341_read_memory(int x, int y, int w, int h, int len, uint16_t *out)
{
	uint8_t xx[4] = { x >> 8, x, (x+w-1) >> 8, (x+w-1) };
	uint8_t yy[4] = { y >> 8, y, (y+h-1) >> 8, (y+h-1) };
	ili9341_spi_wait_bulk();
	
	send_command(0x2A, 4, xx);
	send_command(0x2B, 4, yy);

	ili9341_read_memory_raw(0x2E, len, out);
}

void
ili9341_read_memory_continue(int len, uint16_t* out)
{
	ili9341_read_memory_raw(0x3E, len, out);
}


void
ili9341_set_flip(bool flipX, bool flipY) {
	ili9341_spi_wait_bulk();
	uint8_t memAcc = 0b00101000;
	if(flipX) memAcc |= 0b01000000;
	if(flipY) memAcc |= 0b10000000;
	send_command(0x36, 1, &memAcc);
}

void
ili9341_drawchar_5x7(uint8_t ch, int x, int y, uint16_t fg, uint16_t bg)
{
  uint16_t *buf = ili9341_spi_buffer;
  uint8_t bits;
  int c, r;
  for(c = 0; c < 7; c++) {
	bits = x5x7_bits[(ch * 7) + c];
	for (r = 0; r < 5; r++) {
	  *buf++ = (0x80 & bits) ? fg : bg;
	  bits <<= 1;
	}
  }
  ili9341_bulk(x, y, 5, 7);
}

void
ili9341_drawstring_5x7_inv(const char *str, int x, int y, uint16_t fg, uint16_t bg, bool invert)
{
  if (invert)
    ili9341_drawstring_5x7(str, x, y, bg, fg);
  else
    ili9341_drawstring_5x7(str, x, y, fg, bg);
}

void
ili9341_drawstring_5x7(const char *str, int x, int y, uint16_t fg, uint16_t bg)
{
  while (*str) {
	ili9341_drawchar_5x7(*str, x, y, fg, bg);
	x += 5;
	str++;
  }
}


void
ili9341_drawstring_5x7(const char *str, int len, int x, int y, uint16_t fg, uint16_t bg)
{
  const char* end = str + len;
  while (str < end) {
	ili9341_drawchar_5x7(*str, x, y, fg, bg);
	x += 5;
	str++;
  }
}

void
ili9341_drawchar_size(uint8_t ch, int x, int y, uint16_t fg, uint16_t bg, uint8_t size)
{
  uint16_t *buf = ili9341_spi_buffer;
  uint8_t bits;
  int c, r;
  for(c = 0; c < 7*size; c++) {
	bits = x5x7_bits[(ch * 7) + (c / size)];
	for (r = 0; r < 5*size; r++) {
	  *buf++ = (0x80 & bits) ? fg : bg;
	  if (r % size == (size-1)) {
		  bits <<= 1;
	  }
	}
  }
  ili9341_bulk(x, y, 5*size, 7*size);
}

void
ili9341_drawstring_size(const char *str, int x, int y, uint16_t fg, uint16_t bg, uint8_t size)
{
  int origX = x;
  while (*str) {
	if((*str) == '\n') {
        x = origX;
        y += 7 * size;
    } else {
        ili9341_drawchar_size(*str, x, y, fg, bg, size);
        x += 5 * size;
    }
    str++;
  }
}

#define SWAP(x,y) do { int z=x; x = y; y = z; } while(0)

void
ili9341_line(int x0, int y0, int x1, int y1, int fg)
{
  if (x0 > x1) {
	SWAP(x0, x1);
	SWAP(y0, y1);
  }

  while (x0 <= x1) {
	int dx = x1 - x0 + 1;
	int dy = y1 - y0;
	if (dy >= 0) {
	  dy++;
	  if (dy > dx) {
		dy /= dx; dx = 1;
	  } else {
		dx /= dy; dy = 1;
	  }
	} else {
	  dy--;
	  if (-dy > dx) {
		dy /= dx; dx = 1;
	  } else {
		dx /= -dy; dy = -1;
	  }
	}
	if (dy > 0)
	  ili9341_fill(x0, y0, dx, dy, fg);
	else
	  ili9341_fill(x0, y0+dy, dx, -dy, fg);
	x0 += dx;
	y0 += dy;
  }
}


const font_t NF20x22 = { 20, 22, 1, 3*22, (const uint8_t *)numfont20x22 };

void
ili9341_drawfont(uint8_t ch, const font_t *font, int x, int y, uint16_t fg, uint16_t bg)
{
	uint16_t *buf = ili9341_spi_buffer;
	const uint8_t *bitmap = &font->bitmap[font->slide * ch];
	int c, r;

	for (c = 0; c < font->height; c++) {
		uint8_t bits = *bitmap++;
		uint8_t m = 0x80;
		for (r = 0; r < font->width; r++) {
			*buf++ = (bits & m) ? fg : bg;
			m >>= 1;

			if (m == 0) {
				bits = *bitmap++;
				m = 0x80;
			}
		}
	}
	ili9341_bulk(x, y, font->width, font->height);
}

#if 1
const uint16_t colormap[] = {
  RGB565(255,0,0), RGB565(0,255,0), RGB565(0,0,255),
  RGB565(255,255,0), RGB565(0,255,255), RGB565(255,0,255)
};

void
ili9341_test(int mode)
{
  int x, y;
  int i;
  switch (mode) {
  default:
#if 1
	ili9341_fill(0, 0, 320, 240, 0);
	for (y = 0; y < 240; y++) {
	  ili9341_fill(0, y, 320, 1, RGB565(y, (y + 120) % 256, 240-y));
	}
	break;
  case 1:
	ili9341_fill(0, 0, 320, 240, 0);
	for (y = 0; y < 240; y++) {
	  for (x = 0; x < 320; x++) {
		ili9341_pixel(x, y, (y<<8)|x);
	  }
	}
	break;
  case 2:
	//send_command16(0x55, 0xff00);
	ili9341_pixel(64, 64, 0xaa55);
	break;
#endif
#if 1
  case 3:
	for (i = 0; i < 10; i++)
	  ili9341_drawfont(i, &NF20x22, i*20, 120, colormap[i%6], 0x0000);
	break;
#endif
#if 0
  case 4:
	draw_grid(10, 8, 29, 29, 15, 0, 0xffff, 0);
	break;
#endif
  case 4:
	ili9341_line(0, 0, 15, 100, 0xffff);
	ili9341_line(0, 0, 100, 100, 0xffff);
	ili9341_line(0, 15, 100, 0, 0xffff);
	ili9341_line(0, 100, 100, 0, 0xffff);
	break;
  }
}
#endif
