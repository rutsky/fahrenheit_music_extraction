/*
 * main.c
 * Hacking Fahrenheit music program.
 * Bob Rutsky <rutsky_v@rambler.ru>
 * 10.08.2006
 */

#include "SDL.h"

#include "adpcm.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define DEBUG_SDL_AUDIO
#define DEBUG

/* Global variables structure */
typedef struct
{
  /* Audio specification */
  SDL_AudioSpec audio_spec;
  /* WAVE data structure */
  WAVEData wd;
  /* Exit flag */
  volatile int is_exit;
} GlobalVars;

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
  int pos;
  static long buffer_pos = 0;
  GlobalVars *global_vars = (GlobalVars *)user_data;
  int16_t *int16_stream = (int16_t *)stream;
  int int16_stream_len = len / sizeof(int16_t);
  
  for (pos = 0; pos < int16_stream_len; )
  {
    if (feof(global_vars->wd.fp))
    {
      global_vars->is_exit = 1;
      break;
    }
    
    if (buffer_pos < global_vars->wd.buffer_len)
    {
      //if ((global_vars->wd.nchannels == 2 && buffer_pos % 2 == 0) || global_vars->wd.nchannels == 1)
        //fwrite(&global_vars->wd.buffer[buffer_pos], 2, 1, stderr);
      int16_stream[pos++] = global_vars->wd.buffer[buffer_pos++];
    }
    else
    {
      buffer_pos = 0;
      decode_next_block(&global_vars->wd);
    }
  } 
  
  while (pos < int16_stream_len)
    int16_stream[pos++] = global_vars->audio_spec.silence;
} /* End of 'mix_audio' function */

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
  if ((global_vars->wd.fp = fopen(file_name, "rb")) == NULL)
  {
    perror("fopen");
    return 1;
  }
  
  if (fseek(global_vars->wd.fp, offset, SEEK_SET))
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
  int block_align = 0x400;
  char *file_name = "t.wav";
  long offset = 0;
  long rate = 22050;
  int nchannels = 1;

  if (argc == 1)
  {
    printf(
      "Usage:\n"
      "  %s file_name [ block_align = 0x400 [ offset_in_file = 0 [ sample_rate = 22050 [ nchannels = 1 ] ] ] ]\n",
      argv[0]);
    return 0;
  }
  
  if (argc > 1)
    file_name = argv[1];
  if (argc > 2)
    sscanf(argv[2], "%i", &block_align);
  if (argc > 3)
    sscanf(argv[3], "%li", &offset);
  if (argc > 4)
    sscanf(argv[4], "%li", &rate);
  if (argc > 5)
    sscanf(argv[5], "%i", &nchannels);

  /* Setting audio specification */
  desired_audio_spec.freq = rate;
  desired_audio_spec.format = AUDIO_S16;
  desired_audio_spec.channels = nchannels;
  /*
  desired_audio_spec.freq = 44100;
  desired_audio_spec.format = AUDIO_S16;
  desired_audio_spec.channels = 1;
  */
  desired_audio_spec.samples = 512;  /* Good value for games */
  desired_audio_spec.callback = mix_audio;
  desired_audio_spec.userdata = &global_vars;
  
  /* Initializing SDL library */
  if (init_SDL(&global_vars, &desired_audio_spec))
    return 1;
    
#ifdef DEBUG_SDL_AUDIO
  print_audio_spec(&global_vars);
#endif /* DEBUG_SDL_AUDIO */
  if (open_sound_file(&global_vars, file_name, offset))
    exit(1);
  
  global_vars.wd.samples_per_sec = rate;
  global_vars.wd.bits_per_sample = 4;
  global_vars.wd.block_align = block_align;
  global_vars.wd.nchannels = nchannels;
  global_vars.wd.samples_per_block = 
    (global_vars.wd.block_align - sizeof(struct BlockHeader) * global_vars.wd.nchannels) / 
    global_vars.wd.nchannels * (8 / global_vars.wd.bits_per_sample) + 1;
  
  /*
  global_vars.wd.samples_per_sec = 44100;
  global_vars.wd.bits_per_sample = 4;
  global_vars.wd.block_align = 0x400;
  global_vars.wd.samples_per_block = (global_vars.wd.block_align - 4) * 2 + 1;
  */
#ifdef DEBUG
  printf("WAVE format: %li Hz, %i bits, BlockAlign = %i, SamplesPerBlock = %i, Channels = %i.\n", 
    global_vars.wd.samples_per_sec, global_vars.wd.bits_per_sample, 
    global_vars.wd.block_align, global_vars.wd.samples_per_block,
    global_vars.wd.nchannels);
#endif /* DEBUG */
  if (init_WAVE(&global_vars.wd))
    exit(1);
      
  /* Main loop */
  global_vars.is_exit = 0;
  main_loop(&global_vars);

  deinit_WAVE(&global_vars.wd);    
  /* Deinitializing SDL library */
  deinit_SDL();

  return 0;
} /* End of 'main' function */

/* END OF 'main.c' FILE */
