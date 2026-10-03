#ifndef _1SS_H_INCLUDED
#define _1SS_H_INCLUDED

#include <compat/win32.h>

#ifndef EXCLUDE_LIBS
#ifdef _DEBUG
#pragma comment(lib,"1ssd.lib")
#else
#pragma comment(lib,"1ss.lib")
#endif
#endif
#pragma comment(lib,"winmm.lib")

//error codes
#define SOS_ERROR 200
#define SOS_DS_NOT_INSTALLED 201
#define SOS_CANNOT_INITIALIZE 202
#define SOS_OUT_OF_MEMORY 203

//initialization modes
#define SOS_NONE    0
#define SOS_WAVE    1
#define SOS_16_S_22 2
#define SOS_8_S_22  3
#define SOS_16_S_11 4
#define SOS_8_S_11  5

//pan values
#define PAN_LEFT -3000
#define PAN_CENTER 0
#define PAN_RIGHT 3000

//volume values
#define VOLUME_MIN 0
#define VOLUME_MAX 0xFFFF

//max number of channels
#define SOS_MAX_CHANNELS 16

class Sample {
public:
  Sample();
  ~Sample();
  void load(char const* long_name);
  void play(int panning,int volume);
  void stop(void);
  void free(void);
  int playing(void);
  int panning;
  int volume;
private:
  char const* name;
  void* samples;
  IDirectSoundBuffer* buffer;
  WAVEFORMATEX format;
  unsigned size;
  unsigned time;
  unsigned start;
  unsigned handle;
  int priority;
  friend class Sounds;
};

class Mixer {
private:
  static void init(DWORD line);
  static void quit(void);
  static void save_volumes(void);
  static void restore_volumes(void);
  static int get_sound_volume(void);
  static int get_music_volume(void);
  static void set_sound_volume(int volume);
  static void set_music_volume(int volume);
  static int internal_change;
  friend class Sounds;
};

class Sounds {
public:
  static void init(int mode,int channels=8,DWORD mixer_music_line=MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC);
  static void stop_all(void);
  static inline int get_sound_volume(void) {return Mixer::get_sound_volume();}
  static inline int get_music_volume(void) {return Mixer::get_music_volume();}
  static inline void set_sound_volume(int volume) {Mixer::set_sound_volume(volume);}
  static inline void set_music_volume(int volume) {Mixer::set_music_volume(volume);}
  static int window_proc (int *result, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
  static int extra_debug;
private:
  static void play(Sample* sample,int panning,int volume=VOLUME_MAX);
  static void stop(Sample* sample);
  static int playing(Sample* sample);
  static void quit(void);
  static int active; 
  static int active_mode;
  static int app_active;
  static IDirectSound* sound_driver;
  friend class Sample;
  friend class CD;
  friend class Song;
  friend class Mixer;
};

class CD {
public:
  CD();
  ~CD();
  static int tracks;
  static void init(void);
  static void play(int track,int repeat=1);
  static void stop(void);
private:
  static int current_track;
  static UINT DeviceID;
  friend class Sounds;
};

class Song {
public:
  Song();
  ~Song();
  static void play(char* midi_file,int repeat=1);
  static void stop(void);
private:
  static char* song;
  static UINT DeviceID;
  friend class Sounds;
};

#endif
