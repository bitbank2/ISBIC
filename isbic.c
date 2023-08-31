//
// ISBIC - Incredibly Simple Bitonal Image Compression
// Copyright (c) 2023 BitBank Software, Inc.
// written by Larry Bank
// project started 30/8/2023
//
#include "isbic.h"

//
// Read a single pixel and advance the x,y values
//
static uint8_t isbicRead1Pixel(ISBICIMG *pIMG)
{
uint8_t uc, *s;

	if (pIMG->y >= pIMG->iHeight) return 0; // allow reads past end of image
        s = pIMG->pImage;
        s += (pIMG->x >> 3) + (pIMG->y * pIMG->iPitch);
        uc = (s[0] & (0x80 >> (pIMG->x & 7))); // return in lowest bit position
        // advance X,Y
        pIMG->x++;
        if (pIMG->x == pIMG->iWidth) {
		pIMG->x = 0;
                pIMG->y++;
        }
        return (uc != 0);
} /* isbicRead1Pixel() */

//
// Count the length of the current run of pixels
// It can be 1
//
static int isbicGetRun(ISBICIMG *pIMG)
{
int iRun = 0;
int x, y; // keep original x/y in case we get a short run
uint8_t ucColor;

        x = pIMG->x; y = pIMG->y; // keep starting point
	pIMG->ucColor = ucColor = isbicRead1Pixel(pIMG); // starting color to match
        while (iRun < MAX_RUN && pIMG->ucColor == ucColor && pIMG->y < pIMG->iHeight) {
		iRun++;
		ucColor = isbicRead1Pixel(pIMG);
        }
        if (iRun < 8) { // no compression, reset x,y to start
		pIMG->x = x; pIMG->y = y;
        } else { // a good run, but we still need to back up 1 pixel
		pIMG->x--;
                if (pIMG->x < 0) {
			pIMG->x = pIMG->iWidth-1;
			if (pIMG->y != pIMG->iHeight) // don't decrement the last pixel or it will loop forever
				pIMG->y--;
		}
        }
return iRun;
} /* isbicGetRun() */

//
// Compress a bitonal image into ISBIC format
// returns the number of bytes of compressed data
//
int isbicCompress(uint8_t *pDest, ISBICIMG *pIMG)
{
int iOutSize = 0;
int iRun;
uint8_t *d, uc;

	d = pDest;
	d[0] = HEADER_BYTE_0;
	d[1] = HEADER_BYTE_1;
	d[2] = (uint8_t)pIMG->iWidth;
	d[3] = (uint8_t)(pIMG->iWidth >> 8);
	d[4] = (uint8_t)pIMG->iHeight;
	d[5] = (uint8_t)(pIMG->iHeight >> 8);
	d += 6;
        pIMG->x = pIMG->y = 0;
	while (pIMG->x < pIMG->iWidth && pIMG->y < pIMG->iHeight) { // encode loop
		iRun = isbicGetRun(pIMG);  
                if (iRun < 8) { // no compression, encode the next 7 pixels
                        uc = 0;
                        for (int i=6; i>=0; i--) {
                            uc |= (isbicRead1Pixel(pIMG) << i);
			}
                	*d++ = (0x80 | uc);
                } else { // encode the run
                	uc = (pIMG->ucColor << 6); // high bit is color
                        uc |= (iRun - 8);
                        *d++ = uc;
                }
	} // while encoding
	iOutSize = (int)(d - pDest);
return iOutSize;
} /* isbicCompress() */

void isbicDecompress(uint8_t *pSrc, int iLen, uint8_t *pDest, int iWidth, int iHeight, int iPitch)
{
} /* isbicDecompress() */

//
// Initialize the decode process
// returns 1 for success, 0 for failure
//
int isbicDecodeInit(ISBICIMG *pIMG, uint8_t *pSrc)
{
uint8_t uc, *s;

	s = pSrc;
	if (!s) return 0;
	if (s[0] != HEADER_BYTE_0 || s[1] != HEADER_BYTE_1) return 0; // not an ISBIC file
	pIMG->iWidth = (s[2] | ((uint16_t)s[3] << 8));
	pIMG->iHeight = (s[4] | ((uint16_t)s[5] << 8));
	uc = s[6]; // first compressed byte
	pIMG->ucColor = uc; // keep the current data byte here for decoding
	pIMG->pImage = &s[7]; // next 1 byte code
	pIMG->x = pIMG->y = 0; // destination x,y
	// Set initial repeat/non-repeat count
	if (uc & 0x80) // non-repeating
		pIMG->len = 7;
	else
		pIMG->len = (uc & 0x3f) + 8;
	return 1;
} /* isbicDecodeInit() */

//
// Decode a single pixel from a compressed stream
// returns 0 or 1. 0 is also returned when the decoder is in an invalid state
//
uint8_t isbicDecode1Pixel(ISBICIMG *pIMG)
{
uint8_t uc;

	if (!pIMG) return 0;
	if (pIMG->y >= pIMG->iHeight) return 0; // past bottom
	if (pIMG->len == 0) { // get the next data chunk
		pIMG->ucColor = *pIMG->pImage++;
		if (pIMG->ucColor & 0x80) // uncompressed
			pIMG->len = 7;
		else
			pIMG->len = 8 + (pIMG->ucColor & 0x3f);
	}
	if (pIMG->ucColor & 0x80) { // last data chunk was uncompressed
		uc = (pIMG->ucColor >> (pIMG->len-1)) & 1; 
	} else { // compressed has the color in bit 6
		uc = (pIMG->ucColor >> 6);
	}
	pIMG->len--;
	// increment the current pixel
	pIMG->x++;
	if (pIMG->x >= pIMG->iWidth) {
		pIMG->x = 0;
		pIMG->y++;
	}
	return uc;
} /* isbicDecode1Pixel() */

