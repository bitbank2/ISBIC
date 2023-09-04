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
    uint8_t ucColor;
    
    pIMG->ucColor = ucColor = isbicRead1Pixel(pIMG); // starting color to match
    while (iRun < MAX_RUN && pIMG->ucColor == ucColor && pIMG->y < pIMG->iHeight) {
        iRun++;
        ucColor = isbicRead1Pixel(pIMG);
    }
    // back up over the last pixel since it's a different color
    pIMG->x--;
    if (pIMG->x < 0) {
        pIMG->x = pIMG->iWidth-1;
        if (pIMG->y != pIMG->iHeight) // don't decrement the last pixel or it will loop forever
            pIMG->y--;
    }
    return iRun;
} /* isbicGetRun() */

//
// Compress a bitonal image into ISBIC format
// returns the number of bytes of compressed data
//
int isbicCompress(uint8_t *pDest, ISBICIMG *pIMG)
{
int x1, y1, iOutSize = 0;
int iRun, iRun2;
uint8_t *d, uc, ucColor;

	d = pDest;
	d[0] = HEADER_BYTE_0;
	d[1] = HEADER_BYTE_1 | (pIMG->pImage[0] >> 7); // store first color in header
	d[2] = (uint8_t)pIMG->iWidth;
	d[3] = (uint8_t)(pIMG->iWidth >> 8);
	d[4] = (uint8_t)pIMG->iHeight;
	d[5] = (uint8_t)(pIMG->iHeight >> 8);
	d += 6;
    pIMG->x = pIMG->y = 0;
    while (pIMG->x < pIMG->iWidth && pIMG->y < pIMG->iHeight) { // encode loop
        x1 = pIMG->x; y1 = pIMG->y; // preserve start of all runs
        iRun = isbicGetRun(pIMG);
        ucColor = pIMG->ucColor; // color of this run
        if (iRun > 8) { // long run
            uc = (ucColor << 6); // high bit is color
            uc |= (iRun - 8);
            *d++ = uc | CHUNK_LONG;
        } else {
            iRun2 = isbicGetRun(pIMG);
            if (iRun + iRun2 < 7) { // not worth using RLE1/2
                pIMG->x = x1; pIMG->y = y1; // restore to start
                uc = 0;
                for (int i=5; i>=0; i--) { // store 6 pixels
                    uc |= (isbicRead1Pixel(pIMG) << i);
                } // for i
                *d++ = (CHUNK_UNCOMP | uc);
            } else { // we can use the short R1/R2 encoding
                if (iRun2 > 4) { // need to encode the next part in a different chunk
                    pIMG->x -= (iRun2 - 4); // excess run length
                    while (pIMG->x < 0) {
                        pIMG->x += pIMG->iWidth;
                        pIMG->y--;
                    }
                    iRun2 = 4;
                }
                *d++ = CHUNK_SHORT | (ucColor << 5) | ((iRun-1) << 2) | (iRun2-1);
            }
        } // short/uncomp chunk
    } // while encoding
	iOutSize = (int)(d - pDest);
return iOutSize;
} /* isbicCompress() */

//
// Initialize the decode process
// returns 1 for success, 0 for failure
//
int isbicDecodeInit(ISBICIMG *pIMG, uint8_t *pSrc)
{
uint8_t uc, *s;

	s = pSrc;
	if (!s) return 0;
	if (s[0] != HEADER_BYTE_0 || (s[1] & 0xfe) != HEADER_BYTE_1) return 0; // not an ISBIC file
	pIMG->iWidth = (s[2] | ((uint16_t)s[3] << 8));
	pIMG->iHeight = (s[4] | ((uint16_t)s[5] << 8));
	pIMG->ucChunk = uc = s[6]; // first compressed byte
	pIMG->ucColor = s[1] & 1; // first color is saved in the header byte
	pIMG->pImage = &s[7]; // next 1 byte code
	pIMG->x = pIMG->y = 0; // destination x,y
	// Set initial repeat/non-repeat count
    switch (uc & CHUNK_MASK) {
        case CHUNK_UNCOMP: // non-repeating
            pIMG->len = 6;
            pIMG->len2 = 0;
            break;
        case CHUNK_SHORT:
            pIMG->len = ((uc >> 2) & 7)+1;
            pIMG->len2 = (uc & 3) + 1;
            break;
        case CHUNK_LONG: // long run length
        case CHUNK_LONG2:
            pIMG->len = (uc & 0x3f) + 8;
            pIMG->len2 = 0;
            break;
    }
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
    if (pIMG->len == 0 && pIMG->len2 == 0) { // get the next data chunk
        uc = pIMG->ucChunk = *pIMG->pImage++;
        switch (uc & CHUNK_MASK) {
            case CHUNK_UNCOMP: // uncompressed
                pIMG->len = 6;
                break;
            case CHUNK_SHORT:
                pIMG->ucColor = (uc >> 5) & 1;
                pIMG->len = ((uc >> 2) & 7)+1;
                pIMG->len2 = (uc & 3) + 1;
                break;
            case CHUNK_LONG:
            case CHUNK_LONG2:
                pIMG->ucColor = (uc >> 6) & 1;
                pIMG->len = 8 + (uc & 0x3f);
                break;
        } // switch
    } // need to read the next compressed data chunk
    
    // Pull 1 pixel from the current chunk
	switch (pIMG->ucChunk & CHUNK_MASK) {
        case CHUNK_UNCOMP:
            uc = (pIMG->ucChunk >> (pIMG->len-1)) & 1;
            pIMG->len--;
            break;
            
        case CHUNK_SHORT:
            if (pIMG->len) {
                uc = pIMG->ucColor;
                pIMG->len--;
            } else { // second half is opposite color
                uc = 1-pIMG->ucColor;
                pIMG->len2--;
            }
            break;
            
        case CHUNK_LONG:
        case CHUNK_LONG2:
            uc = pIMG->ucColor;
            pIMG->len--;
            break;
	} // switch on chunk type
	// increment the current pixel
	pIMG->x++;
	if (pIMG->x >= pIMG->iWidth) {
		pIMG->x = 0;
		pIMG->y++;
	}
	return uc;
} /* isbicDecode1Pixel() */

