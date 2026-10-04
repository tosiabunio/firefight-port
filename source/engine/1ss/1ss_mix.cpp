#include "1ss_hdrs.h"
#include <1ba.h>
#include <1rg.h>
#include <1cw_strg.h>
#include <SDL_mixer.h>

// Port: the original set the system-wide wave and CD volumes through the Windows mixer, and
// never restored them. The port keeps both volumes in the game: the sound volume scales every
// sample (Mix_MasterVolume), the music volume the soundtrack (Mix_VolumeMusic). Both run from
// VOLUME_MIN to VOLUME_MAX, as the mixer controls did. The game never stored them, because it
// read them back from the system mixer, so the port keeps them in the settings.
//
// Both default to half (-6 dB). The original's sound card mixed the CD audio with the samples in
// analogue; here they share one digital mix, and at full volume a busy scene clips.

static char key_sound_volume[] = "sound volume";
static char key_music_volume[] = "music volume";
static const int default_volume=VOLUME_MAX/2;
static int sound_volume=default_volume;
static int music_volume=default_volume;
int Mixer::internal_change=0;

static int clamp_volume(int volume)
{
  if(volume<VOLUME_MIN) return VOLUME_MIN;
  if(volume>VOLUME_MAX) return VOLUME_MAX;
  return volume;
}

static int mix_level(int volume)
{
  return (volume*MIX_MAX_VOLUME+VOLUME_MAX/2)/VOLUME_MAX;
}

void Mixer::init(DWORD line)
{
  (void)line;  // the mixer line that carried the music (CD, line in or MIDI)
  sound_volume=clamp_volume((int)Registry::get_int(sec_sos,key_sound_volume,default_volume));
  music_volume=clamp_volume((int)Registry::get_int(sec_sos,key_music_volume,default_volume));
  if(Sounds::active&&(Sounds::active_mode!=SOS_NONE)) {
    Mix_MasterVolume(mix_level(sound_volume));
    Mix_VolumeMusic(mix_level(music_volume));
  }
}

void Mixer::quit(void)
{
}

int Mixer::get_sound_volume(void)
{
  return sound_volume;
}

int Mixer::get_music_volume(void)
{
  return music_volume;
}

void Mixer::set_sound_volume(int volume)
{
  sound_volume=clamp_volume(volume);
  Registry::set_int(sec_sos,key_sound_volume,sound_volume);
  if(Sounds::active&&(Sounds::active_mode!=SOS_NONE)) Mix_MasterVolume(mix_level(sound_volume));
}

void Mixer::set_music_volume(int volume)
{
  music_volume=clamp_volume(volume);
  Registry::set_int(sec_sos,key_music_volume,music_volume);
  if(Sounds::active&&(Sounds::active_mode!=SOS_NONE)) Mix_VolumeMusic(mix_level(music_volume));
}
