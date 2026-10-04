#include "1sp_hdrs.h"
#include "1sp_ldsa.h"

//----- komunikaty -----------------------------------------------------------

static char F_notspec[]   = "none %s specified in volume directory";
static char F_nosrc[]     = "%s specified in volume directory not exist";
static char F_missfor[]   = "no %s specified for %s in volume directory";

static char F_parterr[]   = "unable to load %s data";
static char F_loaderr[]   = "loading sprite '%s' failed";
static char F_pmis_e[]    = "prologue missing";
static char F_pmis_l[]    = "prologue missing";

//----- stale i zmienne ------------------------------------------------------

static char source_id[]= "source";
static char hsource_id[]="hsource";
static char lsource_id[]="lsource";
static char csource_id[]="csource";
static char hires_id[]=  "hires";
static char lores_id[]=  "lores";
static char collis_id[]= "collis";

static int hwanted,lwanted,cwanted;
static int hmemused,lmemused,cmemused;

static int minim (int a, int b, int c)
{
  if (a<b)
    return((a<c) ?a :c);
  return((b<c) ?b :c);
}

static int maxim (int a, int b, int c)
{
  if (a>b)
    return((a>c) ?a :c);
  return((b>c) ?b :c);
}

void Spr::load_prologue (int toload)
{
  if (toload==-1)
    toload=Spr::toload;
  hwanted=(toload&hires)  ?1 :0;
  lwanted=(toload&lores)  ?1 :0;
  cwanted=(toload&collis) ?1 :0;
  hmemused=lmemused=cmemused=0;
  char *yesno[2]={"[no]","[yes]"};
  MESSAGE("prologue: hires %s, lores %s, collis %s",
    yesno[hwanted],yesno[lwanted],yesno[cwanted]);
  load_enabled=1;
}

void Spr::load_epilogue (int update_palette)
{
  if (!load_enabled)
    FAILURE (F_pmis_e);
  load_enabled=0;
  if(update_palette)
    Video::update_palette();
  Video::free_paltabs();
  int snum=0, pnum=0;
  for (Sprite::Data *ptr=Sprite::first.next; ptr!=&Sprite::first; ptr=ptr->next)
  {
    pnum+=ptr->phases;
    snum++;
  }
  MESSAGE("epilogue: h:%d l:%d c:%d bytes, %d sprites, %d phases",
    hmemused,lmemused,cmemused,snum,pnum);
  if(Video::current_mode!=-1)
    Spr::touch(Video::mode_flag|Spr::collis);
}

//----- zarzadca ladowania ---------------------------------------------------

Lsprite::Lsprite(char *_name, char *_source, int _scale,
  char *rle_name, char *_master, int _wanted, int _onecolor)
{
  phase=NULL;
  rledata=NULL;

  scale=_scale;
  source_datetime=ready=phases=0;

  strcpy(name,_name);
  strcpy(master_name,_master);

  specified=File::specified(_name);
  wanted=_wanted;

  trace_name=rle_name;
  onecolor=_onecolor;
  mirrors=0;

  if (!File::volume())
  {
    strcpy(source_name,source_id);

    int specsrc=0;
    if(File::specified(_source))
    {
      strcpy(source_name,_source);
      specsrc=1;
    }

    if(File::specified(source_name))
    {
      if (!File::exist(source_name))
        FAILURE (F_nosrc,source_name);
      source_datetime=File::date(source_name);
    }

    if ((specified)&&(!source_datetime))
      FAILURE (F_missfor,"source",name);
    if ((!specified)&&(specsrc)&&(source_datetime))
      FAILURE (F_missfor,"target",source_name);
  }

  if (specified)
  {
    File::get_flags(name);
    char *flags=File::info.flags;
    if (*flags)
    {
      if (strstr(flags,"m+")!=NULL)
        mirrors=1;
      if (strstr(flags,"m-")!=NULL)
        mirrors=0;
      if (strstr(flags,"o+")!=NULL)
        onecolor=1;
      if (strstr(flags,"o-")!=NULL)
        onecolor=0;
      if (strstr(flags,"1x1")!=NULL)
        scale=1;
      if (strstr(flags,"2x1")!=NULL)
        scale=2;
      if (strstr(flags,"4x1")!=NULL)
        scale=4;
      if (strstr(flags,"8x1")!=NULL)
        scale=8;
    }
  }
}

