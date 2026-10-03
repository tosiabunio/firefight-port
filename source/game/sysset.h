#ifndef __SYSSET__
#define __SYSSET__

//KLASA SYSSET
class SysSet
{
  private:
   static int initialized;
   static int version;
  public:
   enum SetType
   {
    _VERSION,
    _FIRST,
     INPUT_DEVICE,
     CHEATING,
     AUTORUN,
     RESOLUTION,
     DETAIL_LEVEL,
     GAMMA,
     FX_VOL,
     MUSIC_VOL,
     REVERSE_STEREO,
     MUTE,
    _LAST
   };
   struct SetRange
   {
     int lorange;
     int uprange;
   };
  private:
   // ranges and defaults have one extra slot because init() writes element _LAST. On MSVC
   // that only zeroed the next array's first entries, which were zero anyway; elsewhere the
   // statics may be laid out differently. data keeps its size: it is saved with the pilot.
   static SetRange ranges[SysSet::_LAST+1];
   static int defaults[SysSet::_LAST+1];
   static int data[SysSet::_LAST];
   static int check_range(SysSet::SetType type,int& value);
   static void action(SysSet::SetType type);
   static void update(SysSet::SetType type);
  public:
   static void init(char *_defaults);
   static void quit(void);
   static void set(void);
   static int  set(void *ptr,int size);
   static void set(SysSet::SetType type,int value);
   static int  get(SysSet::SetType type);
   static void *dump_buffer(void);
   static int  dump_size(void);
};

#endif
