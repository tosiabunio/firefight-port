#include "1sp_hdrs.h"

#pragma pack (push,flic_cpp)
#pragma pack (1)
// Flic Header
struct
{
  int                 size;     // Size of flic including this header.
  unsigned short int  type;     // Either FLI_TYPE or FLC_TYPE below.
  unsigned short int  frames;   // Number of frames in flic.
  unsigned short int  width;    // Flic width in pixels.
  unsigned short int  height;   // Flic height in pixels.
  unsigned short int  depth;    // Bits per pixel.  (Always 8 now.)
  unsigned short int  flags;    // FLI_FINISHED | FLI_LOOPED ideally.
  int                 speed;    // Delay between frames.
  short int           reserved1; // Set to zero.
  unsigned            created;  // Date of flic creation. (FLC only.)
  unsigned            creator;  // Serial # of flic creator. (FLC only.)
  unsigned            updated;  // Date of flic update. (FLC only.)
  unsigned            updater;  // Serial # of flic updater. (FLC only.)
  unsigned short int  aspect_dx;  // Width of square rectangle. (FLC only.)
  unsigned short int  aspect_dy;  // Height of square rectangle. (FLC only.)
  char                reserved2 [38]; // Set to zero.
  int                 oframe1;  // Offset to frame 1. (FLC only.)
  int                 oframe2;  // Offset to frame 2. (FLC only.)
  char                reserved3 [40]; // Set to zero.
}
static header;

// Values for FlicHead.type

#define FLI_TYPE 0xAF11  // 320x200 .FLI type ID
#define FLC_TYPE 0xAF12  // Variable rez .FLC type ID

// Optional Prefix Header
struct
{
  int                 size;    // Size of prefix including header.
  unsigned short int  type;    // Always PREFIX_TYPE.
  short int           chunks;  // Number of subchunks in prefix.
  char                reserved [8]; // Always 0.
}
static prefix;

// Value for PrefixHead.type

#define PREFIX_TYPE  0xF100

// Frame Header
struct
{
  int             size;   // Size of frame including header.
  unsigned short  type;   // Always FRAME_TYPE
  short int       chunks; // Number of chunks in frame.
  char            reserved [8]; // Always 0.
}
static frame;

// Value for FrameHead.type

#define FRAME_TYPE 0xF1FA

// Chunk Header
struct
{
  int                 size;  // Size of chunk including header.
  unsigned short int  type;  // Value from ChunkTypes below.
}
static chunk;

enum ChunkTypes
{
  COLOR_256 = 4,  // 256 level color pallette info. (FLC only.)
  DELTA_FLC = 7,  // Word-oriented delta compression. (FLC only.)
  COLOR_64 = 11,  // 64 level color pallette info.
  DELTA_FLI = 12, // Byte-oriented delta compression.
  BLACK = 13,     // whole frame is color 0
  BYTE_RUN = 15,  // Byte run-length compression.
  LITERAL = 16,   // Uncompressed pixels.
  PSTAMP = 18     // "Postage stamp" chunk. (FLC only.)
};

typedef unsigned char byte;
typedef unsigned short int word;
#pragma pack ()

#pragma pack (1)
static File ff;
struct FLIC_DESC flic_desc;

static void decode_palette_64 (byte *buf, void *palette)
{
  word cnt, num, i;
  byte *pal;

  pal=(byte*)palette;
  cnt=*(word*)buf;
  buf+=2;
  while (cnt>0)
  {
    pal+=*buf++*3;
    if ((num=*buf++)==0) num = 256;
    for  (i=0; i<num*3; i++)
      *pal++=*buf++;
    cnt--;
  }
}

static void decode_palette_256 (byte *buf, void *palette)
{
  word cnt, num, i;
  byte *pal;

  pal=(byte*)palette;
  cnt=*(word*)buf;
  buf+=2;
  while (cnt>0)
  {
    pal+=*buf++*3;
    if ((num=*buf++)==0) num = 256;
    for  (i=0; i<num*3; i++)
      *pal++=(byte)((*buf++)>>2);
    cnt--;
  }
}

static void decode_brun (byte *buf, void *screen, unsigned row_cnt)
{
  word row;
  byte *scr, len, op_cnt, color, i;

  for (row=0; row<row_cnt; row++)
  {
    scr=(byte*)screen;
    scr+=flic_desc.x_size*row;
    op_cnt=*buf++;
    {
      int sss=flic_desc.x_size;
      do
      {
        if ((len=*buf++)&0x80)
        {
          for (i=0; i<256-len; i++)
          {
            *scr++=*buf++;
            sss--;
          }
        }
        else
        {
          color=*buf++;
          for (i=0; i<len; i++)
          {
            *scr++=color;
            sss--;
          }
        }
      }
      while(sss);
    }
  }
}

