/*
 * main.c
 * Hacking Fahrenheit data.
 * Extracting data to real files program.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 12.08.2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG_BLOCKS_DESCR_LOADING
//#define DEBUG_LOAD_BLOCK

enum
{
  DATA_MAGICK_OFFSET = 24,      /* Length of 'QUANTICDREAMTABINDEX' + 0x08000000 */
  DATA_MAX_NBLOCKS = 30000,     /* Maximum number of blocks in file function */
  DATA_MAX_HEADER_LEN = 50,     /* Maximum length of 'HEADER__' data in file */
  DATA_MAX_STREAMAB_LEN = 2000, /* Maximum length of 'STREAMAB' data in file */
  MAX_FILE_NAME_LEN = 100,      /* Maximum file name length */
  MAX_COUNTRY_LEN = 4           /* Maximum country reduction name length */
};

/* Block description structure definition */
typedef struct
{
  char name[9];  /* Block name */
  long id;       /* Block id */
  long data_len; /* Block data length */
  long offset;   /* Offet in data file to block data first byte */
} BlockDescr;

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

/* Loading one block description function */
int load_block_descr( FILE *fp, BlockDescr *block_descr )
{
  long offset;
  
  /* Skipping void data */
  skip_void(fp);
  
  if (feof(fp))
    return 1;
  if ((offset = ftell(fp)) < 0)
  {
    perror("ftell");
    return 1;
  }
  /* Reading block name */
  if (fread(block_descr->name, 1, 8, fp) != 8)
  {
    fprintf(stderr, "Error reading block name (0x%04X%04X).\n",
      (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF));
    if (feof(fp))
      fprintf(stderr, "EOF reached.\n");
    return 1;
  }
  block_descr->name[8] = 0;
  /* Reading ID */
  if (fread(&block_descr->id, 4, 1, fp) != 1)
  {
    fprintf(stderr, "Error reading block ID (0x%04X%04X).\n",
      (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF));
    if (feof(fp))
      fprintf(stderr, "EOF reached.\n");
    return 1;
  }
  if (block_descr->id < 0)
  {
    fprintf(stderr, "Error: ID < 0.\n");
    return 1;
  }
  /* Reading data length */
  if (fread(&block_descr->data_len, 4, 1, fp) != 1)
  {
    fprintf(stderr, "Error reading block data length (0x%04X%04X).\n",
      (unsigned int)((offset >> 16) & 0xFFFF), (unsigned int)(offset & 0xFFFF));
    if (feof(fp))
      fprintf(stderr, "EOF reached.\n");
    return 1;
  }
  if (block_descr->data_len < 0)
  {
    fprintf(stderr, "Error: data_len < 0.\n");
    return 1;
  }
  /* Calculating data offset */
  block_descr->offset = offset + 8 + 4 + 4;
  
  return 0;
} /* End of 'load_block_descr' function */

/* Creating blocks description list from data file function */
long load_blocks_descr_list( FILE *fp, BlockDescr *blocks_descr, long max_nblocks )
{
  long nblocks;
  
  if (fseek(fp, DATA_MAGICK_OFFSET, SEEK_SET))
  {
    perror("fseek");
    return -1;
  }
  
  /* Loading blocks descriptions */
  printf("Loading blocks description list");
  fflush(stdout);
  nblocks = 0;
  while (nblocks < max_nblocks && !load_block_descr(fp, &blocks_descr[nblocks]))
  {
    if (nblocks % 300 == 0)
    {
      printf(".");
      fflush(stdout);
    }
    if (fseek(fp, blocks_descr[nblocks].data_len, SEEK_CUR))
    {
      perror("fseek");
      return 1;
    }
    nblocks++;
  }
  printf(" done\n");
  
  return nblocks;
} /* End of 'create_blocks_descr_list' function */

