/*
 * adpcm.c
 * Hacking Fahrenheit music program.
 * DVI ADPCM decoding module.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 11.08.2006
 */

#include "adpcm.h"

#include <stdio.h>
#include <stdlib.h>

#define DEBUG
#define DEBUG_EOF
#define DEBUG_BLOCK_HEADER
//#define DEBUG_DECODED_SAMPLES
#define DEBUG_INCORRECT_HEADER

/* DVI ADPCM step table */
const int StepTab[89]  = {
                    7,     8,     9,    10,    11,    12,    13,    14,
                   16,    17,    19,    21,    23,    25,    28,    31,
                   34,    37,    41,    45,    50,    55,    60,    66,
                   73,    80,    88,    97,   107,   118,   130,   143,
                  157,   173,   190,   209,   230,   253,   279,   307,
                  337,   371,   408,   449,   494,   544,   598,   658,
                  724,   796,   876,   963,  1060,  1166,  1282,  1411,
                 1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
                 3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
                 7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
                15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
                32767 };

/* 4 bit DVI ADPCM index table */
const int IndexTab4[16] = { -1, -1, -1, -1, 2, 4, 6, 8,
                            -1, -1, -1, -1, 2, 4, 6, 8 };

/* 3 bit DVI ADPCM index table */
const int IndexTab3[8] = { -1, -1, 1, 2,
                           -1, -1, 1, 2 };

/* Initializing WAVE data structure function
 * ARGUMENTS:
 *   - filled WAVEData structure (only 'buffer' and 'buffer_len' shouldn't be initialized):
 *       WAVEData *wd;
 * RETURNS:
 *   (int) zero on success, non-zero otherwise.
 */
int init_WAVE( WAVEData *wd )
{
  /* Allocating memory for decoded data buffer */
  if ((wd->buffer = malloc(sizeof(int) * wd->samples_per_block * wd->nchannels)) == NULL)
  {
    perror("malloc");
    return 1;
  }
  if (decode_next_block(wd))
    return 1;

  return 0;
} /* End of 'init_WAVE' function */

/* Deinitializing WAVE data structure function */
void deinit_WAVE( WAVEData *wd )
{
  if (wd->buffer != NULL)
    free(wd->buffer);
} /* End of 'deinit_WAVE' function */