Lsprite::~Lsprite ()
{
  free();
}

void Lsprite::free (void)
{
  if (phase) Heap::free(phase,FILE_LINE);
  phase=NULL;
}

// Port: every sprite is built in memory from its FLC master. The original loaded the prepared
// definitions from a cache file (.sph/.spc) and rebuilt that file when it was missing or stale.
void Lsprite::load (void)
{
  try
  {
    if (wanted&&specified)
    {
      rebuild();
      ready=1;
    }
  }
  catch (Failure)
  {
    FAILURE (F_parterr,name);
  }
}

// Port: with sprite_dump=1, describes the sprite just loaded (see Spr::dump_printf): its phase
// bounds, "<target> <phases> l,r,u,d[*repeat] ...", and the size and CRC-32 of its hires and
// collision data, "<target> hires <size> <crc> collis <size> <crc>".
static void dump_sprite (Phase *phase, int phases, Lsprite &h, Lsprite &c)
{
  if (!Spr::dump)
    return;
  File::get_flags(File::specified(hires_id) ?hires_id :File::specified(collis_id) ?collis_id :lores_id);
  char target[sizeof(File::info.real_name)];
  strcpy(target,File::info.real_name);

  Spr::dump_printf(0,"%s %d",target,phases);
  for (int i=0; i<phases; )
  {
    int n=1;
    while ((i+n<phases)&&(phase[i+n].l==phase[i].l)&&(phase[i+n].r==phase[i].r)&&
      (phase[i+n].u==phase[i].u)&&(phase[i+n].d==phase[i].d))
      n++;
    Spr::dump_printf(0,(n>1) ?" %d,%d,%d,%d*%d" :" %d,%d,%d,%d",
      phase[i].l,phase[i].r,phase[i].u,phase[i].d,n);
    i+=n;
  }
  Spr::dump_printf(0,"\n");

  Spr::dump_printf(1,"%s",target);
  if (h.phases)
    Spr::dump_printf(1," hires %d %08x",h.rledata_size,Spr::crc32(h.rledata,h.rledata_size));
  if (c.phases)
    Spr::dump_printf(1," collis %d %08x",c.rledata_size,Spr::crc32(c.rledata,c.rledata_size));
  Spr::dump_printf(1,"\n");
}