/* Extracting raw block by description function */
int extract_raw_block( FILE *infp, BlockDescr *block_descr )
{
  FILE *outfp;
  long pos;  
  char file_name[MAX_FILE_NAME_LEN];
  int ch;

  if (fseek(infp, block_descr->offset, SEEK_SET))
  {
    perror("fseek");
    return 1;
  }
  
  /* Creating output file */
  snprintf(file_name, MAX_FILE_NAME_LEN, "0x%04X%04X_%s_%li",
    (unsigned int)(((block_descr->offset - 8 - 4 - 4) >> 16) & 0xFFFF),
    (unsigned int)((block_descr->offset - 8 - 4 - 4) & 0xFFFF),
    block_descr->name, block_descr->id);
  printf("%s (%li bytes)... ", file_name, block_descr->data_len);
  if ((outfp = fopen(file_name, "wb")) == NULL)
  {
    perror("fopen");
    return 1;
  }

  /* Extracting data */
  for (pos = 0; pos < block_descr->data_len; pos++)
  {
    if ((ch = fgetc(infp)) == EOF)
    {
      fprintf(stderr, "EOF reached while reading block data (0x%04X%04X) at offset %li.\n",
        (unsigned int)(((block_descr->offset - 8 - 4 - 4) >> 16) & 0xFFFF),
        (unsigned int)((block_descr->offset - 8 - 4 - 4) & 0xFFFF), pos);
    }
    fputc(ch, outfp);
  }
  
  fclose(outfp);
  printf("done\n");
  
  return 0;
} /* End of 'extract_raw_block' function */

/* The loading block content from data file function */
int load_block( FILE *fp, BlockDescr *block_descr, unsigned char *data )
{
  long pos;
  int ch;
  
  /* Seeking to begin of data */
  if (fseek(fp, block_descr->offset, SEEK_SET))
  {
    perror("fseek");
    return 1;
  }
  /* Loadign data */
  for (pos = 0; pos < block_descr->data_len; pos++)
  {
    if ((ch = fgetc(fp)) == EOF)
    {
      fprintf(stderr, "EOF reached while reading block data (0x%04X%04X) at offset %li.\n",
        (unsigned int)(((block_descr->offset - 8 - 4 - 4) >> 16) & 0xFFFF),
        (unsigned int)((block_descr->offset - 8 - 4 - 4) & 0xFFFF), pos);
      return 1;
    }
    data[pos] = ch;
#ifdef DEBUG_LOAD_BLOCK
    printf("%02X ", ch);
#endif /* DEBUG_LOAD_BLOCK */
  }
#ifdef DEBUG_LOAD_BLOCK
  printf("\n");
#endif

  return 0;
} /* End of 'load_block' function */

/* Writing compressed WAVE (0x0011) header to file function */
int write_WAVE_header0x0011( FILE *fp, long RIFF_size, short nchannels, 
    long sample_rate, long byte_rate, short block_align, short bits_per_sample,
    short samples_per_block, long data_size )
{
  long format_chunk_size = 0x14;
  short audio_format = 0x11;
  short extra_format_size = 0x2;
  long fact_chunk_size = 0x4;
  long fact_data = data_size * 2;

  /* RIFF chunk identificator (4 bytes) */
  fwrite("RIFF", 1, 4, fp);
  /* RIFF chunk size (file size) (4 bytes) */
  fwrite(&RIFF_size, 4, 1, fp);
  /* WAVE format (4 bytes) */
  fwrite("WAVE", 1, 4, fp);
  
  /* Format chunk identificator (4 bytes) */
  fwrite("fmt ", 1, 4, fp);
  /* Format chunk size (4 bytes) */
  fwrite(&format_chunk_size, 4, 1, fp);
  /* Audio format (2 bytes) */
  fwrite(&audio_format, 2, 1, fp);
  /* Number of audio channels (2 bytes) */
  fwrite(&nchannels, 2, 1, fp);
  /* Sample rate (4 bytes) */
  fwrite(&sample_rate, 4, 1, fp);
  /* Byte rate (4 bytes) */
  fwrite(&byte_rate, 4, 1, fp);
  /* Block align (2 bytes) */
  fwrite(&block_align, 2, 1, fp);
  /* Bits per sample (2 bytes) */
  fwrite(&bits_per_sample, 2, 1, fp);
  /* Extra format chunk size (2 bytes) */
  fwrite(&extra_format_size, 2, 1, fp);
  /* Samples per block (2 bytes) */
  fwrite(&samples_per_block, 2, 1, fp);
  
  /* 'fact' chunk identificator (4 bytes) */
  fwrite("fact", 1, 4, fp);
  /* 'fact' chunk size (4 bytes) */
  fwrite(&fact_chunk_size, 4, 1, fp);
  /* 'fact' chunk data (4 bytes) */
  fwrite(&fact_data, 4, 1, fp);
  
  /* Data chunk identificator (4 bytes) */
  fwrite("data", 1, 4, fp);
  /* Data chunk size (4 bytes) */
  fwrite(&data_size, 4, 1, fp);
  
  return 0;
} /* End of 'write_WAVE_header' function */