/* Decoding one block from compressed WAVE file function */
int decode_next_block( WAVEData *wd )
{
  struct BlockHeader header; /* Header */
  long sample;               /* Decoded sample */
  int index;                 /* Current index */
  int samples_count = 0;     /* Number of proceeded samples */
  int bnum;
  int channel, is_eof;
  long start_offset;
#ifdef DEBUG_BLOCK_HEADER
  long header_offset;
#endif /* DEBUG_BLOCK_HEADER */

  wd->buffer_len = 0;
      
  if ((start_offset = ftell(wd->fp)) < 0)
  {
    perror("ftell");
    return 1;
  }

  for (channel = 0; channel < wd->nchannels; channel++)
  {
    samples_count = 0;
    is_eof = 0;
  
    /* Seeking to header */
    if (fseek(wd->fp, start_offset + 4 * channel, SEEK_SET))
    {
      perror("ftell");
      return 1;
    }

#ifdef DEBUG_BLOCK_HEADER
    if ((header_offset = ftell(wd->fp)) < 0)
    {
      perror("ftell");
      return 1;
    }
#endif /* DEBUG_BLOCK_HEADER */
  
    /* Reading block header */
    if (fread(&header, 4, 1, wd->fp) != 1)
    {
      wd->buffer_len = 0;
#ifdef DEBUG_EOF
      printf("EOF in block header reached.\n");
#endif /* DEBUG_EOF */
      return 0;
    }
    //fwrite(&header, sizeof(header), 1, stderr);
    
    /* Parsing readed header */
    sample = header.sample;
    index = header.index;
#ifdef DEBUG_BLOCK_HEADER
    printf("0x%04X%04X s: %li, i: %d ch: %i\n", (int)((header_offset >> 16) & 0xFFFF), 
      (int)(header_offset & 0xFFFF), sample, index, channel);
#endif /* DEBUG_BLOCK_HEADER */

    /* Values should be correct but... */
    if (index > 88)
    {
#ifdef DEBUG_INCORRECT_HEADER
      exit(1);
#endif /* DEBUG_INCORRECT_HEADER */
      index = 88;
    }
    else if (index < 0)
    {
#ifdef DEBUG_INCORRECT_HEADER
      exit(1);
#endif /* DEBUG_INCORRECT_HEADER */
      index = 0;
    }
      
    wd->buffer[samples_count * wd->nchannels + channel] = sample;
    //fwrite(&wd->buffer[samples_count * wd->nchannels + channel], 2, 1, stderr);
    /* printf("    0x%04X%04X\n", 
        (unsigned int)(((samples_count * wd->nchannels + channel) >> 16) & 0xFFFF),
        (unsigned int)((samples_count * wd->nchannels + channel) & 0xFFFF)); */
    samples_count++;

    while (samples_count < wd->samples_per_block && !is_eof)
    {
      if (fseek(wd->fp, 4 * (wd->nchannels - 1), SEEK_CUR))
      {
        perror("ftell");
        return 1;
      }
      /* printf("  0x%04X%04X\n", 
        (unsigned int)((ftell(wd->fp) >> 16) & 0xFFFF),
        (unsigned int)(ftell(wd->fp) & 0xFFFF)); */
      
      /* Proceeding samples */
      for (bnum = 0; bnum < 4; bnum++)
      {
        int nsample, byte;
        
        if ((byte = fgetc(wd->fp)) == EOF)
        {
          wd->buffer_len = samples_count;
          is_eof = 1;
          break;
        }
        //fwrite(&byte, 1, 1, stderr);
        
        for (nsample = 0; nsample < 2; nsample++)
        {
          int diff;
          int sample_code;
    
          sample_code = (byte >> (4 * nsample)) & 0x0F;
          //sample_code = (byte >> (4 * (1 - nsample))) & 0x0F;

          /* Decoding */
          diff = 0;
          
          if (sample_code & 4)
            diff = diff + StepTab[index];
          if (sample_code & 2)
            diff = diff + (StepTab[index] >> 1);
          if (sample_code & 1)
            diff = diff + (StepTab[index] >> 2);
          diff = diff + (StepTab[index] >> 3);
        
          if (sample_code & 8)
            diff = -diff;
          sample += diff;
            
          if (sample > 32767)
          {
            //printf("Sample too large.\n");
            sample = 32767;
          }
          else if (sample < -32768)
          {
            //printf("Sample too small.\n");
            sample = -32768;
          }
                
          wd->buffer[samples_count * wd->nchannels + channel] = sample;
          //fwrite(&wd->buffer[samples_count * wd->nchannels + channel], 2, 1, stderr);
          /*printf("    0x%04X%04X\n", 
            (unsigned int)(((samples_count * wd->nchannels + channel) >> 16) & 0xFFFF),
            (unsigned int)((samples_count * wd->nchannels + channel) & 0xFFFF));*/
          samples_count++;
        
          /* Finishing decoding process */
          index += IndexTab4[sample_code];
          if (index > 88)
          {
            //printf("Index too large.\n");
            index = 88;
          }
          else if (index < 0)
          {
            //printf("Index too small.\n");
            index = 0;
          }
        }
      }
    }
    /*printf("  0x%04X%04X - e\n", 
      (unsigned int)((ftell(wd->fp) >> 16) & 0xFFFF),
      (unsigned int)(ftell(wd->fp) & 0xFFFF));*/
#ifdef DEBUG_DECODED_SAMPLES
    printf("%i total samples decoded.\n", samples_count);
#endif /* DEBUG_DECODED_SAMPLES */
  }
  
  wd->buffer_len = samples_count * wd->nchannels;

  return 0;
} /* End of 'decode_next_block' function */

/* END OF 'adpcm.c' FILE */
