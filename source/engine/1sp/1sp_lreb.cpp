#include "1sp_hdrs.h"
#include "1sp_ldsa.h"

//----- komunikaty -----------------------------------------------------------

static char F_flcerr[] =   "error in flic frame number %d";
static char F_cantrebu[] = "rebuild process failed";
static char F_nomem[] =    "not enough memory for RLE buffer";

static char M_flcconv[] = "building %s %s";
static char *M_withm[] = {"no","yes"};
static char *M_onec[] = {"no","yes"};
static char *M_scale[] = {"","1x1","2x1","","4x1","","","","8x1"};

//----- stale i zmienne ------------------------------------------------------

static int progress_max, progress_current;

struct Src
{
  int sx,sy;
  unsigned char *adr;
  void init (unsigned char *_adr, int _sx, int _sy) { adr=_adr; sx=_sx; sy=_sy;}
  unsigned char *operator[] (int y) {return(&adr[y*sx]);}
} static src;

static const unsigned key_color=255;

struct Buffer
{
  unsigned char *mem;
  int size,max_size;

  void init (void)
  {
    size=0;
    mem=NULL;
    max_size=2*1024*1024;
    CHECK((mem=(unsigned char*)VirtualAlloc(NULL,max_size,MEM_COMMIT,PAGE_READWRITE)));
  }

  void quit (void)
  {
    CHECK(VirtualFree(mem,0,MEM_RELEASE));
  }

  void *push (int val, int bnum)
  {
    if (size+bnum>=max_size)
      FAILURE (F_nomem);
    int s=size;
    unsigned char *c=(unsigned char*)&val;
    for (int i=0; i<bnum; i++)
      mem[size++]=c[i];
    return(&mem[s]);
  }
} static buffer;

//----- kompresja RLE z wykorzystaniem stosu ---------------------------------

class RLE
{
  int sx,sy,scale,sens;
  int not_empty (int x, int y);
  unsigned value (int x, int y);
  void store_block (int x, int y, int len);

  int find_packet (int x, int y, short &plen, short &pnum);
  void compress_packet (int x, int y, int len, short &pnum);
  void compress_line (int y);
public:
  unsigned char *compress (int _sx, int _sy, int _scale, int _sens);
} static rle;

int RLE::not_empty (int x, int y)
{
  int num=0;
  for (int y1=0; y1<scale; y1++)
    for (int x1=0; x1<scale; x1++)
      if (src[y*scale+y1][x*scale+x1]!=key_color)
        num++;
  return(num>sens);
}

unsigned RLE::value (int x, int y)
{
  unsigned char c[8*8];
  int num=0;
  for (int y1=0; y1<scale; y1++)
    for (int x1=0; x1<scale; x1++)
      if (src[y*scale+y1][x*scale+x1]!=key_color)
        c[(num++)]=src[y*scale+y1][x*scale+x1];
  unsigned result=c[0];
  if (num>1)
  {
    int r,g,b;
    r=g=b=0;
    for (int i=0; i<num; i++)
    {
      Color &col=Video::palette[c[i]];
      r+=col.rval();
      g+=col.gval();
      b+=col.bval();
    }
    r/=num;
    g/=num;
    b/=num;
    result=Video::closest(r,g,b);
  }
  return(result);
}

void RLE::compress_packet (int x, int y, int len, short &pnum)
{
  int l=0,val=value(x,y),c;
  do
  {
    c=value(x+l,y);
    l++;
  } while ((l<len)&&(val==c));

  if (l<len)
  {
    buffer.push(len,sizeof(short));
    l=0;
    do
    {
      buffer.push(value(x+l,y),sizeof(unsigned char));
      l++;
    } while (l<len);
  }
  else
  {
    buffer.push(-len,sizeof(short));
    buffer.push(value(x,y),sizeof(unsigned char));
  }

  pnum++;
}

int RLE::find_packet (int x, int y, short &plen, short &pnum)
{
  plen=pnum=0;
  int epos=x;
  while ((x<sx)&&(!not_empty(x,y)))
    x++;
  int fpos=x;
  while ((x<sx)&&(not_empty(x,y)))
    x++;
  if (fpos==x)
    return (0);
  buffer.push(fpos-epos,sizeof(short));
  compress_packet(fpos,y,x-fpos,pnum);
  plen=(short)(x-epos);
  return(1);
}

void RLE::compress_line (int y)
{
  short *sizep=(short*)buffer.push(0,sizeof(short));
  short bssav=(short)buffer.size;

  short *packets_num=(short*)buffer.push(0,sizeof(short));;
  short plen,pnum;
  int x=0;

  while (find_packet(x,y,plen,pnum))
  {
    (*packets_num)=(short)((*packets_num)+pnum);
    x+=plen;
  }

  if (*packets_num)
    *sizep=(short)(buffer.size-bssav);
  else
    buffer.size-=sizeof(*packets_num);
}

