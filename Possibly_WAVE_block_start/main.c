/*
 * main.c
 * Hacking Fahrenheit game data file program.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 11.08.2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif

/* DVI ADPCM block header
 * Size of this structure MUST be 4.
 */
struct BlockHeader
{
  short sample;  /* First sample */
  char index;    /* Start index */
  unsigned char reserved; /* Reserved byte */
};

/* The main program function.
 * ARGUMENTS:
 *   - number of command line arguments:
 *       int argc;
 *   - array of command line arguments:
 *       char *argv[];
 * RETURNS:
 *   (int) program exit status.
 */
int main( int argc, char *argv[] )
{
  char *file_name = "data";
  long offset = 0;
  FILE *fp;
  struct BlockHeader header;

  assert(sizeof(struct BlockHeader) == 4);

  if (argc == 1)
  {
    printf(
      "Usage:\n"
      "  %s file_name [ offset ]\n", argv[0]);
    return 0;
  }
  if (argc > 1)
    file_name = argv[1];
  if (argc > 2)
    sscanf(argv[2], "%li", &offset);
    
  if ((fp = fopen(file_name, "rb")) == NULL)
  {
    perror("fopen");
    return 1;
  }
  if (fseek(fp, offset, SEEK_SET))
  {
    perror("fseek");
    return 1;
  }
  
  while (!feof(fp))
  {
    if (fread(&header, 4, 1, fp) != 1)
    {
      printf("EOF reached.\n");
      break;
    }
    
    if (header.index >= 0 && header.index <= 88 && header.reserved == 0)
    {
      printf("0x%04X%04X %8li s: %6d, i: %2d, r: 0x%02X\n",
        (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF), offset,
        header.sample, header.index, (unsigned int)header.reserved);
    }
    offset += 4;
  }

  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
