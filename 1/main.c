/*
 * main.c
 * Fahrehheit music hacking.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 14.07.2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER 0x200L
#define SEEK 0x15144L
#define XOR 0xD2
#define CR 10

/* The main program function */
int main( int argc, char *argv[] )
{
  FILE *infp, *outfp = stdout;
  long long i;
  unsigned int ch, low, high, olow = 0, ohigh = 0;
  
  if (argc < 2)
  {
    fprintf(stderr, "Pass file name as argument.\n");
    exit(1);
  }
    
  if ((infp = fopen(argv[1], "rb")) == NULL)
  {
    perror("fopen");
    exit(1);
  }
  
  /* Seeking */
  for (i = 0; i < HEADER; i++)
    if ((ch = fgetc(infp)) == EOF)
    {
      fprintf(stderr, "EOF reached at offset %lli.\n", i);
      exit(1);
    }
    else
      fputc(ch, outfp);
      
  fseek(infp, SEEK, SEEK_SET);
  
/*  for (j = 0; j < 127; j++) 
  {
    for (i = 0; i < 1000; i++)
    {
      fputc(0, outfp);
      fputc(127 - j, outfp);
      fputc(0, outfp);
      fputc(j + 127, outfp);
    }
  }
  exit(0);*/
      
  /* Filtering */
  i = 3;
  while ((low = fgetc(infp)) != EOF && (high = fgetc(infp)) != EOF)
  {
    /*if (i % 3)
    {
      fputc(olow, outfp);
      fputc(ohigh, outfp);
    }
    else
    {
      fputc(low, outfp);
      fputc(high, outfp);
      olow = low;
      ohigh = high;
    }*/
    
    low = fgetc(infp);
    fputc(high, outfp);
    fputc(low, outfp);
    
    /*if (abs(ohigh - high) > CR)
    {
      fputc(olow, outfp);
      fputc(ohigh, outfp);
    }
    else
    {
      fputc(low, outfp);
      fputc(high, outfp);
      olow = low;
      ohigh = high;
    }*/
    
    i++;
  }

  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
