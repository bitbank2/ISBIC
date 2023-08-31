//
// ISBIC - Incredibly Simple Bitonal Image Compression
// Copyright (c) 2023 BitBank Software, Inc.
// written by Larry Bank
// project started 30/8/2023
//
#include "isbic.h"

//
// Write a Windows BMP file header
// returns the pitch needed when writing the pixel data
//
int WriteBMP(FILE *outfile, int width, int height, int bpp)
{
    int lsize, i, iHeaderSize, iBodySize;
    uint8_t pBuf[1024]; // holds BMP header
 
    lsize = ((width * bpp) + 7)/8;
    lsize = (lsize + 3) & 0xfffc; // DWORD aligned
    iHeaderSize = 54;
    iHeaderSize += (1<<(bpp+2)); // palette size
    iBodySize = lsize * height;
    i = iBodySize + iHeaderSize; // datasize
    memset(pBuf, 0, 54);
    pBuf[0] = 'B';
    pBuf[1] = 'M';
    pBuf[2] = i & 0xff;     // 4 bytes of file size
    pBuf[3] = (i >> 8) & 0xff;
    pBuf[4] = (i >> 16) & 0xff;
    pBuf[5] = (i >> 24) & 0xff;
    /* Offset to data bits */
    pBuf[10] = iHeaderSize & 0xff;
    pBuf[11] = (unsigned char)(iHeaderSize >> 8);
    pBuf[14] = 0x28;
    pBuf[18] = width & 0xff; // xsize low
    pBuf[19] = (unsigned char)(width >> 8); // xsize high
    i = 0-height; // top down bitmap
    pBuf[22] = i & 0xff; // ysize low
    pBuf[23] = (unsigned char)(i >> 8); // ysize high
    pBuf[24] = 0xff;
    pBuf[25] = 0xff;
    pBuf[26] = 1; // number of planes
    pBuf[28] = (uint8_t)bpp;
    pBuf[30] = 0; // uncompressed
    i = iBodySize;
    pBuf[34] = i & 0xff;  // data size
    pBuf[35] = (i >> 8) & 0xff;
    pBuf[36] = (i >> 16) & 0xff;
    pBuf[37] = (i >> 24) & 0xff;
    pBuf[54] = pBuf[55] = pBuf[56] = pBuf[57] = pBuf[61] = 0; // palette
    pBuf[58] = pBuf[59] = pBuf[60] = 0xff;
    fwrite(pBuf, 1, iHeaderSize, outfile);
    return lsize;
} /* WriteBMP() */

//
// Read a Windows BMP file into memory
//  
uint8_t * ReadBMP(const char *fname, int *width, int *height, int *bpp, unsigned char *pPal) 
{  
    int y, w, h, bits, offset; 
    uint8_t *s, *d, *pTemp, *pBitmap;
    int pitch, bytewidth;
    int iSize, iDelta, iColorsUsed;
    FILE *thefile;
   
    thefile = fopen(fname, "r+b");
    if (thefile == NULL) {  
        printf("Error opening input file %s\n", fname);
        return NULL;
    }
    // Read the bitmap into RAM
    fseek(thefile, 0, SEEK_END);
    iSize = (int)ftell(thefile);
    fseek(thefile, 0, SEEK_SET);
    pBitmap = (uint8_t *)malloc(iSize);
    pTemp = (uint8_t *)malloc(iSize);
    fread(pTemp, 1, iSize, thefile);
    fclose(thefile);
  
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
    char *s;
    int i, x, y, iDataSize, iBpp;
    FILE *thefile;
    uint8_t *pOutput;
    int iWidth, iHeight;
    uint8_t *pTemp, *pBitmap;
    ISBICIMG state;
  
    if (argc != 3) {
       printf("ISBIC codec library demo program\n");
       printf("Usage: isbic_demo <infile> <outfile>\n");
       printf("The input and output files can be either WinBMP (*.bmp) or ISBIC (*.isb)\n");
       return 0;
    }
    s = (char *)argv[1];
    i = strlen(s);
    if (memcmp(&s[i-4], ".BMP", 4) == 0 || memcmp(&s[i-4], ".bmp", 4) == 0) { // convert from BMP to ISBIC
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
    printf("Compressing a %d x %d bitmap as ISBIC data\n", iWidth, iHeight);
    iDataSize = isbicCompress(pOutput, &state);
        printf("ISBIC image successfully created. %d bytes = %d:1 compression\n", iDataSize, (state.iPitch*iHeight) / iDataSize);
        thefile = fopen(argv[2], "w+b");
        if (thefile != NULL) {
            fwrite(pOutput, 1, iDataSize, thefile);
            fclose(thefile);
        } else {
		printf("Error opening file %s\n", argv[2]);
	}
        free(pOutput);
    free(pBitmap);
    } else { // convert from ISBIC to BMP
	printf("Converting from ISBIC to BMP\n");
    thefile = fopen(argv[1], "r+b");
    if (thefile == NULL) {   
        printf("Error opening input file %s\n", argv[1]);
        return -1; 
    }
    // Read the file into RAM
    fseek(thefile, 0, SEEK_END);
    iDataSize = (int)ftell(thefile);
    fseek(thefile, 0, SEEK_SET);
    pTemp = (uint8_t *)malloc(iDataSize);
    fread(pTemp, 1, iDataSize, thefile);
    fclose(thefile);
	if (isbicDecodeInit(&state, pTemp) == 0) {
		printf("The ISBIC file has a problem...aborting.\n");
		return -1;
	}
	thefile = fopen(argv[2], "w+b");
	if (thefile == NULL) {
		printf("Error creating output file %s\n", argv[2]);
		return -1;
	}
        printf("Creating %d x %d 1bpp Windows BMP file\n", state.iWidth, state.iHeight);
        i = WriteBMP(thefile, state.iWidth, state.iHeight, 1);
	pBitmap = malloc(i); // allocate memory to hold each bitmap line
	for (y=0; y<state.iHeight; y++) {
		memset(pBitmap, 0, i);
		for (x=0; x<state.iWidth; x++) {
			if (isbicDecode1Pixel(&state))
				pBitmap[x>>3] |= 0x80 >> (x & 7);
		} // for x
		fwrite(pBitmap, 1, i, thefile);
	} // for y
	free(pBitmap);
	free(pTemp);
	fclose(thefile);
    } // ISBIC to BMP
    return 0;
} /* main() */

