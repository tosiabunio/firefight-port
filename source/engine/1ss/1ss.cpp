#include "1ss_hdrs.h"

static Comm::wpp prev_win_proc=NULL;
static IDirectSoundBuffer* primary_buffer;
static Sample* channels[SOS_MAX_CHANNELS];
static WAVEHDR waveout_header;
static HWAVEOUT waveout_handle;
static int total_channels;
static int start_sound_volume;
static int start_music_volume;
static int song=0;
static char* midi_song=NULL;
static int mixer_active=0;
static int priority=0;
static int extra_debug=0;
static unsigned handle=0;
static unsigned start_time;
static unsigned sample_time;
static unsigned handle_seed=1;
static HINSTANCE lib_handle=NULL;
typedef HRESULT (WINAPI *DSC)(GUID FAR* lpGUID,LPDIRECTSOUND* ppDS,IUnknown FAR *pUnkOuter);

int Sounds::extra_debug=0;
int Sounds::active=0;
int Sounds::active_mode;
int Sounds::app_active=1;
IDirectSound* Sounds::sound_driver;

void Sounds::init(int mode,int required_channels,DWORD _music_line)
{
  try {
    DBG_CHECK(Comm::hwnd!=NULL);
    if((mode==SOS_16_S_22)||(mode==SOS_16_S_11)||(mode==SOS_8_S_22)||(mode==SOS_8_S_11)) {
      // the system uses Direct Sound
      // load DSOUND.DLL
      lib_handle=LoadLibrary("DSOUND.DLL");
      if(lib_handle==NULL)
        FAILURE2(SOS_DS_NOT_INSTALLED,"initialization of DirectSound failed (DSOUND.DLL load failed)");
      // access Direct Sound Create function from DLL
      DSC dsc=(DSC)GetProcAddress(lib_handle,"DirectSoundCreate");
      if(dsc==NULL) FAILURE2(SOS_DS_NOT_INSTALLED,"initialization of DirectSound failed (GetProcAddress failed)");
      // create Direct Sound object (sound driver)
      if(dsc(NULL,&sound_driver,NULL)!=DS_OK)
        FAILURE2(SOS_CANNOT_INITIALIZE,"initialization of DirectSound failed (DS object create failed)");
      DSCAPS capabilities;
      capabilities.dwSize=sizeof(capabilities);
      // query Direct Sound object capabilities
      if(sound_driver->GetCaps(&capabilities)==DS_OK) {
        SYSINFO("Direct Sound system information:");
#define FLAG(mask,text) if(capabilities.dwFlags&mask) SYSINFO(text)
        FLAG(DSCAPS_CONTINUOUSRATE, "  supports continuous sample rate");
        FLAG(DSCAPS_EMULDRIVER,     "  uses emulation driver");
        FLAG(DSCAPS_CERTIFIED,      "  Microsoft certified");
        FLAG(DSCAPS_PRIMARY16BIT,   "  primary 16-bit buffer");
        FLAG(DSCAPS_PRIMARY8BIT,    "  primary 8-bit buffer");
        FLAG(DSCAPS_PRIMARYSTEREO,  "  primary stereo buffer");
        FLAG(DSCAPS_PRIMARYMONO,    "  primary mono buffer");
        FLAG(DSCAPS_SECONDARY16BIT, "  hardware secondary 16-bit buffers");
        FLAG(DSCAPS_SECONDARY8BIT,  "  hardware secondary 8-bit buffers");
        FLAG(DSCAPS_SECONDARYSTEREO,"  hardware secondary stereo buffers");
        FLAG(DSCAPS_SECONDARYMONO,  "  hardware secondary mono buffers");
        SYSINFO("  sound card memory=%dkb(%dkb free)",kilo(capabilities.dwTotalHwMemBytes),kilo(capabilities.dwFreeHwMemBytes));
        SYSINFO("  transfer rate=%dkb/s",capabilities.dwUnlockTransferRateHwBuffers);
        SYSINFO("  cpu overhead=%d%%",capabilities.dwPlayCpuOverheadSwBuffers);
      }
      // set exclusive cooperative level (no cooperation at all :)
      if(sound_driver->SetCooperativeLevel(Comm::hwnd,DSSCL_EXCLUSIVE)!=DS_OK) {
        sound_driver->Release();
        sound_driver=NULL;
        FAILURE2(SOS_CANNOT_INITIALIZE,"initialization of DirectSound failed (cooperative level cannot be set)");
      }
      // prepare primary buffer description
      DSBUFFERDESC primary_buffer_description;
      primary_buffer_description.dwSize=sizeof(DSBUFFERDESC);
      primary_buffer_description.dwFlags=DSBCAPS_PRIMARYBUFFER;
      primary_buffer_description.dwBufferBytes=0;
      primary_buffer_description.dwReserved=0;
      primary_buffer_description.lpwfxFormat=NULL;
      // create primary buffer
      int res=sound_driver->CreateSoundBuffer(&primary_buffer_description,&primary_buffer,NULL);
      if(res==DSERR_OUTOFMEMORY) {
        sound_driver->Release();
        sound_driver=NULL;
        FAILURE2(SOS_OUT_OF_MEMORY,"initialization of DirectSound failed (create primary buffer failed - out of memory)");
      }
      if(res!=DS_OK) {
        sound_driver->Release();
        sound_driver=NULL;
        FAILURE2(SOS_CANNOT_INITIALIZE,"initialization of DirectSound failed (create primary buffer failed)");
      }
      WAVEFORMATEX primary_buffer_format;
      // change format of primary buffer to one of preffered formats
      switch(mode) {
      case SOS_16_S_22:
        // change to 16-bit, stereo, 22.1 kHz
        primary_buffer_format.wFormatTag=WAVE_FORMAT_PCM;
        primary_buffer_format.nChannels=2;
        primary_buffer_format.nSamplesPerSec=22050;
        primary_buffer_format.nAvgBytesPerSec=22050*2*2;
        primary_buffer_format.nBlockAlign=4;
        primary_buffer_format.wBitsPerSample=16;
        primary_buffer_format.cbSize=0;
        if(primary_buffer->SetFormat(&primary_buffer_format)==DS_OK) {
          ENGINFO("Direct Sound primary buffer: 16-bit, stereo, 22050Hz");
          goto buffer_set;
        }
        // try another format if this one fails (no break!)
      case SOS_16_S_11:
        // change to 16-bit, stereo, 11.05 kHz
        primary_buffer_format.wFormatTag=WAVE_FORMAT_PCM;
        primary_buffer_format.nChannels=2;
        primary_buffer_format.nSamplesPerSec=11025;
        primary_buffer_format.nAvgBytesPerSec=11025*2*2;
        primary_buffer_format.nBlockAlign=4;
        primary_buffer_format.wBitsPerSample=16;
        primary_buffer_format.cbSize=0;
        if(primary_buffer->SetFormat(&primary_buffer_format)==DS_OK) {
          ENGINFO("Direct Sound primary buffer: 16-bit, stereo, 11025Hz");
          goto buffer_set;
        }
        // try another format if this one fails (no break!)
      case SOS_8_S_22:
        // change to 8-bit, stereo, 22.1 kHz
        primary_buffer_format.wFormatTag=WAVE_FORMAT_PCM;
        primary_buffer_format.nChannels=2;
        primary_buffer_format.nSamplesPerSec=22050;
        primary_buffer_format.nAvgBytesPerSec=22050*2;
        primary_buffer_format.nBlockAlign=2;
        primary_buffer_format.wBitsPerSample=8;
        primary_buffer_format.cbSize=0;
        if(primary_buffer->SetFormat(&primary_buffer_format)==DS_OK) {
          ENGINFO("Direct Sound primary buffer: 8-bit, stereo, 22050Hz");
          goto buffer_set;
        }
        // try another format if this one fails (no break!)
      case SOS_8_S_11:
        // change to 8-bit, stereo, 11.05 kHz
        primary_buffer_format.wFormatTag=WAVE_FORMAT_PCM;
        primary_buffer_format.nChannels=2;
        primary_buffer_format.nSamplesPerSec=11025;
        primary_buffer_format.nAvgBytesPerSec=11025*2;
        primary_buffer_format.nBlockAlign=2;
        primary_buffer_format.wBitsPerSample=8;
        primary_buffer_format.cbSize=0;
        if(primary_buffer->SetFormat(&primary_buffer_format)==DS_OK) {
          ENGINFO("Direct Sound primary buffer: 8-bit, stereo, 11025Hz");
          goto buffer_set;
        }
      default:
        // device doesn't accept any preferred format, stays in its default (whatever it is)
        MESSAGE("unable to set preferred buffer format");
      }
buffer_set:
      // play primary buffer in loop
      res=primary_buffer->Play(0,0,DSBPLAY_LOOPING);
      if(res==DSERR_BUFFERLOST) {
        primary_buffer->Restore();
        res=primary_buffer->Play(0,0,DSBPLAY_LOOPING);
      }
      if(res!=DS_OK) {
        primary_buffer->Release();
        primary_buffer=NULL;
        sound_driver->Release();
        sound_driver=NULL;
        FAILURE2(SOS_CANNOT_INITIALIZE,"initialization of DirectSound failed (primary buffer does not play)");
      }
      // initialize channels array
      for(int i=0;i<SOS_MAX_CHANNELS;i++) channels[i]=NULL;
    } else if(mode==SOS_WAVE) {
      // the system uses WaveOut
      // set wave out format (8-bit, mono, 11kHz)
      WAVEFORMATEX waveout_format;
      waveout_format.wFormatTag=WAVE_FORMAT_PCM;
      waveout_format.nChannels=1;
      waveout_format.nSamplesPerSec=11025;
      waveout_format.nAvgBytesPerSec=11025;
      waveout_format.nBlockAlign=1;
      waveout_format.wBitsPerSample=8;
      waveout_format.cbSize=0;
      // initialize wave out device
      MMRESULT res=waveOutOpen((LPHWAVEOUT)&waveout_handle,WAVE_MAPPER,&waveout_format,0,0,CALLBACK_NULL);
      if(res!=MMSYSERR_NOERROR)
        FAILURE2(SOS_CANNOT_INITIALIZE,"initialization of WaveOut failed");
      memset(&waveout_header,0,sizeof(waveout_header));
      waveOutReset(waveout_handle);
      priority=0;
      ENGINFO("WaveOut: single channel, mono sounds, 11025Hz");
    } else if(mode==SOS_NONE) {
      priority=0;
    } else {
      FAILURE("invalid intialization mode specified");
    }
    // init mixer
    if((required_channels<=0)||(required_channels>SOS_MAX_CHANNELS))
      FAILURE("invalid number of channels: %d (1..%d allowed)",required_channels,SOS_MAX_CHANNELS);
    total_channels=required_channels;
    ENGINFO("Number of active channels: %d",total_channels);
    // finish initialization and prepare quit handler
    Comm::quit_me(quit,"*sos","log mmu xio");
    active=1;
    active_mode=mode;
    prev_win_proc=Comm::set_window_proc(window_proc);
    Mixer::init(_music_line);
    Mixer::save_volumes();
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
    if(active_mode!=SOS_WAVE) {
      if(primary_buffer!=NULL) {
        // stop then release primary buffer
        primary_buffer->Stop();
        primary_buffer->Release();
      }
      // release Direct Sound object
      if(sound_driver!=NULL) sound_driver->Release();
    } else {
      waveOutReset(waveout_handle);
      waveOutClose(waveout_handle);
    }
    // restore mixer settings
    Mixer::quit();
    if(lib_handle!=NULL) FreeLibrary(lib_handle);
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
        unsigned long status;
        channels[channel]->buffer->GetStatus(&status);
        return status==DSBSTATUS_PLAYING;
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
        if(priority>0) waveOutReset(waveout_handle);
        waveout_header.lpData=(char*)sample->samples;
        waveout_header.dwBufferLength=sample->size;
        waveout_header.dwFlags=0;
        waveOutPrepareHeader(waveout_handle,&waveout_header,sizeof(waveout_header));
        waveOutWrite(waveout_handle,&waveout_header,sizeof(waveout_header));
        waveOutUnprepareHeader(waveout_handle,&waveout_header,sizeof(waveout_header));
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
        unsigned long status;
        channels[channel]->buffer->GetStatus(&status);
        if(status!=DSBSTATUS_PLAYING) {
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
        unsigned long status;
        channels[channel]->buffer->GetStatus(&status);
        if(status!=DSBSTATUS_PLAYING) {
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
          channels[channel]->buffer->Stop();
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
    channels[channel]->buffer->SetPan(panning);
    channels[channel]->buffer->SetVolume(VOLUME_FORMULA(volume));
    channels[channel]->buffer->SetCurrentPosition(0);
    if(channels[channel]->buffer->Play(0,0,0)==DSERR_BUFFERLOST) {
      if(channels[channel]->buffer->Restore()!=DS_OK) return;
      channels[channel]->buffer->Play(0,0,0);
    }
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
      waveOutReset(waveout_handle);
      priority=0;
    }
    break;
  default:
    // decode channel number from handle
    unsigned channel=sample->handle>>28;
    if(channels[channel]!=NULL) {
      if(channels[channel]->handle==sample->handle) {
        // handle identifies the sample occupying this channel, stop it
        channels[channel]->buffer->Stop();
        channels[channel]=NULL;
      }
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
    waveOutReset(waveout_handle);
    priority=0;
    break;
  default:
    // stop all samples on all channels
    for(int channel=0;channel<total_channels;channel++) {
      if(channels[channel]!=NULL) {
        channels[channel]->buffer->Stop();
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
          if(app_active) {
            // application has been activated, play current CD track from the beginning
            if((active_mode!=SOS_NONE)&&(active_mode!=SOS_WAVE)) primary_buffer->Play(0,0,DSBPLAY_LOOPING);
            CD::play(CD::current_track);
            Song::play(Song::song);
          } else {
            // application has been deactivated, stop current CD track
            int tmp=CD::current_track;
            CD::stop();
            CD::current_track=tmp;
            char* stmp=Song::song;
            Song::stop();
            Song::song=stmp;
            // stop all sounds playing
            if(active_mode==SOS_WAVE) {
              waveOutReset(waveout_handle);
              priority=0;
            }
            if((active_mode!=SOS_NONE)&&(active_mode!=SOS_WAVE)) primary_buffer->Stop();
          }
          break;
        case MM_MCINOTIFY:
          if((lparam==(LONG)CD::DeviceID)&&(wparam==(WPARAM)MCI_NOTIFY_SUCCESSFUL)) {
            DBG_MESSAGE("MM_MCINOTIFY+MCI_NOTIFY_SUCCESSFUL received");
            // current CD track has been played to its end, restart it.
            if(CD::current_track>0) CD::play(CD::current_track);
            *result=0;
            processed=1;
          }
          if((lparam==(LONG)Song::DeviceID)&&(wparam==(WPARAM)MCI_NOTIFY_SUCCESSFUL)) {
            // current CD track has been played to its end, restart it.
            Song::play(Song::song);
            processed=1;
            *result=0;
            DBG_MESSAGE("MIDI song restarted");
          }
          break;
        case MM_MIXM_CONTROL_CHANGE:
          // other application has changed mixer settings
          Mixer::save_volumes();
//          if(!Mixer::internal_change) Mixer::save_volumes();
//          Mixer::internal_change=0;
          break;
      }
    }
  }
  return(processed);
}

