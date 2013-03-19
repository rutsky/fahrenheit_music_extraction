/*
 * main.c
 * Hacking Fahrenheit music program.
 * DVI ADPCM decoding module header.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 10.08.2006
 */

#ifndef __adpcm_h__
#define __adpcm_h__

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

/* DVI ADPCM step table */
extern const int StepTab[89];

/* 4 bit DVI ADPCM index table */
extern const int IndexTab4[16];

/* 3 bit DVI ADPCM index table */
extern const int IndexTab3[8];

/* DVI ADPCM block header
 * Size of this structure MUST be 4.
 */
struct BlockHeader
{
  int16_t sample; /* First sample */
  char index;     /* Start index */
  char reserved;  /* Reserved byte */
};

/* WAVE file data structure definition */
typedef struct
{
  FILE *fp;              /* WAVE file pointer */
  int nchannels;         /* Number of audio channels */
  long samples_per_sec;  /* Sample rate of the WAVE file (8000, 11025, 22050, 44100...) */
  int bits_per_sample;   /* Number of bits per sample of data (3 or 4) */
  int block_align;       /* The number of bytes for one sample including all channels */
  int samples_per_block; /* Count of the number of samples per channel per block */
  int *buffer;           /* Decoded buffer */
  int buffer_len;        /* Decoded buffer length */
} WAVEData;

/* Initializing WAVE data structure function
 * ARGUMENTS:
 *   - filled WAVEData structure (only 'buffer' and 'buffer_len' shouldn't be initialized):
 *       WAVEData *wd;
 * RETURNS:
 *   (int) zero on success, non-zero otherwise.
 */
int init_WAVE( WAVEData *wd );

/* Deinitializing WAVE data structure function */
void deinit_WAVE( WAVEData *wd );

/* Decoding next block from compressed WAVE file function */
int decode_next_block( WAVEData *wd );

#endif /* __adpcm_h__ */

/* END OF 'adpcm.h' FILE */