static void decode_delta_fli (byte *buf, void *screen)
{
  word  row, row_cnt, tmp;
  byte *scr, cnt, len, op_cnt, color, i, *scr_org;

  tmp=*(word*)buf;
  buf+=2;
  scr_org=(byte*)screen;
  scr_org+=(tmp*flic_desc.x_size);
  row_cnt=*(word*)buf;
  buf+=2;

  for (row=0; row<row_cnt; row++)
  {
    scr=scr_org;
    scr+=flic_desc.x_size*row;
    op_cnt=*buf++;
    for (cnt=0; cnt<op_cnt; cnt++)
    {
      scr+=*buf++;        /* pixels to skip */
      if ((len=*buf++)&0x80)
      {
        color=*buf++;
        for (i=0; i<256-len; i++)
          *scr++=color;
      }
      else
      {
        for (i=0; i<len; i++)
          *scr++=*buf++;
      }
    }
  }
}

typedef struct
{
  unsigned char pixels [2];
}
Pixels2;

static void decode_delta_flc (byte *data, void *screen)
{
  int xorg=0;
  int yorg=0;
  int width=flic_desc.x_size;
  int x,y;
  short int lp_count;
  short int opcount;
  short int psize;
  union {unsigned short int *w; byte *ub; char *b; Pixels2 *p2;} wpt;
  int lastx;

  byte *scr;

  scr=(byte *)screen;
  lastx=xorg+width-1;
  wpt.ub=data;
  lp_count=*wpt.w++;
  y=yorg;

  goto LPACK;

SKIPLINES:      /* Advance over some lines. */
  y-=opcount;

LPACK:          // do next line
  if ((opcount=*wpt.w++)>=0) goto DO_SS2OPS;

  if (((unsigned short int)opcount)&0x4000) goto SKIPLINES; // skip lines

  *(scr+y*width+lastx)=(byte)opcount;

  if ((opcount=*wpt.w++)==0)
  {
    ++y;
    if (--lp_count>0) goto LPACK;
    goto LEAVEPROC;
  }

DO_SS2OPS:
  x=xorg;

PPACK:                          /* do next packet */
  x+=*wpt.ub++;

  signed char sc;
  sc=*wpt.b++;
  psize=(short int)sc;

  if ((psize=(short)(psize+psize))>=0)
  {
    byte *p, *s;
    p=wpt.ub;
    s=scr+y*width+x;
    for (int cntr=psize; cntr>0; cntr--)
      *s++=*p++;

    x+=psize;
    wpt.ub+=psize;

    if (--opcount!=0) goto PPACK;
    ++y;
    if (--lp_count>0) goto LPACK;
  }
  else
  {
    psize=(short)-psize;

    word *p, *s;
    p=(word*)wpt.p2;
    s=(word *)(scr+y*width+x);
    for (int cntr=psize>>1; cntr>0; cntr--)
      *s++=*p;

    wpt.p2++;
    x+=psize;

    if (--opcount!=0) goto PPACK;
    ++y;
    if (--lp_count>0) goto LPACK;
  }

LEAVEPROC:
  return;
}


void open_flic (char *name)
{
  try
  {
    ff.open(name);
    ff.read((char*)&header, sizeof (header));
    if (!((header.type==FLI_TYPE)||(header.type==FLC_TYPE)))
      FAILURE ("bad FLC (FLI) header [open_flic]");
    flic_desc.type=header.type;
    flic_desc.x_size=header.width;
    flic_desc.y_size=header.height;
    flic_desc.frames=header.frames;
    if (header.type==FLC_TYPE)
    {
      ff.close();
      ff.open(name);
      ff.ignore(header.oframe1);
    }
  }
  catch (Failure)
  {
    FAILURE ("error opening FLC (FLI) file [open_flic]");
  }
}

void get_frame (void *frame_buffer, void *palette)
{
  byte *buf;

  try
  {
    ff.read((char*)&frame, sizeof (frame));
    if (frame.type!=FRAME_TYPE)
      FAILURE ("bad frame header [get_frame]");
    for (int cnt=1; cnt<=frame.chunks; cnt++)
    {
      ff.read((char*)&chunk, sizeof (chunk));
      buf=(byte*)Heap::alloc (chunk.size-sizeof (chunk),"[spr] tmp flic buf");
      if((frame.chunks==1)&&(chunk.type==LITERAL))
        ff.read((char*)buf,chunk.size-4);
      else
        ff.read((char*)buf,chunk.size-sizeof(chunk));
      switch (chunk.type)
      {
        case DELTA_FLC:
          decode_delta_flc (buf, frame_buffer);
          break;
        case PSTAMP:
          break;
        case COLOR_256:
          decode_palette_256 (buf, palette);
          break;
        case COLOR_64:
          decode_palette_64 (buf, palette);
          break;
        case DELTA_FLI:
          decode_delta_fli (buf, frame_buffer);
          break;
        case BLACK:
          memset(frame_buffer, 0, flic_desc.x_size*flic_desc.y_size);
          break;
        case BYTE_RUN:
          decode_brun (buf, frame_buffer, flic_desc.y_size);
          break;
        case LITERAL:
          memcpy(frame_buffer, buf, flic_desc.x_size*flic_desc.y_size);
          break;
      }
      Heap::free (buf,FILE_LINE);
    }
  }
  catch (Failure)
  {
    FAILURE ("error processing frame [get_frame]");
  }
}

void close_flic (void)
{
  ff.close();
}

#pragma pack (pop,flic_cpp)