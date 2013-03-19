/*
 * main.c
 * Hacking Fahrenheit data program.
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
} /* End of 'read_name' function */

/* Recursively loading child function */
int load_child( FILE *infp, FILE *outfp, int ind )
{
  char name[9];
  long nchild, data_len, skipped, offset;
  int i, error;
  
  skipped = skip_void(infp);
  offset = ftell(infp);
  if (fread(name, 1, 8, infp) != 8)
  {
    perror("fread");
    fprintf(stderr, "<name>\n");
    if (feof(infp))
      fprintf(stderr, "EOF reached!\n");
    return 1;
  }
  name[8] = 0;
  if (fread(&nchild, 4, 1, infp) != 1)
  {
    perror("fread");
    fprintf(stderr, "<nchild>\n");
    return 1;
  }
  if (nchild < 0)
  {
    fprintf(stderr, "nchild < 0!\n");
    return 1;
  }
  if (fread(&data_len, 4, 1, infp) != 1)
  {
    perror("fread");
    fprintf(stderr, "<data_len>\n");
    return 1;
  }
  if  (data_len < 0)
  {
    fprintf(stderr, "data_len < 0!\n");
    return 1;
  }
  
  for (i = 0; i < ind; i++)
    fprintf(outfp, " ");
  /*
  fprintf(outfp, "'%s' 0x%04X%04X (%li skipped): %li children, %li data length\n", 
    name, (unsigned int)((offset >> 16) & 0xFFFF),
    (unsigned int)(offset & 0xFFFF), skipped, nchild, data_len);
  */
  /*
  fprintf(outfp, "%s %li\n", name, nchild);
  */
  /*
  if (!strcmp(name, "STREAMAB"))
  {
    //fprintf(outfp, "%li: ", data_len);
    for (i = 0; i < data_len; i++)
      fprintf(outfp, "%c", fgetc(infp));
    while (i++ < 36 * 20)
      fprintf(outfp, "-");
    fseek(infp, -data_len, SEEK_CUR);
  }
  */
  /*
  if (!strcmp(name, "HEADER__"))
  {
    //fprintf(outfp, "%li: ", data_len);
    for (i = 0; i < data_len; i++)
      fprintf(outfp, "%c", fgetc(infp));
    while (i++ < 36 * 20)
      fprintf(outfp, "-");
    fseek(infp, -data_len, SEEK_CUR);
  }
  */
  if (!strcmp(name, "HEADER__"))
  {
    fprintf(outfp, "'%s' 0x%04X%04X (%li skipped): %li children, %li data length\n", 
      name, (unsigned int)((offset >> 16) & 0xFFFF),
      (unsigned int)(offset & 0xFFFF), skipped, nchild, data_len);
  }
  fflush(outfp);
  
  if (fseek(infp, data_len, SEEK_CUR))
  {
    perror("fseek");
    fprintf(stderr, "<fseek(data_len)>\n");
    return 1;
  }
  
  for (i = 0; i < nchild; i++)
    if ((error = load_child(infp, outfp, ind)))
      return error;
  fprintf(outfp, "up!\n");
  
  return 0;
} /* End of 'load_child' function */

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
  FILE *infp = stdin, *outfp = stdout;
  char buf[100];
  long root_nchild;
  long i;
  
  if (argc > 1)
  {
    if ((infp = fopen(argv[1], "rb")) == NULL)
    {
      perror("fopen");
      return 1;
    }
  }
  
  fread(&buf, 1, 20, infp);
  buf[20] = 0;
  //fprintf(outfp, "ROOT: '%s' ", buf);
  fread(&root_nchild, 4, 1, infp);
  //fprintf(outfp, "%li chldren\n", root_nchild);
  
  i = 0;
  while (!feof(infp))
  {
    load_child(infp, outfp, 0);
    i++;
  }
  //fprintf(outfp, "Total entries: %li.\n", i);
  /*for (i = 0; i < root_nchild; i++)
  {
    load_child(infp, outfp, 0);
  }*/
  
  if (infp != stdin)
    fclose(infp);
  if (outfp != stdout)
    fclose(outfp);

  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
