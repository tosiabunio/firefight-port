#include "1ss_hdrs.h"
#include <math.h>
#include <SDL.h>
#include <SDL_mixer.h>

// Port: SDL_mixer replaces DirectSound and WaveOut. The voice model is the original's: each
// Sample is one voice (playing it again restarts it), on one of total_channels channels, and a
// sample takes the channel of a playing one with the same or a lower priority when all are busy.
// SDL_mixer channels stand for the DirectSound buffers' playing slots; SOS_WAVE uses channel 0.

static Comm::wpp prev_win_proc=NULL;
static Sample* channels[SOS_MAX_CHANNELS];
static int total_channels;
static int priority=0;
static int extra_debug=0;
static unsigned handle=0;
static unsigned start_time;
static unsigned sample_time;
static unsigned handle_seed=1;

int Sounds::extra_debug=0;
int Sounds::active=0;
int Sounds::active_mode;
int Sounds::app_active=1;

// Port: opens the default audio device at its own rate (was the DirectSound primary buffer,
// whose format the mode chose, or the WaveOut device). Samples are converted to it when loaded.
static int open_audio(void)
{
  if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) {
    WARNING("no sound: SDL audio failed (%s)",SDL_GetError());
    return 0;
  }
#ifdef FF_BROWSER
  // the browser build's music is Ogg Vorbis (1ss_song.cpp)
  if((Mix_Init(MIX_INIT_OGG)&MIX_INIT_OGG)==0)
    WARNING("no music: SDL_mixer has no Ogg Vorbis support (%s)",Mix_GetError());
#else
  if((Mix_Init(MIX_INIT_FLAC)&MIX_INIT_FLAC)==0)
    WARNING("no music: SDL_mixer has no FLAC support (%s)",Mix_GetError());
#endif
  if(Mix_OpenAudioDevice(MIX_DEFAULT_FREQUENCY,AUDIO_S16SYS,2,1024,NULL,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE)!=0) {
    WARNING("no sound: the audio device does not open (%s)",Mix_GetError());
    Mix_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return 0;
  }
  int frequency,channels;
  Uint16 format;
  Mix_QuerySpec(&frequency,&format,&channels);
  ENGINFO("SDL_mixer: %s driver, %dHz, %d output channels",SDL_GetCurrentAudioDriver(),frequency,channels);
  return 1;
}

void Sounds::init(int mode,int required_channels,DWORD _music_line)
{
  try {
    DBG_CHECK(Comm::hwnd!=NULL);
    if((mode==SOS_16_S_22)||(mode==SOS_16_S_11)||(mode==SOS_8_S_22)||(mode==SOS_8_S_11)||(mode==SOS_WAVE)) {
      // port: without an audio device the game runs silent (was a failure)
      if(!open_audio()) mode=SOS_NONE;
    } else if(mode!=SOS_NONE) {
      FAILURE("invalid intialization mode specified");
    }
    priority=0;
    for(int i=0;i<SOS_MAX_CHANNELS;i++) channels[i]=NULL;
    // init mixer
    if((required_channels<=0)||(required_channels>SOS_MAX_CHANNELS))
      FAILURE("invalid number of channels: %d (1..%d allowed)",required_channels,SOS_MAX_CHANNELS);
    total_channels=required_channels;
    if(mode==SOS_WAVE) {
      Mix_AllocateChannels(1);
      ENGINFO("single channel (WaveOut mode)");
    } else if(mode!=SOS_NONE) {
      Mix_AllocateChannels(total_channels);
    }
    ENGINFO("Number of active channels: %d",total_channels);
    // finish initialization and prepare quit handler
    Comm::quit_me(quit,"*sos","log mmu xio");
    active=1;
    active_mode=mode;
    prev_win_proc=Comm::set_window_proc(window_proc);
    Mixer::init(_music_line);
  } catch (Failure) {
    FAILURE("initialization of sound system failed [Sounds::init]");
  }
}

void Sounds::quit(void)
{
  if(!active) return;
  // stop CD
  CD::stop();
  if(active_mode!=SOS_NONE) {
    // port: was the release of the DirectSound objects or the WaveOut device
    Mix_HaltChannel(-1);
    Mixer::quit();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }
  active=0;
}

