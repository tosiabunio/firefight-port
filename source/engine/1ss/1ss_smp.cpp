#include "1ss_hdrs.h"
#include <SDL.h>
#include <SDL_mixer.h>

#define OLD_LOGGING_STYLE 0
#define NEW_LOGGING_STYLE (1-OLD_LOGGING_STYLE)

Sample::Sample()
{
  chunk=NULL;
  handle=0;
  panning=PAN_CENTER;
  volume=VOLUME_MAX;
}

Sample::~Sample()
{
  free();
}

#pragma pack(1)

static struct {
  FOURCC id;
  DWORD  size;  // bez chunk header!
} chunk_header;

#define FORMAT_SIZE 18

static struct {
  union {
    struct {
      char chunk[FORMAT_SIZE];
    };
    struct {
      WORD  format_tag; //2
      WORD  channels;   //2
      DWORD frequency;  //4
      DWORD buffer;     //4 
      WORD  align;      //2
      WORD  bits;       //2 or 4
    };
  };
} wave_format;

#pragma pack()

static void put16(Uint8* p,unsigned v) { p[0]=(Uint8)v; p[1]=(Uint8)(v>>8); }
static void put32(Uint8* p,unsigned v) { put16(p,v&0xFFFF); put16(p+2,v>>16); }

// Port: reads the sample data into a WAV image for SDL_mixer, which converts it to the audio
// device's format (was a DirectSound buffer in the sample's own format, or the data itself for
// WaveOut).
static Mix_Chunk* load_chunk(const WAVEFORMATEX& format,unsigned size)
{
  Uint8* wav=(Uint8*)SDL_malloc(44+size);
  if(wav==NULL) FAILURE2(SOS_OUT_OF_MEMORY,"sample memory allocation failed");
  memcpy(wav,"RIFF",4); put32(wav+4,36+size); memcpy(wav+8,"WAVEfmt ",8); put32(wav+16,16);
  put16(wav+20,format.wFormatTag); put16(wav+22,format.nChannels);
  put32(wav+24,format.nSamplesPerSec); put32(wav+28,format.nAvgBytesPerSec);
  put16(wav+32,format.nBlockAlign); put16(wav+34,format.wBitsPerSample);
  memcpy(wav+36,"data",4); put32(wav+40,size);
  try {
    File::read(wav+44,size);
  } catch(Failure) {
    SDL_free(wav);
    throw;
  }
  Mix_Chunk* chunk=Mix_LoadWAV_RW(SDL_RWFromConstMem(wav,44+size),1);
  SDL_free(wav);
  if(chunk==NULL) FAILURE2(SOS_ERROR,"sample conversion failed (%s)",Mix_GetError());
  return chunk;
}

