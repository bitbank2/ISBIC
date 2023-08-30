//
// ISBIC - Incredibly Simple Bitonal Image Compression
// by Larry Bank
// project started 30/8/2023
//
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct tagisbicimage
{
  uint8_t *pImage;
  int iWidth, iHeight, iPitch;
  int x, y;
  uint8_t ucColor; // current color
} ISBICIMG;
// the maximum length of run we can store in a single byte of output
#define MAX_RUN 71
//
// 2 basic encoding blocks:
// A: 0CLLLLLL (C = color, L = run length (8-71)
// B: 1PPPPPPP (P = 7 non-repeating pixels)
//

//
// Read a single pixel and advance the x,y values
//
uint8_t isbicRead1Pixel(ISBICIMG *pIMG)
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
int isbicGetRun(ISBICIMG *pIMG)
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

printf("entering isbicCompress, width=%d, height=%d, pitch=%d\n", pIMG->iWidth, pIMG->iHeight, pIMG->iPitch);
	d = pDest;
        pIMG->x = pIMG->y = 0;
	while (pIMG->x < pIMG->iWidth && pIMG->y < pIMG->iHeight) { // encode loop
//printf("x/y = %d,%d, iRun = ", pIMG->x, pIMG->y);
		iRun = isbicGetRun(pIMG);  
//printf("%d, new x,y = %d, %d\n", iRun, pIMG->x, pIMG->y);
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
// Read a Windows BMP file into memory
//  
uint8_t * ReadBMP(const char *fname, int *width, int *height, int *bpp, unsigned char *pPal) 
{  
    int y, w, h, bits, offset; 
    uint8_t *s, *d, *pTemp, *pBitmap;
    int pitch, bytewidth;
    int iSize, iDelta, iColorsUsed;
    FILE *infile;
   
    infile = fopen(fname, "r+b");
    if (infile == NULL) {  
        printf("Error opening input file %s\n", fname);
        return NULL;
    }
    // Read the bitmap into RAM
    fseek(infile, 0, SEEK_END);
    iSize = (int)ftell(infile);
    fseek(infile, 0, SEEK_SET);
    pBitmap = (uint8_t *)malloc(iSize);
    pTemp = (uint8_t *)malloc(iSize);
    fread(pTemp, 1, iSize, infile);
    fclose(infile);
  
    if (pTemp[0] != 'B' || pTemp[1] != 'M' || pTemp[14] < 0x28) {
        free(pBitmap);
        free(pTemp);
        printf("Not a Windows BMP file!, size = %d, pTemp[14] = 0x%02x\n", iSize, pTemp[14]);
        return NULL;
    }
    iColorsUsed = pTemp[46];
    w = *(int32_t *)&pTemp[18];
    h = *(int32_t *)&pTemp[22];
    bits = *(int16_t *)&pTemp[26] * *(int16_t *)&pTemp[28];
    if (iColorsUsed == 0 || iColorsUsed > (1<<bits))
        iColorsUsed = (1 << bits); // full palette

    if (bits <= 8 && pPal != NULL) { // it has a palette, copy it
        uint8_t *p = pPal;
        int iOff = pTemp[10] + (pTemp[11] << 8);
        iOff -= (iColorsUsed*4);
        for (int i=0; i<iColorsUsed; i++)
        {
           *p++ = pTemp[iOff+2+i*4];
           *p++ = pTemp[iOff+1+i*4];
           *p++ = pTemp[iOff+i*4];
        }
    }
    offset = *(int32_t *)&pTemp[10]; // offset to bits
    bytewidth = (w * bits) >> 3;
    pitch = (bytewidth + 3) & 0xfffc; // DWORD aligned
// move up the pixels
    d = pBitmap;
    s = &pTemp[offset];
    iDelta = pitch;
    if (h > 0) {
        iDelta = -pitch;
        s = &pTemp[offset + (h-1) * pitch];
    } else {
        h = -h;
    }
    for (y=0; y<h; y++) {
        if (bits == 32) {// need to swap red and blue
            for (int i=0; i<bytewidth; i+=4) {
                d[i] = s[i+2];
                d[i+1] = s[i+1];
                d[i+2] = s[i];
                d[i+3] = s[i+3];
            }
        } else {
            memcpy(d, s, bytewidth);
        }
        d += bytewidth;
        s += iDelta;
    }
    *width = w;
    *height = h;
    *bpp = bits;
    free(pTemp);
    return pBitmap;

} /* ReadBMP() */

int main(int argc, const char * argv[]) {
    int iDataSize, iBpp;
    //FILE *ohandle;
    uint8_t *pOutput;
    int iWidth, iHeight;
    uint8_t *pBitmap;
    ISBICIMG state;
  
    if (argc != 3 && argc != 2) {
       printf("ISBIC codec library demo program\n");
       printf("Usage: isbic_demo <infile> <outfile>\n");
       printf("The input and output files can be either WinBMP (*.bmp) or ISBIC (*.isb)\n");
       return 0;
    }

    pBitmap = ReadBMP(argv[1], &iWidth, &iHeight, &iBpp, NULL);
       if (pBitmap == NULL)
       {
           fprintf(stderr, "Unable to open file: %s\n", argv[1]);
           return -1; // bad filename passed?
       }
        if (iBpp != 1) {
	   fprintf(stderr, "Only 1-bpp images are supported\n");
           return -1;
        }
        state.iWidth = iWidth;
        state.iHeight = iHeight;
        state.pImage = pBitmap;
        state.iPitch = (iWidth + 7) >> 3;
    pOutput = malloc(iWidth * iHeight); // output buffer
    // DEBUG
    {
	int iW=0, iB=0;
	int i, iDataSize;
	iDataSize = state.iPitch * state.iHeight;
	for (i=0; i<iDataSize; i++) {
		if (pBitmap[i] == 0) iB++;
		else if (pBitmap[i] == 0xff) iW++;
	}
	printf("0x00 = %d, 0xff = %d, total = %d\n", iB, iW, iDataSize);
    }
    printf("Compressing a %d x %d bitmap as ISBIC data\n", iWidth, iHeight);
    iDataSize = isbicCompress(pOutput, &state);
        printf("ISBIC image successfully created. %d bytes = %d:1 compression\n", iDataSize, (state.iPitch*iHeight) / iDataSize);
//        ohandle = fopen(argv[iOutIndex], "w+b");
//        if (ohandle != NULL) {
//            fwrite(pOutput, 1, iDataSize, ohandle);
//            fclose(ohandle);
//        }
        free(pOutput);
    free(pBitmap);
    return 0;
} /* main() */

