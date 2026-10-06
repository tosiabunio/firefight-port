#include "1ss_hdrs.h"
#include <SDL_mixer.h>
#ifdef FF_BROWSER
#include <emscripten.h>
#endif

// Port: the CD audio tracks are FLAC files, <music>/track02.flac, track03.flac... (was MCI CD
// audio), or Ogg Vorbis files (track02.ogg...) in the browser build. Track numbers count audio
// tracks only, as the original did once it had skipped the CD's data track 1: track N is the file
// numbered N+1. SDL_mixer streams the file; playing it with repeat loops it, as the MCI
// notification that restarted a finished track did. With sound off (SOS_NONE) nothing plays; the
// original still played the CD then.
//
// An empty track file is a track still on its way: the browser build downloads the music after
// the start (web/pre.js). Such a track plays when it arrives, if it is still wanted then.

int CD::tracks;
int CD::current_track;
static const int data_skip=1;
static char music_dir[_MAX_PATH+1]="music";
static Mix_Music* music=NULL;
static int pending_track=0;   // the track to play when it arrives, and its repeat flag
static int pending_repeat=0;

// Finds the track's file: its size, or -1 when there is none.
static long track_file(char* path,int track)
{
  static const char* const types[]={"flac","ogg"};
  for(const char* type : types) {
    sprintf(path,"%s/track%02d.%s",music_dir,track+data_skip,type);
    FILE* file=fopen(path,"rb");
    if(file==NULL) continue;
    fseek(file,0,SEEK_END);
    long size=ftell(file);
    fclose(file);
    return size;
  }
  return -1;
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
  while(track_file(path,tracks+1)>=0)
    tracks++;
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
  if(track_file(path,track)==0) {
    MESSAGE("music: track %d has not arrived yet",track);
    pending_track=track;
    pending_repeat=repeat;
    return;
  }
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
  pending_track=0;
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

#ifdef FF_BROWSER
// The page calls this when a track has downloaded.
extern "C" EMSCRIPTEN_KEEPALIVE void ff_music_arrived(void)
{
  if(pending_track!=0)
    CD::play(pending_track,pending_repeat);
}
#endif

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
  (void)midi_file; (void)repeat;
}

void Song::stop(void)
{
  song=NULL;
}
