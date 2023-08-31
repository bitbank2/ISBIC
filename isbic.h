//
// ISBIC - Incredibly Simple Bitonal Image Compression
// Copyright (c) 2023 BitBank Software, Inc.
// written by Larry Bank
// project started 30/8/2023
//
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//
// File Header
// The ISBIC file header consists of 6 bytes:
// 2 byte identifier - 0xfe 0xed
// 2 byte width in little endian order
// 2 byte height in little endian order
//
#define HEADER_BYTE_0 0xfe
#define HEADER_BYTE_1 0xed
//
// each byte of the file is an atomic 'code' that independently encodes
// 8-71 compressed (same color) pixels or 7 uncompressed pixels
//
// the maximum length of run we can store in a single byte of output
#define MAX_RUN 71
//
// 2 basic encoding blocks:
// A: 0CLLLLLL (C = color, L = run length (8-71)
// B: 1PPPPPPP (P = 7 non-repeating pixels)
//

typedef struct tagisbicimage
{
  uint8_t *pImage; // source or destination image
  int iWidth, iHeight, iPitch; // image dimensions and bytes per line
  int x, y; // current encoder or decoder position
  int len; // remaining length of current run being decoded
  uint8_t ucColor; // current color
} ISBICIMG;
//
// Public functions
//
//
// Compress a bitonal image into ISBIC format
// The ISBICIMG structure must contain the correct image dimensions
// source pointer and bytes per line (pitch) values
//
// returns the number of bytes of compressed data
// pDest must be large enough to hold the output data
//
int isbicCompress(uint8_t *pDest, ISBICIMG *pIMG);
//
// Initialize the decode process
//
int isbicDecodeInit(ISBICIMG *pIMG, uint8_t *pSrc);
//
// Decode a single pixel from a compressed stream
//
uint8_t isbicDecode1Pixel(ISBICIMG *pIMG);
