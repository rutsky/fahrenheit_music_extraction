/*
 * main.c
 * Hacking Fahrenheit music program.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 16.07.2006
 */

#include "SDL.h"

#include <stdio.h>
#include <stdlib.h>

int NBits = 8;
int XOR = 0;

/* Global variables structure */
typedef struct
{
  /* Audio specification */
  SDL_AudioSpec audio_spec;
  /* Decoding file parameters */
  FILE *infp;
  /* Exit flag */
  int is_exit;
} GlobalVars;

/* Initializing SDL library function.
 * ARGUMENTS:
 *   - pointer to global variables structure:
 *       GlobalVars *global_vars;
 *   - pointer to desired audio specification:
 *       SDL_AudioSpec *desired_audio_spec;
 * RETURNS:
 *   (int) zero on success, non-zero otherwise.
 */
int init_SDL( GlobalVars *global_vars, SDL_AudioSpec *desired_audio_spec )
{
  /* Initializing SDL library */
  if (SDL_Init(SDL_INIT_AUDIO) < 0)
  {
    fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
    return 1;
  }
  
  /* Initializing SDL sound */
  if (SDL_OpenAudio(desired_audio_spec, &global_vars->audio_spec) < 0)
  {
    fprintf(stderr, "Error: SDL_OpenAudio: %s\n", SDL_GetError());
    return 1;
  }
  SDL_PauseAudio(0);
  
  return 0;
} /* End of 'init_SDL' function */

/* Deinitializing SDL library function.
 * ARGUMENTS: None.
 * RETURNS: None.
 */
void deinit_SDL( void )
{
  SDL_Quit();
} /* End of 'deinit_SDL' function */

/* Reading bits from file function.
 * ARGUMENTS:
 *   - pointer to FILE structure:
 *       FILE *fp;
 *   - number of bits to read:
 *       int nbits;
 *   - pointer to result variable:
 *       unsigned long *result;
 * RETURNS:
 *   (int) zero on success, non-zero otherwise.
 */
int read_bits( FILE *fp, int nbits, unsigned long *result )
{
  static int pos = -1;
  static int byte;
  int i;
  
  // FIXME: This function only for single 'fp'! And one single.
  *result = 0;
  for (i = 0; i < nbits; i++)
  {
    if (pos < 0)
    {
      if ((byte = fgetc(fp)) == EOF)
        return 1;
      pos = 7;
    }
    *result = (*result << 1) | ((byte >> pos--) & 1);
  }
  
  return 0;
} /* End of 'read_bits' function */

/* The filling audio buffer callback function.
 * ARGUMENTS:
 *   - pointer to user data:
 *       void *userdata;
 *   - pointer to audio stream:
 *       Uint8 *stream;
 *   - audio stream filling length:
 *       int len;
 * RETURNS: None.
 */
void mix_audio( void *user_data, Uint8 *stream, int len )
{
  int i;
  GlobalVars *global_vars = (GlobalVars *)user_data;
  unsigned long bits, a, b, c, d;

  for (i = 0; i < len; i++)
  {
    //bits = fgetc(global_vars->infp);
    /*read_bits(global_vars->infp, NBits, &bits);
    read_bits(global_vars->infp, NBits, &bits);*/
    
    read_bits(global_vars->infp, NBits, &a);
    read_bits(global_vars->infp, NBits, &b);
    read_bits(global_vars->infp, NBits, &c);
    read_bits(global_vars->infp, NBits, &d);
    
    //stream[i++] = (Uint8)((((double)(bits ^ XOR) / ((unsigned long)1 << NBits) - 1)) * 255);
    stream[i++] = (Uint8)((a + b + c + d) / 4);
    stream[i++] = (Uint8)((a + b + c + d) / 4);
    stream[i++] = (Uint8)((a + b + c + d) / 4);
    stream[i] = (Uint8)((a + b + c + d) / 4);
    
    if (feof(global_vars->infp))
    {
      global_vars->is_exit = 1;
      memset(stream + i, global_vars->audio_spec.silence, len - i);
      break;
    }
  }
} /* End of 'mix_audio' function */

/* The main program loop.
 * ARGUMENTS:
 *   - pointer to global variables structure:
 *       GlobalVars *global_vars;
 * RETURNS: None.
 */
void main_loop( GlobalVars *global_vars )
{
  while (!global_vars->is_exit)
    ;
} /* End of 'main_loop' function */

/* Prints audio specification function
 * ARGUMENTS:
 *   - pointer to globals variables structure:
 *       GlobalVars *global_vars;
 * RETURNS: None.
 */
void print_audio_spec( GlobalVars *global_vars )
{
  printf("Hardware sound:\n"
    " Freq.: %i.\n"
    " Format: %i.\n"
    " Channels: %i.\n"
    " Silence: %i.\n"
    " Samples: %i.\n"
    " Size: %i.\n",
    global_vars->audio_spec.freq, global_vars->audio_spec.format & 0xFF,
    global_vars->audio_spec.channels, global_vars->audio_spec.silence,
    global_vars->audio_spec.samples, global_vars->audio_spec.size);
} /* End of 'print_audio_spec' function */

/* Opening and preparing sound file function
 * ARGUMENTS:
 *   - pointer to global variables structure:
 *       GlobalVars *global_vars;
 *   - file name:
 *       char *file_name;
 *   - offset in input file:
 *       long offset;
 * RETURNS:
 *   (int) zero on success, non-zero otherwise.
 */
int open_sound_file( GlobalVars *global_vars, char *file_name, long offset )
{
  if ((global_vars->infp = fopen(file_name, "rb")) == NULL)
  {
    perror("fopen");
    return 1;
  }
  
  if (fseek(global_vars->infp, offset, SEEK_SET))
  { 
    perror("fseek");
    return 1;
  }
  
  return 0;
} /* End of 'open_sound_file' function */

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
  GlobalVars global_vars;
  SDL_AudioSpec desired_audio_spec;
  
  global_vars.infp = NULL;
  if (argc > 2)
  {
    sscanf(argv[1], "%d", &NBits);
    sscanf(argv[2], "%d", &XOR);
  }
  
  /* Setting audio specification */
  desired_audio_spec.freq = 44100;
  desired_audio_spec.format = AUDIO_U8;
  desired_audio_spec.channels = 1;
  desired_audio_spec.samples = 512;  /* Good value for games */
  desired_audio_spec.callback = mix_audio;
  desired_audio_spec.userdata = &global_vars;
  
  /* Initializing SDL library */
  if (init_SDL(&global_vars, &desired_audio_spec))
    return 1;
    
  print_audio_spec(&global_vars);
  open_sound_file(&global_vars, "d03.c1", 0x00014190L);
    
  /* Main loop */
  global_vars.is_exit = 0;
  main_loop(&global_vars);
    
  /* Deinitializing SDL library */
  deinit_SDL();

  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