unsigned char *RLE::compress (int _sx, int _sy, int _scale, int _sens)
{
  sx=_sx;
  sy=_sy;
  scale=_scale;
  sens=_sens;

  int *sizep=(int*)buffer.push(0,sizeof(int));
  int bssav=buffer.size;

  for (int y=0; y<sy; y++)
  {
    Video::progress_bar(progress_current+1,progress_max);
    progress_current++;
    compress_line(y);
  }

  buffer.push(0xffffffff,sizeof(short));

  *sizep=buffer.size-bssav;
  return((unsigned char*)sizep);
}

//----- preparowanie kolejnych faz -------------------------------------------

void Lsprite::Phase::rebuild(int _ox, int _oy, int scale, int sens, int mirrors)
{
  def[0]=def[1]=NULL;

  sx=(short)(src.sx/scale);
  sy=(short)(src.sy/scale);
  ox=(short)(_ox/scale), oy=(short)(_oy/scale);

  def[0]=rle.compress(sx,sy,scale,sens);

  if (mirrors)
  {
    for (int y=0; y<src.sy; y++)
    {
      Video::progress_bar(progress_current+1,progress_max);
      progress_current++;
      for (int x=0; x<src.sx/2; x++)
      {
        unsigned char c=src[y][x];
        src[y][x]=src[y][src.sx-1-x];
        src[y][src.sx-1-x]=c;
      }
    }

    def[1]=rle.compress(sx,sy,scale,sens);
  }
}

void Lsprite::rebuild(void)
{
  frame.buf=NULL;
  src.adr=NULL;
  try
  {
    open_flic (source_name);
    sx=flic_desc.x_size-2;
    sy=flic_desc.y_size-2;
    frame.buf=(unsigned char*)Heap::alloc((sx+2)*(sy+2),"[spr] tmp frame buffer");
    frame.llen=sx+2;
    phases=flic_desc.frames;

    progress_max=phases*(sy+sy/scale);
    if (mirrors)
      progress_max+=phases*(sy+sy/scale);
    progress_current=0;
    Video::display_message(M_flcconv,master_name,name);

    src.init((unsigned char*)Heap::alloc(sx*sy,"[spr] tmp src buffer"),sx,sy);

    phase=(Phase*)Heap::alloc(4096*sizeof(Phase),"[spr] tmp phases");
    memset(phase,0,4096*sizeof(Phase));
    buffer.init();

    for (frame_num=1; frame_num<phases+1; frame_num++)
    {
      try
      {
        unsigned char pal[256][3];
        get_frame(frame.buf,pal);

        for (int y=0; y<sy; y++)
          for (int x=0; x<sx; x++)
            src[y][x]=frame[y+1][x+1];

        unsigned char cconv[256];
        memset(&cconv[0],255,256);

        unsigned keyc=frame[0][0];
        for (y=0; y<sy; y++)
        {
          Video::progress_bar(progress_current+1,progress_max);
          progress_current++;
          for (int x=0; x<sx; x++)
          {
            unsigned i=y*sx+x;
            unsigned c=src.adr[i];
            if (c==keyc)
              src.adr[i]=(unsigned char)255;
            else
            {
              if (cconv[c]==255)
                if (!onecolor)
                  cconv[c]=(unsigned char)Video::closest(Color(pal[c][0],pal[c][1],pal[c][2]));
                else
                  cconv[c]=(unsigned char)Video::closest(Color(Color::cwhite));
              src.adr[i]=cconv[c];
            }
          }
        }

        int ox=-1,oy=-1;
        for (int i=1; i<sx+1; i++)
          if ((unsigned)frame[sy+1][i]!=keyc)
            ox=i-1;
        for (i=1; i<sy+1; i++)
          if ((unsigned)frame[i][sx+1]!=keyc)
            oy=i-1;

       if ((ox==-1)||(oy==-1))
        {
          ox=sx/2;
          oy=sy/2;
        }

        phase[frame_num-1].rebuild(ox,oy,scale,0/*sensitivity*/,mirrors);
      }
      catch (Failure) {FAILURE (F_flcerr,frame_num);}
    }

    close_flic();

    Video::restore_background();

    save_prepared();

    MESSAGE("%s %dx%d scale:%s mirr:%s 1col:%s (%s) rebuilt and saved",
      master_name,sx+2,sy+2,
        M_scale[scale],
        M_withm[mirrors?1:0],
        M_onec[onecolor?1:0],name);
  }
  catch (Failure)
  {
    if (frame.buf) Heap::free(frame.buf,FILE_LINE);
    if (src.adr) Heap::free(src.adr,FILE_LINE);
    buffer.quit();
    free();
    FAILURE(F_cantrebu);
  }
  if (frame.buf) Heap::free(frame.buf,FILE_LINE);
  if (src.adr) Heap::free(src.adr,FILE_LINE);
  buffer.quit();
  free();
}