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
#define HEADER_BYTE_1 0xec
//
// each byte of the file is an atomic 'code' that independently encodes
// 8-71 compressed (same color) pixels or 7 uncompressed pixels
//
// the maximum length of run we can store in a single byte of output
#define MAX_RUN 71
//
// 3 basic encoding blocks:
// A: 0CLLLLLL (C = color, L = run length (8-71)
// B: 10PPPPPP (P = 6 non-repeating pixels)
// C: 11CRRRrr (RRR = run 1-8, rr = opposite color, run 1-4)
#define CHUNK_MASK 0xc0
#define CHUNK_LONG 0x00
#define CHUNK_LONG2 0x40
#define CHUNK_SHORT 0xc0
#define CHUNK_UNCOMP 0x80

typedef struct tagisbicimage
{
  uint8_t *pImage; // source or destination image
  int iWidth, iHeight, iPitch; // image dimensions and bytes per line
  int x, y; // current encoder or decoder position
  int len, len2; // remaining length of current run(s) being decoded
  uint8_t ucColor; // current color
  uint8_t ucChunk; // current chunk being decoded
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