void Sample::load(char const* long_name)
{
  if(!Sounds::active) return;
  DBG_CHECK(chunk==NULL);
  char filename[Text::max_label+1];
  DBG_CHECK(strlen(long_name)<=Text::max_label);
  strcpy(filename,long_name);
  strupr(filename);
  try {
#if OLD_LOGGING_STYLE
    MESSAGE("loading sample %s...",filename);
#endif
    File::open(long_name);
    File::read((char*)&chunk_header,sizeof(chunk_header));
    if(chunk_header.id!=FOURCC_RIFF) FAILURE("bad sample format - not RIFF file");
    DWORD wave;
    File::read((char*)&wave,sizeof(wave));
    if(wave!=mmioFOURCC('W','A','V','E')) FAILURE("bad sample format - not WAVE file");
    int fmt_fnd;
    do {
      File::read((char*)&chunk_header,sizeof(chunk_header));
      fmt_fnd=chunk_header.id==mmioFOURCC('f','m','t',' ');
      if(!fmt_fnd) File::ignore(chunk_header.size);
    } while(!fmt_fnd);
    CHECK(sizeof(wave_format)==FORMAT_SIZE);
    memset(&wave_format,0,FORMAT_SIZE);
    if(chunk_header.size>FORMAT_SIZE) FAILURE("format chunk too big (%d bytes)",chunk_header.size);
    File::read((char*)&wave_format,chunk_header.size);
    if(wave_format.format_tag!=WAVE_FORMAT_PCM) FAILURE("bad sample format - not PCM");
    if(wave_format.channels>2) FAILURE("bad sample format - too many channels: %d",wave_format.channels);
    if((wave_format.bits!=8)&&(wave_format.bits!=16)) FAILURE("bad sample format - neither 8 nor 16-bit");
    int dta_fnd;
    do {
      File::read((char*)&chunk_header,sizeof(chunk_header));
      dta_fnd=chunk_header.id==mmioFOURCC('d','a','t','a');
      if(!dta_fnd) File::ignore(chunk_header.size);
    } while(!dta_fnd);
    size=chunk_header.size;
    format.wFormatTag=wave_format.format_tag;
    format.nChannels=wave_format.channels;
    format.nSamplesPerSec=wave_format.frequency;
    format.nAvgBytesPerSec=wave_format.buffer;
    DBG_CHECK((format.nAvgBytesPerSec/1000)!=0);
    format.nBlockAlign=wave_format.align;
    format.wBitsPerSample=wave_format.bits;
    format.cbSize=0;
    time=(size*1000)/format.nAvgBytesPerSec;
    if(Sounds::active_mode==SOS_NONE) {
      // no data needed (was "samples==NULL;", a comparison with no effect)
    } else if(Sounds::active_mode==SOS_WAVE) {
      if(format.nChannels!=1) FAILURE ("invalid sample format (not mono) for WaveOut");
      if(format.nSamplesPerSec!=11025) FAILURE ("invalid sample format (not 11025Hz) for WaveOut");
      if(format.wBitsPerSample!=8) FAILURE ("invalid sample format (not 8-bit) for WaveOut");
      chunk=load_chunk(format,size);
    } else {
      chunk=load_chunk(format,size);
    }
    char flag=File::info.flags[0]==0?'5':File::info.flags[0];
    File::close();
    priority=(flag>='1')&&(flag<='9')?flag-'0':5;
#if NEW_LOGGING_STYLE
    if(!Comm::production) MESSAGE("sample %s, %s %s %dHz, %d bytes, %d",filename,wave_format.bits==8?"8-bit":"16-bit"
            ,wave_format.channels==2?"stereo":"mono"
            ,wave_format.frequency,size,priority);
#endif
#if OLD_LOGGING_STYLE
    MESSAGE("...format: %s, %s, %dHz (%d bytes), priority=%d",wave_format.bits==8?"8-bit":"16-bit"
            ,wave_format.channels==2?"stereo":"mono"
            ,wave_format.frequency,size,priority);
#endif
    name=long_name;
  } catch(Failure) {
    FAILURE("sample load failed [Sample::load(%s)]",long_name);
  }
}

void Sample::free(void)
{
  // port: was the release of the DirectSound buffer, or of the data for WaveOut
  if((chunk!=NULL)&&(Sounds::active)) {
    Sounds::forget(this);
    Mix_FreeChunk(chunk);
  }
  chunk=NULL;
}

void Sample::play(int _panning,int _volume)
{
  if(!Sounds::active) return;
  if(_panning>PAN_RIGHT) _panning=PAN_RIGHT;
  if(_panning<PAN_LEFT) _panning=PAN_LEFT;
  panning=_panning;
  if(_volume>VOLUME_MAX) _volume=VOLUME_MAX;
  if(_volume<VOLUME_MIN) _volume=VOLUME_MIN;
  Sounds::play(this,panning,_volume);
}

void Sample::stop(void)
{
  if(!Sounds::active) return;
  Sounds::stop(this);
}

int Sample::playing(void)
{
  if(!Sounds::active) return 0;
  return Sounds::playing(this);
}

