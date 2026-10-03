#include "1ss_hdrs.h"

static MIXERLINE sound_line;
static MIXERLINE music_line;
static MIXERLINECONTROLS controls;
static MIXERCONTROL sound_volume_control;
static MIXERCONTROL music_volume_control;
static MIXERCONTROLDETAILS sound_volume_control_details;
static MIXERCONTROLDETAILS music_volume_control_details;
static MIXERCONTROLDETAILS_UNSIGNED saved_sound_volume[2];
static MIXERCONTROLDETAILS_UNSIGNED saved_music_volume[2];
static MIXERCONTROLDETAILS_UNSIGNED new_sound_volume[2];
static MIXERCONTROLDETAILS_UNSIGNED new_music_volume[2];
static HMIXER mixer_handle;
static HWAVEOUT waveout_handle;
static int sound_volume=0;
static int music_volume=0;
static int mixer_active=0;
static unsigned mixer_parts=0;
int Mixer::internal_change=0;

#define MUSIC_VOLUME_ACTIVE 0x00000001
#define SOUND_VOLUME_ACTIVE 0x00000002

void Mixer::save_volumes(void)
{
  if(mixer_parts&SOUND_VOLUME_ACTIVE) {
    // save sound volume values
    sound_volume_control_details.cbStruct=sizeof(sound_volume_control_details);
    sound_volume_control_details.dwControlID=sound_volume_control.dwControlID;
    sound_volume_control_details.cChannels=sound_line.cChannels;
    sound_volume_control_details.cMultipleItems=0;
    sound_volume_control_details.cbDetails=sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    sound_volume_control_details.paDetails=&saved_sound_volume;
    MMRESULT result=mixerGetControlDetails((HMIXEROBJ)mixer_handle,&sound_volume_control_details,MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
    if(result!=MMSYSERR_NOERROR) {
      mixer_parts&=~SOUND_VOLUME_ACTIVE;
      sound_volume=0;
      return;
    }
    if(sound_line.cChannels==1) saved_sound_volume[1]=saved_sound_volume[0];
    sound_volume=saved_sound_volume[0].dwValue>=saved_sound_volume[1].dwValue?saved_sound_volume[0].dwValue:saved_sound_volume[1].dwValue;
  }
  if(mixer_parts&MUSIC_VOLUME_ACTIVE) {
    // save music volume values
    music_volume_control_details.cbStruct=sizeof(music_volume_control_details);
    music_volume_control_details.dwControlID=music_volume_control.dwControlID;
    music_volume_control_details.cChannels=music_line.cChannels;
    music_volume_control_details.cMultipleItems=0;
    music_volume_control_details.cbDetails=sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    music_volume_control_details.paDetails=&saved_music_volume;
    MMRESULT result=mixerGetControlDetails((HMIXEROBJ)mixer_handle,&music_volume_control_details,MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
    if(result!=MMSYSERR_NOERROR) {
      mixer_parts&=~MUSIC_VOLUME_ACTIVE;
      music_volume=0;
      return;
    }
    if(music_line.cChannels==1) saved_music_volume[1]=saved_music_volume[0];
    music_volume=saved_music_volume[0].dwValue>=saved_music_volume[1].dwValue?saved_music_volume[0].dwValue:saved_music_volume[1].dwValue;
  }
}

void Mixer::restore_volumes(void)
{
  if(mixer_parts&SOUND_VOLUME_ACTIVE) {
    // restore sound volume values
    sound_volume_control_details.cbStruct=sizeof(sound_volume_control_details);
    sound_volume_control_details.dwControlID=sound_volume_control.dwControlID;
    sound_volume_control_details.cChannels=sound_line.cChannels;
    sound_volume_control_details.cMultipleItems=0;
    sound_volume_control_details.cbDetails=sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    sound_volume_control_details.paDetails=&saved_sound_volume;
    mixerSetControlDetails((HMIXEROBJ)mixer_handle,&sound_volume_control_details,MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
  }
  if(mixer_parts&MUSIC_VOLUME_ACTIVE) {
    // restore music volume values
    music_volume_control_details.cbStruct=sizeof(music_volume_control_details);
    music_volume_control_details.dwControlID=music_volume_control.dwControlID;
    music_volume_control_details.cChannels=music_line.cChannels;
    music_volume_control_details.cMultipleItems=0;
    music_volume_control_details.cbDetails=sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    music_volume_control_details.paDetails=&saved_music_volume;
    mixerSetControlDetails((HMIXEROBJ)mixer_handle,&music_volume_control_details,MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
  }
}

void Mixer::init(DWORD line)
{
  MMRESULT result;
  // get number of mixer devices available
  mixer_active=mixerGetNumDevs();
  // initialize mixer
  if(mixer_active) {
    mixer_parts=SOUND_VOLUME_ACTIVE|MUSIC_VOLUME_ACTIVE;
    // at least one is available, we will always use first one
    result=mixerOpen(&mixer_handle,0,(DWORD_PTR)Comm::hwnd,0,CALLBACK_WINDOW|MIXER_OBJECTF_MIXER);
    if(result!=MMSYSERR_NOERROR) mixer_parts=0;
    // retrieve information about music audio line
    music_line.cbStruct=sizeof(music_line);
    music_line.dwComponentType=line;
    result=mixerGetLineInfo((HMIXEROBJ)mixer_handle,&music_line,MIXER_OBJECTF_HMIXER|MIXER_GETLINEINFOF_COMPONENTTYPE);
    if(result!=MMSYSERR_NOERROR) mixer_parts&=~MUSIC_VOLUME_ACTIVE;
    // retrieve information about sound audio line
    sound_line.cbStruct=sizeof(sound_line);
    sound_line.dwComponentType=MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
    result=mixerGetLineInfo((HMIXEROBJ)mixer_handle,&sound_line,MIXER_OBJECTF_HMIXER|MIXER_GETLINEINFOF_COMPONENTTYPE);
    if(result!=MMSYSERR_NOERROR) mixer_parts&=~SOUND_VOLUME_ACTIVE;
    // retrieve music volume control
    memset(&controls,0,sizeof(controls));
    controls.cbStruct=sizeof(controls);
    controls.dwLineID=music_line.dwLineID;
    controls.dwControlType=MIXERCONTROL_CONTROLTYPE_VOLUME;
    controls.cControls=1;
    controls.cbmxctrl=sizeof(music_volume_control);
    controls.pamxctrl=&music_volume_control;
    result=mixerGetLineControls((HMIXEROBJ)mixer_handle,&controls,MIXER_OBJECTF_HMIXER|MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if(result!=MMSYSERR_NOERROR) mixer_parts&=~MUSIC_VOLUME_ACTIVE;
    // retrieve sound volume control
    memset(&controls,0,sizeof(controls));
    controls.cbStruct=sizeof(controls);
    controls.dwLineID=sound_line.dwLineID;
    controls.dwControlType=MIXERCONTROL_CONTROLTYPE_VOLUME;
    controls.cControls=1;
    controls.cbmxctrl=sizeof(sound_volume_control);
    controls.pamxctrl=&sound_volume_control;
    result=mixerGetLineControls((HMIXEROBJ)mixer_handle,&controls,MIXER_OBJECTF_HMIXER|MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if(result!=MMSYSERR_NOERROR) mixer_parts&=~SOUND_VOLUME_ACTIVE;
  }
}

void Mixer::quit(void) 
{
  if(mixer_active) {
//    Mixer::restore_volumes();
    mixerClose(mixer_handle);
  }
  mixer_active=0;
}


int Mixer::get_sound_volume(void)
{
  if(mixer_parts&SOUND_VOLUME_ACTIVE) return sound_volume;
  else return 0;
}

int Mixer::get_music_volume(void)
{
  if(mixer_parts&MUSIC_VOLUME_ACTIVE) return music_volume;
  else return 0;
}


void Mixer::set_sound_volume(int volume)
{
  if(!(mixer_parts&SOUND_VOLUME_ACTIVE)) return;
  if(volume>VOLUME_MAX) volume=VOLUME_MAX;
  // calculate new sound volume values
  sound_volume=volume;
  if(saved_sound_volume[0].dwValue==saved_sound_volume[1].dwValue) {
    new_sound_volume[0].dwValue=volume;
    new_sound_volume[1].dwValue=volume;
  } else {
    int louder=saved_sound_volume[0].dwValue>=saved_sound_volume[1].dwValue?0:1;
    new_sound_volume[louder].dwValue=volume;
    new_sound_volume[1-louder].dwValue=(volume*saved_sound_volume[1-louder].dwValue)/VOLUME_MAX;
  }
  // set sound volume
  sound_volume_control_details.cbStruct=sizeof(sound_volume_control_details);
  sound_volume_control_details.dwControlID=sound_volume_control.dwControlID;
  sound_volume_control_details.cChannels=sound_line.cChannels;
  sound_volume_control_details.cMultipleItems=0;
  sound_volume_control_details.cbDetails=sizeof(MIXERCONTROLDETAILS_UNSIGNED);
  sound_volume_control_details.paDetails=&new_sound_volume;
  internal_change=1;
  MMRESULT result=mixerSetControlDetails((HMIXEROBJ)mixer_handle,&sound_volume_control_details,MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
  if(result!=MMSYSERR_NOERROR) mixer_parts&=~SOUND_VOLUME_ACTIVE;
}

void Mixer::set_music_volume(int volume)
{
  if(!(mixer_parts&MUSIC_VOLUME_ACTIVE)) return;
  // calculate new music volume values
  music_volume=volume;
  if(saved_music_volume[0].dwValue==saved_music_volume[1].dwValue) {
    new_music_volume[0].dwValue=volume;
    new_music_volume[1].dwValue=volume;
  } else {
    int louder=saved_music_volume[0].dwValue>=saved_music_volume[1].dwValue?0:1;
    new_music_volume[louder].dwValue=volume;
    new_music_volume[1-louder].dwValue=(volume*saved_music_volume[1-louder].dwValue)/VOLUME_MAX;
  }
  // set music volume
  music_volume_control_details.cbStruct=sizeof(music_volume_control_details);
  music_volume_control_details.dwControlID=music_volume_control.dwControlID;
  music_volume_control_details.cChannels=music_line.cChannels;
  music_volume_control_details.cMultipleItems=0;
  music_volume_control_details.cbDetails=sizeof(MIXERCONTROLDETAILS_UNSIGNED);
  music_volume_control_details.paDetails=&new_music_volume;
  internal_change=1;
  MMRESULT result=mixerSetControlDetails((HMIXEROBJ)mixer_handle,&music_volume_control_details,MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
  if(result!=MMSYSERR_NOERROR) mixer_parts&=~MUSIC_VOLUME_ACTIVE;
}

