#include "1ss_hdrs.h"

#define OLD_LOGGING_STYLE 0
#define NEW_LOGGING_STYLE (1-OLD_LOGGING_STYLE)

Sample::Sample()
{
  buffer=NULL;
  handle=0;
  panning=PAN_CENTER;
  volume=VOLUME_MAX;
  samples=NULL;
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

void Sample::load(char const* long_name)
{
  if(!Sounds::active) return;
  DBG_CHECK(buffer==NULL);
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
      samples==NULL;
    } else if(Sounds::active_mode==SOS_WAVE) {
      if(format.nChannels!=1) FAILURE ("invalid sample format (not mono) for WaveOut");
      if(format.nSamplesPerSec!=11025) FAILURE ("invalid sample format (not 11025Hz) for WaveOut");
      if(format.wBitsPerSample!=8) FAILURE ("invalid sample format (not 8-bit) for WaveOut");
      samples=Heap::alloc(size,"samples");
      File::read(samples,size);
    } else {
      DSBUFFERDESC buffer_description;
      buffer_description.dwSize=sizeof(DSBUFFERDESC);
      buffer_description.dwFlags=DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN|DSBCAPS_LOCSOFTWARE;
      buffer_description.dwBufferBytes=size;
      buffer_description.dwReserved=0;
      buffer_description.lpwfxFormat=&format;
      int res=Sounds::sound_driver->CreateSoundBuffer(&buffer_description,&buffer,NULL);
      if(res==DSERR_OUTOFMEMORY) FAILURE2(SOS_OUT_OF_MEMORY,"static sound buffer creation failed");
      if(res!=DS_OK) FAILURE2(SOS_ERROR,"static sound buffer creation failed");
      char* ptr1;
      char* ptr2;
      unsigned long bytes1;
      unsigned long bytes2;
      res=buffer->Lock(0,size,&ptr1,&bytes1,&ptr2,&bytes2,0);
      if(res==DSERR_BUFFERLOST) {
        buffer->Restore();
        res=buffer->Lock(0,size,&ptr1,&bytes1,&ptr2,&bytes2,0);
      }
      if(res!=DS_OK) FAILURE2(SOS_ERROR,"sound buffer lock failed");
      DBG_CHECK(ptr2==NULL);
      File::read(ptr1,size);
      buffer->Unlock((void*)ptr1,bytes1,(void*)ptr2,bytes2);
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
  if((Sounds::active_mode==SOS_WAVE)&&(samples!=NULL)) {
    Heap::free(samples);
    samples=NULL;
  } else {
    if((buffer!=NULL)&&(Sounds::active)) {
      buffer->Stop();
      buffer->Release();
    }
    buffer=NULL;
  }
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

