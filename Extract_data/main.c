/*
 * main.c
 * Hacking Fahrenheit data.
 * Extracting data blocks program.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 06.08.2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Skipping void data in stream function */
long skip_void( FILE *fp )
{
  int ch;
  long skipped = 0;
  
  while ((ch = fgetc(fp)) != EOF && ch == '-')
    skipped++;

  if (!feof(fp))
    ungetc(ch, fp);
  
  return skipped;
} /* End of 'skip_void' function */

/* Extracting one block function */
int extract_block( FILE *infp )
{
  char name[9], file_name[100];
  long id, data_len, skipped, offset, pos;
  int ch;
  FILE *outfp;
  
  /* Skipping void data */
  skipped = skip_void(infp);
  
  if (feof(infp))
    return 1;
  if ((offset = ftell(infp)) < 0)
  {
    perror("ftell");
    return 1;
  }
  /* Reading block name */
  if (fread(name, 1, 8, infp) != 8)
  {
    fprintf(stderr, "Error reading block name (0x%04X%04X).\n",
      (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF));
    if (feof(infp))
      fprintf(stderr, "EOF reached.\n");
    return 1;
  }
  name[8] = 0;
  /* Reading ID */
  if (fread(&id, 4, 1, infp) != 1)
  {
    fprintf(stderr, "Error reading block ID (0x%04X%04X).\n",
      (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF));
    if (feof(infp))
      fprintf(stderr, "EOF reached.\n");
    return 1;
  }
  if (id < 0)
  {
    fprintf(stderr, "Error: ID < 0.\n");
    return 1;
  }
  /* Reading data length */
  if (fread(&data_len, 4, 1, infp) != 1)
  {
    fprintf(stderr, "Error reading block data length (0x%04X%04X).\n",
      (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF));
    if (feof(infp))
      fprintf(stderr, "EOF reached.\n");
    return 1;
  }
  if (data_len < 0)
  {
    fprintf(stderr, "Error: data_len < 0.\n");
    return 1;
  }
  
  /* Creating output file */
  sprintf(file_name, "0x%04X%04X_%s_%li",
    (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF),
    name, id);
  printf("%s (%li bytes)... ", file_name, data_len);
  if ((outfp = fopen(file_name, "wb")) == NULL)
  {
    perror("fopen");
    return 1;
  }
  
  /* Extracting data */
  for (pos = 0; pos < data_len; pos++)
  {
    if ((ch = fgetc(infp)) == EOF)
    {
      fprintf(stderr, "EOF reached while reading block data (0x%04X%04X) at offset %li.\n",
        (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF), pos);
    }
    fputc(ch, outfp);
  }
  
  fclose(outfp);
  
  printf("done\n");
  
  return 0;
} /* End of 'extract_block' function */

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
  FILE *fp;
  long nblocks;
  
  if (argc == 1)
  {
    printf(
      "Usage:\n"
      "  %s data_file\n",
      argv[0]);
    return 0;
  }
  
  /* Opening data file */
  if ((fp = fopen(argv[1], "rb")) == NULL)
  {
    perror("fopen");
    return 1;
  }
  
  /* Seeking magick */
  if (fseek(fp, 24L, SEEK_SET))
  {
    perror("fseek");
    fclose(fp);
    return 1;
  }

  /* Extracting blocks */
  nblocks = 0;
  while (!extract_block(fp))
    nblocks++;
  printf("%li blocks total.\n", nblocks);
  
  fclose(fp);

  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
