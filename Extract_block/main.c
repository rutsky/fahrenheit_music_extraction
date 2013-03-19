/*
 * main.c
 * Hacking Fahrenheit game data file program.
 * Extracting one block by given offset program.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 11.08.2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  if (argc < 3)
  {
    printf(
      "Hacking Fahrenheit game data file program.\n"
      "Extracting one block by given offset program.\n"
      "Usage:\n"
      "  %s file_name offset\n"
      "      Will extract data block at position 'offset' of 'file_name' file to STDOUT.\n",
      argv[0]);
  }
  else 
  {
    FILE *infp, *outfp = stdout;
    long offset, len, val, i;
    int ch;
    char tag[9];

    if ((infp = fopen(argv[1], "rb")) == NULL)
    {
      perror("fopen");
      return 1;
    }
    sscanf(argv[2], "%li", &offset);
    if (fseek(infp, offset, SEEK_SET))
    {
      perror("fseek");
      fclose(infp);
      return 1;
    }
    fread(&tag, 1, 8, infp);
    tag[8] = 0;
    fread(&val, 4, 1, infp);
    fread(&len, 4, 1, infp);
    fprintf(stderr, "Tag '%s', value = %li, %li bytes length.\n", tag, val, len);
    
    for (i = 0; i < len; i++)
    {
      if ((ch = fgetc(infp)) == EOF)
      {
        fprintf(stderr, "Error: EOF reached at %li position of block.\n", i);
        fclose(infp);
        return 1;
      } 
      fputc(ch, outfp);
    }
    
    fclose(infp);
  }

  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