void Sprite::load (char *_filename)
{
  int umem=(int)Heap::mem_avail();

  char filename[_MAX_PATH];
  strcpy(filename,_filename);
  strupr(filename);

  File::area(filename);
  try
  {
    if (!Spr::load_enabled)
      FAILURE (F_pmis_l);

    Lsprite h (hires_id,hsource_id,Spr::hires_scale,"[spr] hires data",
      filename,hwanted,(Spr::onecolor&Spr::hires)?1:0);
    Lsprite l (lores_id,lsource_id,Spr::lores_scale,"[spr] lores data",
      filename,lwanted,(Spr::onecolor&Spr::lores)?1:0);
    Lsprite c (collis_id,csource_id,Spr::collis_scale,"[spr] collis data",
      filename,cwanted,(Spr::onecolor&Spr::collis)?1:0);

    if (!File::volume())
    {
      if ((!h.source_datetime)&&(!l.source_datetime)&&(!c.source_datetime))
        FAILURE (F_notspec,"source");
      if ((!h.specified)&&(!l.specified)&&(!c.specified))
        FAILURE (F_notspec,"target");
    }

    int memused;

    memused=(int)Heap::mem_avail();
    h.load();
    hmemused+=(int)memused-Heap::mem_avail();

    memused=(int)Heap::mem_avail();
    // Port: was l.load(). The lores pixels are never drawn (hires only), but the lores bounds
    // count towards the phase bounds below, so measure the target from its master (lsource or
    // source) at its scale (a 1x1 flag on the target line overrides Spr::lores_scale).
    if (l.wanted&&l.specified)
      l.measure();
    lmemused+=(int)memused-Heap::mem_avail();

    memused=(int)Heap::mem_avail();
    c.load();
    cmemused+=(int)memused-Heap::mem_avail();

    maxl=0,maxr=0,maxu=0,maxd=0;
    phases=maxim(h.phases,l.phases,c.phases);

    if (phases)
    {
      int tmplen=sizeof(Data)+(strlen(filename)+1)+phases*sizeof(Phase);
      unsigned char *tmpptr=(unsigned char*)Heap::alloc(tmplen,"[spr] info");

      data=(Data*)tmpptr;
      data->owners=1;
      tmpptr+=sizeof(Data);

      name=strcpy((char*)tmpptr,filename);
      tmpptr+=(strlen(filename)+1);

      phase=(Phase*)tmpptr;
      memset(phase,0,phases*sizeof(Phase));
      data->phase=phase;
      data->phases=phases;

      if(first.next!=&first)
      {
        first.next->prev =data;
        data->next       =first.next;
        first.next       =data;
        data->prev       =&first;
      }
      else
      {
        first.next =first.prev =data;
        data->prev =data->next =&first;
      }

      if (h.phases) rledata[0]=h.rledata;
      if (l.phases) rledata[1]=l.rledata;
      if (c.phases) rledata[2]=c.rledata;

      for (int i=0; i<phases; i++)
      {
        Phase::Type &ph=phase[i].hires;
        Phase::Type &pl=phase[i].lores;
        Phase::Type &pc=phase[i].collis;

        if (h.phases>i)
        {
          ph.def[0]=h.phase[i].def[0];
          ph.def[1]=h.phase[i].def[1];
          ph.real_l=(short)-h.phase[i].ox;
          ph.real_r=(short)(-h.phase[i].ox+h.phase[i].sx);
          ph.real_u=(short)-h.phase[i].oy;
          ph.real_d=(short)(-h.phase[i].oy+h.phase[i].sy);
          ph.l=(short)(ph.real_l/2);
          ph.r=(short)(ph.real_r/2);
          ph.u=(short)(ph.real_u/2);
          ph.d=(short)(ph.real_d/2);
        }
 
        if (l.phases>i)
        {
          pl.def[0]=l.phase[i].def[0];
          pl.def[1]=l.phase[i].def[1];
          pl.real_l=(short)-l.phase[i].ox;
          pl.real_r=(short)(-l.phase[i].ox+l.phase[i].sx);
          pl.real_u=(short)-l.phase[i].oy;
          pl.real_d=(short)(-l.phase[i].oy+l.phase[i].sy);
          pl.l=(short)(pl.real_l);
          pl.r=(short)(pl.real_r);
          pl.u=(short)(pl.real_u);
          pl.d=(short)(pl.real_d);
        }

        if (c.phases>i)
        {
          pc.def[0]=c.phase[i].def[0];
          pc.def[1]=c.phase[i].def[1];
          pc.real_l=(short)-c.phase[i].ox;
          pc.real_r=(short)(-c.phase[i].ox+c.phase[i].sx);
          pc.real_u=(short)-c.phase[i].oy;
          pc.real_d=(short)(-c.phase[i].oy+c.phase[i].sy);
          pc.l=(short)(pc.real_l*2);
          pc.r=(short)(pc.real_r*2);
          pc.u=(short)(pc.real_u*2);
          pc.d=(short)(pc.real_d*2);
        }

        phase[i].l=(short)minim(ph.l,pl.l,pc.l);
        phase[i].r=(short)maxim(ph.r,pl.r,pc.r);
        phase[i].u=(short)minim(ph.u,pl.u,pc.u);
        phase[i].d=(short)maxim(ph.d,pl.d,pc.d);

        if (phase[i].l<maxl) maxl=phase[i].l;
        if (phase[i].r>maxr) maxr=phase[i].r;
        if (phase[i].u<maxu) maxu=phase[i].u;
        if (phase[i].d>maxd) maxd=phase[i].d;
      }
      dump_sprite(phase,phases,h,c);
    }
  }
  catch (Failure)
  {
    File::endarea();
    FAILURE (F_loaderr,filename);
  }
  File::endarea();

  used_memory=umem-(int)Heap::mem_avail();
  if(!Comm::production) MESSAGE("sprite %s[%d] (%d,%d)(%d,%d) %d bytes",
    filename,phases,maxl,maxu,maxr,maxd,used_memory);
}