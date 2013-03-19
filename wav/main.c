/*
 * main.c
 * Hacking Fahrenheit game data file program.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 10.08.2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEEK 0x20
//#define START 0x0009
#define START 1000
//#define END 0x000A
#define END 0x0F00
//#define STEP 0xA0
#define STEP 1

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
  FILE *infp, *outfp;
  long pos;
  int i, ch;
  
  for (i = START; i < END; i += STEP)
  {
    printf("\n\n>>> %d <<<\n\n", i);
    if ((infp = fopen("t.wav", "rb")) == NULL)
    {
      perror("fopen");
      return 1;
    }
    if ((outfp = fopen("tmp.wav", "wb")) == NULL)
    {
      perror("fopen");
      fclose(infp);
      return 1;
    }
  
    for (pos = 0; pos < SEEK; pos++)
      fputc(fgetc(infp), outfp);
    
    fgetc(infp);
    fgetc(infp);
    fputc(i & 0xFF, outfp);
    fputc((i >> 8) & 0xFF, outfp);
    
    while ((ch = fgetc(infp)) != EOF)
      fputc(ch, outfp);
      
    fclose(infp);
    fclose(outfp);
    
    system("/usr/local/bin/mplayer tmp.wav");
  }
  
  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
