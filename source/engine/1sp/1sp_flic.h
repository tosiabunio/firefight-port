#ifndef __FLIC__
#define __FLIC__

#pragma pack (1)
enum flic_types {FLI, FLC};   

extern struct FLIC_DESC       
{
  int type;                   
  int x_size;                 
  int y_size;                 
  int frames;                 
}
flic_desc;

void open_flic (char *name);

void get_frame (void *frame_buffer, void *palette);

void close_flic (void);

#pragma pack ()
#endif