/* Writing uncompressed WAVE (0x0001) header to file function */
int write_WAVE_header0x0001( FILE *fp, long RIFF_size, short nchannels, 
    long sample_rate, long byte_rate, short block_align, short bits_per_sample,
    long data_size )
{
  long format_chunk_size = 0x10;
  short audio_format = 0x1;

  /* RIFF chunk identificator (4 bytes) */
  fwrite("RIFF", 1, 4, fp);
  /* RIFF chunk size (file size) (4 bytes) */
  fwrite(&RIFF_size, 4, 1, fp);
  /* WAVE format (4 bytes) */
  fwrite("WAVE", 1, 4, fp);
  
  /* Format chunk identificator (4 bytes) */
  fwrite("fmt ", 1, 4, fp);
  /* Format chunk size (4 bytes) */
  fwrite(&format_chunk_size, 4, 1, fp);
  /* Audio format (2 bytes) */
  fwrite(&audio_format, 2, 1, fp);
  /* Number of audio channels (2 bytes) */
  fwrite(&nchannels, 2, 1, fp);
  /* Sample rate (4 bytes) */
  fwrite(&sample_rate, 4, 1, fp);
  /* Byte rate (4 bytes) */
  fwrite(&byte_rate, 4, 1, fp);
  /* Block align (2 bytes) */
  fwrite(&block_align, 2, 1, fp);
  /* Bits per sample (2 bytes) */
  fwrite(&bits_per_sample, 2, 1, fp);
  
  /* Data chunk identificator (4 bytes) */
  fwrite("data", 1, 4, fp);
  /* Data chunk size (4 bytes) */
  fwrite(&data_size, 4, 1, fp);
  
  return 0;
} /* End of 'write_WAVE_header' function */

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
  long nblocks, i;
  BlockDescr blocks_descr[DATA_MAX_NBLOCKS];
  unsigned char header[DATA_MAX_HEADER_LEN];
  unsigned char streamab[DATA_MAX_STREAMAB_LEN];

  
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
  
  if ((nblocks = load_blocks_descr_list(fp, blocks_descr, DATA_MAX_NBLOCKS)) < 0)
  {
    fclose(fp);
    return 1;
  }
  
#ifdef DEBUG_BLOCKS_DESCR_LOADING
  for (i = 0; i < nblocks; i++)
    printf("0x%04X%04X '%s', id: %li, %7li bytes\n",
      (unsigned int)((blocks_descr[i].offset >> 16) & 0xFFFF),
      (unsigned int)(blocks_descr[i].offset & 0xFFFF),
      blocks_descr[i].name, blocks_descr[i].id,
      blocks_descr[i].data_len);
  printf("%li blocks total.\n", nblocks);
