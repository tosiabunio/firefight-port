#include "headers.h"

int SysSet::initialized=0;
int SysSet::version=0x0006;
SysSet::SetRange SysSet::ranges[SysSet::_LAST+1];
int SysSet::defaults[SysSet::_LAST+1];
int SysSet::data[SysSet::_LAST];

void SysSet::init(char *_defaults_file)
{
   DBG_CHECK(!initialized);
   initialized=1;

   defaults[_VERSION]=0;
   ranges[_VERSION].lorange=0;
   ranges[_VERSION].uprange=0;

   defaults[_FIRST]=0;
   ranges[_FIRST].lorange=0;
   ranges[_FIRST].uprange=0;

   defaults[_LAST]=0;
   ranges[_LAST].lorange=0;
   ranges[_LAST].uprange=0;

   int controls=0;
   switch (RegData::GetCurrentControlSet())
   {
       case RegData::ctlset_Kbd: controls=0;
                                 break;
     case RegData::ctlset_Mouse: controls=1;
                                 break;
       case RegData::ctlset_Joy: controls=2;
                                 break;
                        default: DBG_CHECK(0);
                                 break;
   }
   defaults[INPUT_DEVICE]=controls;
   ranges[INPUT_DEVICE].lorange=0;
   ranges[INPUT_DEVICE].uprange=2;

   defaults[AUTORUN]=0;
   ranges[AUTORUN].lorange=0;
   ranges[AUTORUN].uprange=1;

   defaults[CHEATING]=0;
   ranges[CHEATING].lorange=0;
   ranges[CHEATING].uprange=1;

   defaults[RESOLUTION]=1;
   ranges[RESOLUTION].lorange=0;
   ranges[RESOLUTION].uprange=1;

   defaults[DETAIL_LEVEL]=1;
   ranges[DETAIL_LEVEL].lorange=0;
   ranges[DETAIL_LEVEL].uprange=1;

   defaults[GAMMA]=0;
   ranges[GAMMA].lorange=Video::min_gamma;
   ranges[GAMMA].uprange=Video::max_gamma;

   defaults[FX_VOL]=Sounds::get_sound_volume();
   ranges[FX_VOL].lorange=VOLUME_MIN;
   ranges[FX_VOL].uprange=VOLUME_MAX;

   defaults[MUSIC_VOL]=Sounds::get_music_volume();
   ranges[MUSIC_VOL].lorange=VOLUME_MIN;
   ranges[MUSIC_VOL].uprange=VOLUME_MAX;

   defaults[REVERSE_STEREO]=0;
   ranges[REVERSE_STEREO].lorange=0;
   ranges[REVERSE_STEREO].uprange=1;

   defaults[MUTE]=0;
   ranges[MUTE].lorange=0;
   ranges[MUTE].uprange=1;
}

void SysSet::quit(void)
{
  initialized=0;
}

void SysSet::set(void)
{
  DBG_CHECK(initialized);
  for (int i=(int)_FIRST+1;i<_LAST;i++)
  {
    set((SetType)i,defaults[i]);
  }
}

int SysSet::set(void *ptr,int size)
{
  DBG_CHECK(initialized);
  if (size!=sizeof(data))
  {
    return 0;
  }
  memcpy((void*)data,ptr,size);
  if (data[_VERSION]!=version)
  {
    return 0;
  }

  int controls=0;
  switch (RegData::GetCurrentControlSet())
  {
      case RegData::ctlset_Kbd: controls=0;
                                break;
    case RegData::ctlset_Mouse: controls=1;
                                break;
      case RegData::ctlset_Joy: controls=2;
                                break;
                       default: DBG_CHECK(0);
                                break;
  }
  data[INPUT_DEVICE]=controls;
  data[FX_VOL]=Sounds::get_sound_volume();
  data[MUSIC_VOL]=Sounds::get_music_volume();
  if (RegData::DataToLoad==RegData::load_Lores&&!Mp::BUILDSPRITESMODE)
  {
    data[RESOLUTION]=0;
  }

  for (int i=(int)_FIRST+1;i<_LAST;i++)
  {
    if (check_range((SetType)i,data[i]))
    {
      set((SetType)i,data[i]);
    }
    else
    {
      return 0;
    }
  }
  return 1;
}

void *SysSet::dump_buffer(void)
{
  DBG_CHECK(initialized);
  data[_VERSION]=version;
  return (void*)data;
}

int SysSet::dump_size(void)
{
  return sizeof(data);
}

int SysSet::check_range(SysSet::SetType type,int& value)
{
  DBG_CHECK(type>_FIRST&&type<_LAST);
  return (value>=ranges[type].lorange&&value<=ranges[type].uprange);
}

void SysSet::set(SysSet::SetType type,int value)
{
  DBG_CHECK(type>_FIRST&&type<_LAST);
  if (check_range(type,value))
  {
    data[type]=value;
    action(type);
  }
}

int SysSet::get(SysSet::SetType type)
{
  DBG_CHECK(type>_FIRST&&type<_LAST);
  if (check_range(type,data[type]))
  {
    update(type);
    return data[type];
  }
  else
  {
    return 0;
  }
}

void SysSet::action(SysSet::SetType type)
{
  DBG_CHECK(type>_FIRST&&type<_LAST);
  switch (type)
  {
      case INPUT_DEVICE: switch (data[type])
                         {
                           case 0: RegData::SetCurrentControlSet(RegData::ctlset_Kbd);
                                   break;
                           case 1: RegData::SetCurrentControlSet(RegData::ctlset_Mouse);
                                   break;
                           case 2: RegData::SetCurrentControlSet(RegData::ctlset_Joy);
                                   break;
                          default: DBG_CHECK(0);
                                   break;
                         }
                         break;
        case RESOLUTION: Video::set_mode((data[type]&&Mp::HIRESAVAIL)?Spr::hires:Spr::lores);
                         Statistics::reinit();
                         break;
             case GAMMA: Video::update_palette(data[type]);
                         break;
            case FX_VOL: Sounds::set_sound_volume(data[type]);
                         break;
         case MUSIC_VOL: Sounds::set_music_volume(data[type]);
                         break;
    case REVERSE_STEREO: break;
  }
}

void SysSet::update(SysSet::SetType type)
{
  DBG_CHECK(type>_FIRST&&type<_LAST);
  int value=0;

  switch (type)
  {
            case FX_VOL: value=Sounds::get_sound_volume();
                         if (check_range(type,value))
                         {
                           data[type]=value;
                         }
                         break;
         case MUSIC_VOL: value=Sounds::get_music_volume();
                         if (check_range(type,value))
                         {
                           data[type]=value;
                         }
                         break;
    case REVERSE_STEREO: break;
  }
}
