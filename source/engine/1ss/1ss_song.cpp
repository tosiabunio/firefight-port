#include "1ss_hdrs.h"
#include <SDL_mixer.h>

// Port: the CD audio tracks are FLAC files, <music>/track02.flac, track03.flac... (was MCI CD
// audio). Track numbers count audio tracks only, as the original did once it had skipped the
// CD's data track 1: track N is the file numbered N+1. SDL_mixer streams the file; playing it with
// repeat loops it, as the MCI notification that restarted a finished track did. With sound off
// (SOS_NONE) nothing plays; the original still played the CD then.

int CD::tracks;
int CD::current_track;
static const int data_skip=1;
static char music_dir[_MAX_PATH+1]="music";
static Mix_Music* music=NULL;

static void track_file(char* path,int track)
{
  sprintf(path,"%s/track%02d.flac",music_dir,track+data_skip);
}

void CD::set_directory(const char* dir)
{
  strncpy(music_dir,dir,_MAX_PATH);
  music_dir[_MAX_PATH]=0;
}

CD::CD()
{
  tracks=0;
  current_track=0;
}

CD::~CD()
{
  CD::stop();
}

void CD::init(void)
{
  if(!Sounds::active) return;
  // count the tracks (was the number of audio tracks on the CD)
  char path[_MAX_PATH+32];
  tracks=0;
  for(;;) {
    track_file(path,tracks+1);
    FILE* file=fopen(path,"rb");
    if(file==NULL) break;
    fclose(file);
    tracks++;
  }
  ENGINFO("music: %d tracks in %s",tracks,music_dir);
}

void CD::play(int track,int repeat)
{
  if((!Sounds::active)||(!Sounds::app_active)) return;
  if(track>tracks) return;
  if(track==0) return;
  CD::stop();
  if(Sounds::active_mode==SOS_NONE) return;
  char path[_MAX_PATH+32];
  track_file(path,track);
  music=Mix_LoadMUS(path);
  if(music==NULL) {
    WARNING("music track %s does not load (%s)",path,Mix_GetError());
    return;
  }
  if(Mix_PlayMusic(music,repeat?-1:1)!=0) {
    WARNING("music track %s does not play (%s)",path,Mix_GetError());
    CD::stop();
    return;
  }
  MESSAGE("music: track %d%s",track,repeat?", looped":"");
  current_track=track;
}

void CD::stop(void)
{
  if(!Sounds::active) return;
  if(music!=NULL) {
    Mix_HaltMusic();
    Mix_FreeMusic(music);
    music=NULL;
  }
  current_track=0;
}

void CD::pause(void)
{
  if(music!=NULL) Mix_PauseMusic();
}

void CD::resume(void)
{
  if(music!=NULL) Mix_ResumeMusic();
}

// Port: MIDI songs (the MCI sequencer) were the music of the shareware edition, which the retail
// game, built without SHAREWARE, never plays. Not ported.

char* Song::song;

Song::Song()
{
  song=NULL;
}

Song::~Song()
{
}

void Song::play(char* midi_file,int repeat)
{
  (void)midi_file,repeat;
}

void Song::stop(void)
{
  song=NULL;
}