#endif /* DEBUG_BLOCKS_DESCR_LOADING */

  /* Extracting data */
  for (i = 0; i < nblocks; i++)
  {
    if (!strcmp(blocks_descr[i].name, "HEADER__"))
    {
      /* Found header */
      int is_OGG, is_uncompressed_WAVE, is_dialog;
      char country[MAX_COUNTRY_LEN];
      
      is_OGG = 0;
      is_uncompressed_WAVE = 0;
      is_dialog = 0;

      /* Loading header data */
      if (load_block(fp, &blocks_descr[i], header))
        return 1;
      
      if (blocks_descr[i].data_len == 17)
      {
        long rate;                      /* Sample rate */
        char magick[5], file_name[MAX_FILE_NAME_LEN];
        long pos, ogg_header_offset = 0x800;
        FILE *newfp;
        int ch;

        /* Checking for OGG Vorbis format */
        if (fseek(fp, blocks_descr[i - 3].offset + ogg_header_offset, SEEK_SET))
        {
          perror("fseek");
          fclose(fp);
          return 1;
        }
        
        if (fread(magick, 1, 4, fp) != 4)
        {
          fprintf(stderr, "fread() failed.\n");
          fclose(fp);
          return 1;
        }
        magick[4] = 0;
        
        if (!strcmp(magick, "OggS"))
        {
          /* OGG Vorbis format detected! */
          is_OGG = 1;
          
          /* Seeking to header start */
          pos = ogg_header_offset;
          if (fseek(fp, -4, SEEK_CUR))
          {
            perror("fseek");
            fclose(fp);
            return 1;
          }
          
          /* Getting sample rate */
          rate = *(long *)(header + 9);
          
          /* Creating file */
          snprintf(file_name, MAX_FILE_NAME_LEN, "0x%04X%04X.ogg",
            (unsigned int)(((blocks_descr[i - 3].offset - 8 - 4 - 4) >> 16) & 0xFFFF),
            (unsigned int)((blocks_descr[i - 3].offset - 8 - 4 - 4) & 0xFFFF));
        
          printf("Extracting '%s'...\n"
            "  Rate: %li\n",
            file_name, rate);
          if ((newfp = fopen(file_name, "wb")) == NULL)
          {
            perror("fopen");
            fclose(fp);
            return 1;
          }

          while (pos < blocks_descr[i - 3].data_len)
          {
            pos++;
            if ((ch = fgetc(fp)) == EOF)
              break;
            fputc(ch, newfp);
          }
    
          fclose(newfp);
          printf("Done.\n");
        }
      }
      
      if (is_OGG)
        continue;
      
      if (blocks_descr[i].data_len == 17 && header[0] == 0)
      {
        /* Uncompressed WAVE */
        long rate;                      /* Sample rate */
        int nchannels = 1;              /* Number of audio channels */
        char file_name[MAX_FILE_NAME_LEN];
        long pos, len;
        FILE *newfp;
        int ch;

        /* Creating file */
        snprintf(file_name, MAX_FILE_NAME_LEN, "0x%04X%04X.wav",
          (unsigned int)(((blocks_descr[i - 3].offset - 8 - 4 - 4) >> 16) & 0xFFFF),
          (unsigned int)((blocks_descr[i - 3].offset - 8 - 4 - 4) & 0xFFFF));

        /* Getting sample rate */
        rate = *(long *)(header + 9);
        
        printf("Extracting '%s'...\n"
          "  Rate: %li\n",
          file_name, rate);
        if ((newfp = fopen(file_name, "wb")) == NULL)
        {
          perror("fopen");
          fclose(fp);
          return 1;
        }

        /* Skipping header writing */
        if (fseek(newfp, 0x28, SEEK_SET))
        {
          perror("fseek");
          fclose(newfp);
          fclose(fp);
          return 1;
        }         

        /* Seeking to begin of WAVE data */
        if (fseek(fp, blocks_descr[i - 3].offset, SEEK_SET))
        {
          perror("fseek");
          fclose(fp);
          return 1;
        }

        /* Extracting data */
        pos = 0;
        len = 0;
        while (pos < blocks_descr[i - 3].data_len)
        {
          pos++;
          if ((ch = fgetc(fp)) == EOF)
            break;
          len++;
          fputc(ch, newfp);
        }
        
        /* Writing header */
        if (fseek(newfp, 0, SEEK_SET))
        {
          perror("fseek");
          fclose(newfp);
          fclose(fp);
          return 1;
        }
        if (write_WAVE_header0x0001(newfp, len + 0x28, nchannels, rate, 
          rate * nchannels * 16 / 8, nchannels * 16 / 8, 16, len))
        {
          fclose(newfp);
          fclose(fp);
          return 1;
        }
    
        fclose(newfp);
        printf("Done.\n");
        is_uncompressed_WAVE = 1;
      }
      
      if (is_uncompressed_WAVE)
        continue;
        
      if (blocks_descr[i].data_len == 7 || blocks_descr[i].data_len == 6)
      {
        /* Dialog */
        continue;
        long country_len = *(long *)header;
        int i;
        
        for (i = 0; i < country_len; i++)
          country[i] = (header + 4)[i];
        country[i] = 0;
        
        is_dialog = 1;
      }
      
      if (blocks_descr[i].data_len == 21 || blocks_descr[i].data_len == 17 || is_dialog)
      {
        long rate;         /* Sample rate */
        int nchannels;     /* Number of audio channels */
        short block_align; /* Block align */
        char file_name[MAX_FILE_NAME_LEN];
        FILE *newfp;
        long start_offset, microblock_len, microblock_space_len, space_test_len;
        long len, j, pos;
        int ch, is_skip;
      
        /* Parsing header data */
        /* header[17]:
         *   2 - soundtrack
         *   4 - just sound
         */
         
        if (is_dialog)
        {
          if (blocks_descr[i - 1].data_len > 89) /* DATA_MAX_STREAMAB_LEN */
          {
            printf("==========> Unknown length %li (%s)!!!\n", 
              blocks_descr[i - 1].data_len, country);
            continue;
          }
          
          /* Loading 'STREAMAB' data */
          if (load_block(fp, &blocks_descr[i - 1], streamab))
            return 1;
        }
         
        if (is_dialog)
        {
          start_offset = 0x810;
          if (blocks_descr[i - 1].data_len == 89)
          {
            microblock_len = 0x122F4;
            microblock_space_len = 0x350C;
            space_test_len = 0;
            block_align = 36;
            nchannels = 1;
          }
          else
          {
            if (streamab[0x2C] == 0)
            {
              microblock_len = 0x122F4;
              microblock_space_len = 0x50C;
              space_test_len = 20;
              block_align = 36;
              nchannels = 1;
            }
            else if (streamab[0x2C] == 1)
            {
              microblock_len = 0x245A0;
              microblock_space_len = 0x260;
              space_test_len = 20;
              block_align = 72;
              nchannels = 2;
            }
            else
            {
              printf("==========> Unknown streamab[0x2C]!!!\n");
              continue;
            }
          }
        }
        else
        {
          start_offset = 0x800;
          if (header[8] == 0)
          {
            microblock_len = 0x122F4;
            microblock_space_len = 0x50C;
            space_test_len = 20;
            block_align = 36;
            nchannels = 1;
          }
          else if (header[8] == 1)
          {
            microblock_len = 0x245A0;
            microblock_space_len = 0x260;
            space_test_len = 20;
            block_align = 72;
            nchannels = 2;
          }
          else
          {
            printf("==========> Unknown header[8]!!!\n");
            continue;
          }
        }
        
        /* Getting sample rate */
        if (is_dialog)
          rate = *(long *)(streamab + 0x2d);
        else
          rate = *(long *)(header + 9);
        
        /* Creating file */
        if (is_dialog)
          snprintf(file_name, MAX_FILE_NAME_LEN, "1_0x%04X%04X_%s.wav",
            (unsigned int)(((blocks_descr[i - 3].offset - 8 - 4 - 4) >> 16) & 0xFFFF),
            (unsigned int)((blocks_descr[i - 3].offset - 8 - 4 - 4) & 0xFFFF),
            country);
        else if (blocks_descr[i].data_len == 21)
          snprintf(file_name, MAX_FILE_NAME_LEN, "0x%04X%04X_%d.wav",
            (unsigned int)(((blocks_descr[i - 3].offset - 8 - 4 - 4) >> 16) & 0xFFFF),
            (unsigned int)((blocks_descr[i - 3].offset - 8 - 4 - 4) & 0xFFFF),
            header[17]);
        else
          snprintf(file_name, MAX_FILE_NAME_LEN, "0x%04X%04X.wav",
            (unsigned int)(((blocks_descr[i - 3].offset - 8 - 4 - 4) >> 16) & 0xFFFF),
            (unsigned int)((blocks_descr[i - 3].offset - 8 - 4 - 4) & 0xFFFF));
        
        printf("Extracting '%s'...\n"
          "  Rate: %li\n"
          "  Channels: %i\n",
          file_name, rate, nchannels);
        if ((newfp = fopen(file_name, "wb")) == NULL)
        {
          perror("fopen");
          fclose(fp);
          return 1;
        }

        /* Skipping header writing */
        if (fseek(newfp, 0x3C, SEEK_SET))
        {
          perror("fseek");
          fclose(newfp);
          fclose(fp);
          return 1;
        }         
        
        /* Seeking to begin of data */
        pos = start_offset;
        if (fseek(fp, blocks_descr[i - 3].offset + start_offset, SEEK_SET))
        {
          perror("fseek");
          fclose(newfp);
          fclose(fp);
          return 1;
        }
        /* Extracting data */
        len = 0;
        while (pos < blocks_descr[i - 3].data_len && !feof(fp))
        {
          for (j = 0; j < microblock_len && pos < blocks_descr[i - 3].data_len; j++)
          {
            pos++;
            if ((ch = fgetc(fp)) == EOF)
              break;
            fputc(ch, newfp);
            len++;
          }
          if (feof(fp) || pos >= blocks_descr[i - 3].data_len)
            break;

          /* Checking for micro-blocks delimiter */
          is_skip = 1;
          for (j = 0; j < space_test_len; j++)
            if (fgetc(fp) != 0)
            {
              is_skip = 0;
              j++;
              break;
            }

          /* Seeking back */
          if (fseek(fp, -j, SEEK_CUR))
          {
            perror("fseek");
            fclose(newfp);
            fclose(fp);
            return 1;
          }
            
          if (is_skip)
          {
            /* Skipping block delimiter */
            if (fseek(fp, microblock_space_len, SEEK_CUR))
            {
              perror("fseek");
              fclose(newfp);
              fclose(fp);
              return 1;
            }
            pos += microblock_space_len;
          }
          else
            printf("  No delimiter.\n");
        }
        
        /* Writing header */
        if (fseek(newfp, 0, SEEK_SET))
        {
          perror("fseek");
          fclose(newfp);
          fclose(fp);
          return 1;
        }
        if (write_WAVE_header0x0011(newfp, len + 0x38, nchannels, rate, 
          rate / (2 * nchannels), /* FIXME: I'm not sure */
          block_align, 4,
          ((block_align - 4 * nchannels) / nchannels * (8 / 4) + 1),
          len))
        {
          fclose(newfp);
          fclose(fp);
          return 1;
        }
    
        fclose(newfp);
        printf("Done.\n");
      }
      else
      {
        printf("Unknown header (0x%04X%04X)!\n",
          (unsigned int)(((blocks_descr[i - 3].offset - 8 - 4 - 4) >> 16) & 0xFFFF),
          (unsigned int)((blocks_descr[i - 3].offset - 8 - 4 - 4) & 0xFFFF));
        return 0;
      }
    }
  }
  
  fclose(fp);

  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