int logvol [101] =
{
  -4852,-4708,-4567,-4429,-4293,-4160,-4030,-3903,-3778,-3657,-3537,-3421,-3307,-3195,
  -3086,-2980,-2876,-2774,-2675,-2579,-2484,-2392,-2302,-2215,-2130,-2047,-1966,-1887,
  -1811,-1736,-1664,-1594,-1525,-1459,-1395,-1332,-1272,-1213,-1156,-1101,-1048,-996,
  -946,-898,-852,-807,-764,-722,-682,-643,-606,-570,-536,-503,-472,-442,-413,-385,-359,
  -334,-310,-287,-266,-245,-226,-208,-190,-174,-159,-144,-131,-118,-106,-95,-85,-75,
  -67,-59,-51,-44,-38,-33,-28,-23,-19,-16,-13,-10,-8,-6,-4,-3,-2,-1,-1,0,0,0,0,0,0
};

int VOLUME_FORMULA(int vol)
{
  DBG_CHECK(((vol*100)/VOLUME_MAX)<=100);
  return logvol[(vol*100)/VOLUME_MAX];
}

// Port: DirectSound volumes and pans are attenuations in hundredths of a decibel; SDL_mixer takes
// linear gains.
static double gain(int hundredths)
{
  return pow(10.0,hundredths/2000.0);
}

// Port: was SetVolume and SetPan on the sample's buffer. A positive pan attenuates the left
// channel, a negative one the right.
static void set_volume_and_pan(int channel,int volume,int panning)
{
  Mix_Volume(channel,(int)(MIX_MAX_VOLUME*gain(VOLUME_FORMULA(volume))+0.5));
  Uint8 left=255,right=255;
  if(panning>0) left=(Uint8)(255*gain(-panning)+0.5);
  if(panning<0) right=(Uint8)(255*gain(panning)+0.5);
  Mix_SetPanning(channel,left,right);
}

// Port: was GetStatus()==DSBSTATUS_PLAYING on the channel's buffer. A paused channel counts as
// playing, as a buffer did while the primary buffer was stopped.
static int channel_playing(int channel)
{
  return Mix_Playing(channel)!=0;
}

int Sounds::playing(Sample* sample)
{
  if(!active) return 0;
  switch(active_mode) {
  case SOS_NONE:
  case SOS_WAVE:
    // check if current sample is still playing
    if(priority!=0) {
      if(GetTickCount()<start_time) start_time=0;
      unsigned elapsed=GetTickCount()-start_time;
      if(elapsed>sample_time) {
        DBG_MESSAGE("stopped sample detected in Sounds::playing");
        priority=0;
      }
    }
    if(sample->handle==handle) return priority!=0;
  default:
    // decode channel number from handle
    unsigned channel=sample->handle>>28;
    if(channels[channel]!=NULL) {
       // handle identifies the sample occupying this channel
      if(channels[channel]->handle==sample->handle) {
        // check playing status
        return channel_playing(channel);
      }
    }
  }
  return 0;
}

//#if HI_DEBUG
#define EXTRA_ON if(extra_debug) {
#define EXTRA_OFF }
//#else
//#define EXTRA_ON #if 0
//#define EXTRA_OFF #endif
//#endif

