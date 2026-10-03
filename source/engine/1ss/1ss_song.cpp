#include "1ss_hdrs.h"

UINT CD::DeviceID;
int CD::tracks;
int CD::current_track;
static int data_skip=0;

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
  DWORD dwReturn;
  MCI_OPEN_PARMS mciOpenParms;
  MCI_STATUS_PARMS mciStatusParams;
  if(!Sounds::active) return;
  // Open the cdaudio device (simple) with wait.
  mciOpenParms.lpstrDeviceType="cdaudio";
  dwReturn=mciSendCommand(NULL,MCI_OPEN,MCI_OPEN_TYPE|MCI_WAIT,(DWORD_PTR)(LPVOID)&mciOpenParms);
  if(dwReturn) return;
  // Get the device ID.
  DeviceID=mciOpenParms.wDeviceID;
  // Get number of tracks
  mciStatusParams.dwItem=MCI_STATUS_NUMBER_OF_TRACKS;
  dwReturn=mciSendCommand(DeviceID,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD_PTR)(LPVOID)&mciStatusParams);
  if(!dwReturn) tracks=(int)mciStatusParams.dwReturn;
  if(tracks>0) {
    MCI_STATUS_PARMS mciStatusParams;
    // check track type
    mciStatusParams.dwItem=MCI_CDA_STATUS_TYPE_TRACK;
    mciStatusParams.dwTrack=1;
    if(!mciSendCommand(DeviceID,MCI_STATUS,MCI_STATUS_ITEM|MCI_TRACK|MCI_WAIT,(DWORD_PTR)(LPVOID)&mciStatusParams)) {
      if(mciStatusParams.dwReturn!=MCI_CDA_TRACK_AUDIO) {
        tracks--;
        data_skip=1;
      }
    }
  }
  // close cdaudio device
  MCI_GENERIC_PARMS mciGenericParms;
  mciSendCommand(DeviceID,MCI_CLOSE,MCI_WAIT,(DWORD_PTR)(LPVOID)&mciGenericParms);
}

void CD::play(int track,int repeat)
{
  if((!Sounds::active)||(!Sounds::app_active)) return;
  if(track>tracks) return;
  if(track==0) return;
  CD::stop();
  DWORD dwReturn;
  MCI_OPEN_PARMS mciOpenParms;
  MCI_PLAY_PARMS mciPlayParms;
  MCI_SET_PARMS mciSetParms;
  if(track>tracks) return;
  // open cdaudio device
  mciOpenParms.lpstrDeviceType="cdaudio";
  dwReturn=mciSendCommand(NULL,MCI_OPEN,MCI_OPEN_TYPE|MCI_WAIT,(DWORD_PTR)(LPVOID)&mciOpenParms);
  if(dwReturn) return;
  // Get the device ID.
  DeviceID=mciOpenParms.wDeviceID;
  // Set the current time format to TMSF.
  // This should be the default, but do it anyway.
  mciSetParms.dwTimeFormat=MCI_FORMAT_TMSF;
  dwReturn=mciSendCommand(DeviceID, MCI_SET,MCI_WAIT|MCI_SET_TIME_FORMAT,(DWORD_PTR)(LPVOID)&mciSetParms);
  if(dwReturn) {
    CD::stop();
    return;
  }
  // Set the start and stop positions of the CD.
  mciPlayParms.dwFrom=MCI_MAKE_TMSF(track+data_skip, 0, 0, 0);
  mciPlayParms.dwTo=MCI_MAKE_TMSF(track+data_skip+1, 0, 0, 0);
  mciPlayParms.dwCallback=(DWORD_PTR)Comm::hwnd;
  // Send the play command with notification.
  unsigned notify=repeat?MCI_NOTIFY:0;
  if(track==tracks) dwReturn=mciSendCommand(DeviceID,MCI_PLAY,MCI_FROM|notify,(DWORD_PTR)(LPVOID)&mciPlayParms);
  else dwReturn=mciSendCommand(DeviceID,MCI_PLAY,MCI_FROM|MCI_TO|notify,(DWORD_PTR)(LPVOID)&mciPlayParms);
  if(dwReturn) {
    CD::stop();
    return;
  }
  current_track=track;
}

void CD::stop(void)
{
  if(!Sounds::active) return;
  MCI_GENERIC_PARMS mciGenericParms;
  // stop cdaudio device
  mciSendCommand(DeviceID,MCI_STOP,MCI_WAIT,(DWORD_PTR)(LPVOID)&mciGenericParms);
  // close cdaudio device
  mciSendCommand(DeviceID,MCI_CLOSE,MCI_WAIT,(DWORD_PTR)(LPVOID)&mciGenericParms);
  current_track=0;
} 

char* Song::song;
UINT Song::DeviceID;

Song::Song()
{
  song=NULL;
}

Song::~Song()
{
  Song::stop();
}


void Song::play(char* midi_file,int repeat)
{
  if((!Sounds::active)||(!Sounds::app_active)) return;
  if(midi_file==NULL) return;
  Song::stop();
  DWORD dwReturn;
  MCI_OPEN_PARMS mciOpenParms;
  MCI_PLAY_PARMS mciPlayParms;
  // open sequencer device
  mciOpenParms.lpstrDeviceType="sequencer";
  mciOpenParms.lpstrElementName=midi_file;
  dwReturn=mciSendCommand(NULL,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT|MCI_WAIT,(DWORD_PTR)(LPVOID)&mciOpenParms);
  if(dwReturn) {
    char message[256];
    mciGetErrorString(dwReturn,message,256);
    DBG_MESSAGE("Can't play: %s",message);
    return;
  }
  // Get the device ID.
  DeviceID=mciOpenParms.wDeviceID;
  mciPlayParms.dwFrom=0;
  mciPlayParms.dwCallback=(DWORD_PTR)Comm::hwnd;
  // Send the play command with notification.
  unsigned notify=repeat?MCI_NOTIFY:0;
  dwReturn=mciSendCommand(DeviceID,MCI_PLAY,MCI_FROM|repeat,(DWORD_PTR)(LPVOID)&mciPlayParms);
  if(dwReturn) {
    char message[256];
    mciGetErrorString(dwReturn,message,256);
    DBG_MESSAGE("Can't play: %s",message);
    Song::stop();
  }
  song=midi_file;
}

void Song::stop(void)
{
  if(!Sounds::active) return;
  MCI_GENERIC_PARMS mciGenericParms;
  // stop sequencer device
  mciSendCommand(DeviceID,MCI_STOP,MCI_WAIT,(DWORD_PTR)(LPVOID)&mciGenericParms);
  // close sequencer device
  mciSendCommand(DeviceID,MCI_CLOSE,MCI_WAIT,(DWORD_PTR)(LPVOID)&mciGenericParms);
  song=NULL;
}               