void Sounds::play(Sample* sample,int panning,int volume)
{
  int channel;
  if((!active)||(!app_active)) return;
  switch(active_mode) {
  case SOS_NONE:
  case SOS_WAVE:
    // check if current sample is still playing
    if(priority!=0) {
      if(GetTickCount()<start_time) start_time=0;
      unsigned elapsed=GetTickCount()-start_time;
      if(elapsed>sample_time) {
        DBG_MESSAGE("stopped sample detected in Sounds::play");
        priority=0;
      }
    }
    if(sample->priority>=priority) {
      DBG_MESSAGE("higher priority detected");
      // new sample has higher priority than recently played sample
      if((active_mode==SOS_WAVE)&&(volume>19662)) {
        Mix_PlayChannel(0,sample->chunk,0);  // was waveOutReset and waveOutWrite
      }
      sample_time=sample->time;
      start_time=GetTickCount();
      // store current sample priority
      priority=sample->priority;
      // set unique play handle
      sample->handle=handle_seed++;
      handle=sample->handle;
    } else {
      DBG_MESSAGE("lower priority detected");
    }
    break;
  default:
    channel=sample->handle>>28;
    // check if sample has already been played on some channel
    if(channels[channel]!=NULL) {
      if(channels[channel]->handle==sample->handle) {
        if(!channel_playing(channel)) {
          channels[channel]->volume=0;
        }
        //check volume priority
        if(volume<channels[channel]->volume) return;
        goto play_on_channel;
      }
    }
    //find free channel
    for(channel=0;channel<total_channels;channel++) {
      // free channel if sample has stopped
      if(channels[channel]!=NULL) {
        if(!channel_playing(channel)) {
          channels[channel]=NULL;
        }
      }
      // break if channel is free
      if(channels[channel]==NULL) break;
    }
    if(channel==total_channels) {
      // no more free channels, replace sample with lower or same priority
      for(channel=0;channel<total_channels;channel++) {
        if(channels[channel]->priority<=sample->priority) {
          Mix_HaltChannel(channel);
          channels[channel]=NULL;
          break;
        }
      }
    }
    if(channel==total_channels) {
      return; // all channels have higher priority
    }
    //play sample on free channel [channel]
    channels[channel]=sample;
play_on_channel:
    set_volume_and_pan(channel,volume,panning);
    // was SetCurrentPosition(0) and Play, restoring a lost buffer
    if(Mix_PlayChannel(channel,channels[channel]->chunk,0)<0) return;
    // store last volume played
    channels[channel]->volume=volume;
    // get unique play handle and encode channel number in it
    sample->handle=(channel<<28)|handle_seed;
    // prepare new handle for subsequent use
    handle_seed=((handle_seed+1)&0x0FFFFFFF)==0?1:handle_seed+1;
   //*************************************
  }
}

void Sounds::stop(Sample* sample)
{
  if(!active) return;
  switch(active_mode) {
  case SOS_NONE:
    if(sample->handle==handle) priority=0;
    break;
  case SOS_WAVE:
    if(sample->handle==handle) {
      Mix_HaltChannel(0);  // was waveOutReset
      priority=0;
    }
    break;
  default:
    // decode channel number from handle
    unsigned channel=sample->handle>>28;
    if(channels[channel]!=NULL) {
      if(channels[channel]->handle==sample->handle) {
        // handle identifies the sample occupying this channel, stop it
        Mix_HaltChannel(channel);
        channels[channel]=NULL;
      }
    }
  }
}

// Port: Sample::free calls this, so that no channel refers to a sample whose chunk is gone.
void Sounds::forget(Sample* sample)
{
  for(int channel=0;channel<SOS_MAX_CHANNELS;channel++) {
    if(channels[channel]==sample) {
      Mix_HaltChannel(channel);
      channels[channel]=NULL;
    }
  }
}

void Sounds::stop_all(void)
{
  if(!active) return;
  switch(active_mode) {
  case SOS_NONE:
    priority=0;
    break;
  case SOS_WAVE:
    Mix_HaltChannel(0);  // was waveOutReset
    priority=0;
    break;
  default:
    // stop all samples on all channels
    for(int channel=0;channel<total_channels;channel++) {
      if(channels[channel]!=NULL) {
        Mix_HaltChannel(channel);
        channels[channel]=NULL;
      }
    }
  }
}

int Sounds::window_proc (int *result, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  int processed=0;
  if(!active) return processed;
  if(prev_win_proc!=NULL) {
    // call other message handlers in chain
    if(!prev_win_proc(result,hwnd,message,wparam,lparam)) {
      switch(message)
      {
        case WM_ACTIVATEAPP:
          app_active=LOWORD(wparam);
          // port: the sounds and the music pause and resume (was a stop of the primary buffer
          // and of the CD, which restarted its track from the beginning). The MCI notifications
          // that looped the CD track and the system mixer's change messages are gone: SDL_mixer
          // loops the music itself and the volumes are the game's own.
          if(app_active) {
            // application has been activated
            if((active_mode!=SOS_NONE)&&(active_mode!=SOS_WAVE)) Mix_Resume(-1);
            CD::resume();
          } else {
            // application has been deactivated
            CD::pause();
            // stop all sounds playing
            if(active_mode==SOS_WAVE) {
              Mix_HaltChannel(0);
              priority=0;
            }
            if((active_mode!=SOS_NONE)&&(active_mode!=SOS_WAVE)) Mix_Pause(-1);
          }
          break;
      }
    }
  }
  return(processed);
}
