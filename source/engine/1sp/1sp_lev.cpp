#include "1sp_hdrs.h"

const unsigned short reserved_first=0xffff;
const int            reserved_handle=0xffffffff;

static char trace_leveldef[] = "[lev] data";

static char F_tmtypes[]   = "too many types defined";
static char F_typesntx[]  = "syntax error in type definition %d";
static char F_typelong[]  = "'%s' type name too long";
static char F_sprsntx[]   = "syntax error in sprite declaration %d";
static char F_undeftype[] = "undefined type '%s' in sprite %d";
static char F_spn2long[]  = "'%s' sprite name too long";
static char F_paln2long[] = "'%s' palette name too long";
static char F_levelsntx[] = "syntax error in level declaration %d";
static char F_nolevdef[]  = "no levels defined";
static char F_objsntx[]   = "syntax error in level%d_plane%d";
static char F_cantread[]  = "can't read level '%s' [Level::load]";
static char F_badlink[]   = "bad reglink in object %d [Level::look_at]";
static char F_uobjtype[]  = "undefined type '%s' in level%d_plane%d";
static char F_uobjspr[]   = "undeclared sprite '%s' in level%d_plane%d";
static char F_noroom[]    = "no room for new object";
static char F_outreg[]    = "%s %s[%d] (%d,%d) out of regions [Level::update]";
static char F_invupnum[]  = "can't update object %d [Level::update]";
static char F_buildmod[]  = "trying to update object %d while building object %d [Level::update]";
static char F_nobuild[]   = "no builder registered for %s [Level::build]";

static char F_spr4led[]   = "unable to access level sprite %d (check '-' flag)";
static char F_nlsprite[]  = "unable to access level sprite %d (check '*' flag)";

#if HI_DEBUG
static int now_building;
#endif

const int buffer_size=2048;
static unsigned short buffer[buffer_size];
static int    lsize;

extern int  level_inled;
       int  level_inled;

static int default_remove_sprite (char *) { return(0); }
extern int (*level_remove_sprite) (char *);
       int (*level_remove_sprite) (char *)=default_remove_sprite;

static void default_level_progress (int) {}
void (*Level::progress) (int)=default_level_progress;

Sprite &Level::Levsprite::soper (int num)
{
  if((is_loaded[num]==led_only)&&(!level_inled))
    FAILURE (F_spr4led,num);

  if((is_loaded[num]<loaded)&&(!level_inled))
    FAILURE (F_nlsprite,num);

  return(sprite[num]);
}

static void skip_spaces (char *&c)
{
  while (*c==' ') c++;
}

static int get_string (char *res, char *&c)
{
  while ((*c==' ')||(*c==0x0d)||(*c==0x0a)) c++;
  if (*c==0) return(0);
  while ((*c!=' ')&&(*c!=0x0a)&&(*c!=0x0d)&&(*c!=0)) *(res++)=*(c++);
  *(res++)=0;
  return(1);
}

static int get_dec (int &res, char *&c)
{
  res=0;
  while ((*c==' ')||(*c=='.')||(*c==0x0a)||(*c==0x0d)) c++;
  if (*c==0) return(0);
  while ((*c!=' ')&&(*c!='.')&&(*c!=0x0a)&&(*c!=0x0d)&&(*c!=0))
  {
    if (res) res*=10;
    if ((*c>='0')&&(*c<='9'))
      res+=(*c-'0');
    else
      return(0);
    c++;
  }
  return(1);
}

static int get_hex (int &res, char *&c)
{
  res=0;
  while ((*c==' ')||(*c==0x0a)||(*c==0x0d)) c++;
  if (*c==0) return(0);
  while ((*c!=' ')&&(*c!=0x0a)&&(*c!=0x0d)&&(*c!=0))
  {
    if (res) res*=16;
    if ((*c>='0')&&(*c<='9'))
      res+=(*c-'0');
    else
      if ((*c>='a')&&(*c<='f'))
        res+=(*c-'a')+10;
      else
        if ((*c>='A')&&(*c<='F'))
          res+=(*c-'A')+10;
        else
          return(0);
    c++;
  }
  return(1);
}

void get_links (unsigned short *olinks, Level &level)
{
  int i,l,n,m;
  unsigned short *link=(unsigned short*)Heap::alloc
    (Level::max_size*sizeof(short),"[lev] tmp links");
  for (i=1; i<level.size; i++)
    if (olinks[i*2+0])
    {
      memset (&link[0],0,sizeof(short)*Level::max_size);
      n=olinks[i*2+0];
      l=i;
      m=0;
      do
      {
        if (olinks[l*2+0]==n)
        {
          olinks[l*2+0]=0;
          link[olinks[l*2+1]]=(unsigned short)l;
          if (l>m) m=l;
        }
        l++;
      }
      while (l<level.size);

      int s=0;
      for (l=0; l<=m; l++)
      {
        if (link[l])
          if (s==0)
            s=n=link[l];
          else
          {
            level[n].link=link[l];
            n=link[l];
          }
      }
      if (n!=s)
        level[n].link=(unsigned short)s;
    }
  Heap::free(link,FILE_LINE);
}

void Level::load (char *_filename, char *l2load)
{
  memset (this,0,sizeof(*this));

  int umem=(int)Heap::mem_avail();

  char filename[_MAX_PATH];
  strupr(strcpy(filename,_filename));
  MESSAGE("loading level %s...",filename);

  char names[max_sprites][max_sprname+1];
  int  toremovenum=0;
  char *c;
  int  i,j,l,p,v;
  int really_loaded=0;

  try
  {
    char toremove [max_sprites][max_sprname+1];
    memset (&toremove[0][0],0,sizeof(toremove));

    Text t;
    File::area(filename);
    t.load("source");
    t.find_unused_labels=0;
    File::endarea();

    sprites_num=0;
    size=1;
    for (j=1; j<max_levels; j++)
    {
      if (strchr(l2load,j+'0'))
        lloaded[j]=1;
      for (i=1; i<max_planes; i++)
      {
        plane[j][i].begin = 1;
        plane[j][i].size  = 0;
        plane[j][i].mulx  = plane[j][i].muly  = (float)1;
        plane[j][i].regsx = max_sx/max_regnumx;
        plane[j][i].regsy = max_sy/max_regnumy;
        plane[j][i].gridx = plane[j][i].gridy = 8;
      }
    }

    object=(Lobject*)Heap::alloc(sizeof(Lobject)*max_size,"[lev] tmp objects");
    unsigned short *olinks=(unsigned short*)Heap::alloc
      (max_size*sizeof(short)*2,"[lev] tmp olinks");

    Lobject o;
    memset(&o,0,sizeof(o));
    for(i=0; i<max_size; i++)
    {
      object[i]=o;
      olinks[i*2+0]=0;
      olinks[i*2+1]=0;
    }

    File::area(filename);
    t.check("palette");
    if(strlen(t.string())>max_sprname) FAILURE(F_paln2long,t.string());
    strcpy(palette_name,t.string());
    Video::load_palette(palette_name);
    File::endarea();

    t.check("types");
    if (t.size()>max_types) FAILURE (F_tmtypes);

    for (i=0; i<t.size(); i++)
    {
      char *c=t.string(i);

      if (!get_hex(v,c)) FAILURE (F_typesntx,i+1);
      status[i]=(short)v;
      if (!get_hex(v,c)) FAILURE (F_typesntx,i+1);
      aux1[i]=(short)v;
      if (!get_hex(v,c)) FAILURE (F_typesntx,i+1);
      aux2[i]=(short)v;

      skip_spaces(c);
      if (strlen(c)==0) FAILURE (F_typesntx,i+1);
      if (strlen(c)>max_typename) FAILURE (F_typelong,c);
      strcpy(type[i],c);
      types_num++;
    }

    t.check("sprites");
    for (i=0; i<t.size(); i++)
    {
      char *c=t.string(i);
      if (c[0]=='*')
      {
        sprite.is_loaded[i]=Levsprite::always;
        c++;
      }
      else if (c[0]=='-')
      {
        sprite.is_loaded[i]=Levsprite::led_only;
        c++;
      }

      if (!get_string (names[i],c)) FAILURE (F_sprsntx,i+1);
      strupr(names[i]);
      if (strlen(names[i])>max_sprname) FAILURE (F_spn2long,names[i]);
      skip_spaces(c);

      int num=0;
      while((num<types_num)&&strcmpi(c,type[num])) num++;
      if (num==types_num) FAILURE (F_undeftype,c,i+1);
      deftype[i]=num;
      sprites_num++;
    }

    if (t.size("main_level")) main_level=t.value();
    if (main_level==0) main_level=1;

    int levels_defined=0;
    for (l=1; l<max_levels; l++)
    {
      char level_label[80];
      sprintf (level_label,"level%d",l);
      if(t.size(level_label)&&lloaded[l])
      {
        levels_defined++;
        defined[l]=1;
        for (p=0; p<max_planes; p++)
        {
          char *c=t.string(p);
          if (p==0)
          {
            if (!get_dec(active[l],c)) FAILURE (F_levelsntx,l);
            if (!get_dec(homex[l],c)) FAILURE (F_levelsntx,l);
            if (!get_dec(homey[l],c)) FAILURE (F_levelsntx,l);
          }
          else
          {
            plane[l][p].reginfo=(Reginfo*)Heap::alloc(sizeof(Reginfo)*max_regnum,"[lev] reginfo");
            int v1;
            if (!get_dec(v,c)) FAILURE (F_levelsntx,l);
            c++;
            if (!get_dec(v1,c)) FAILURE (F_levelsntx,l);
            plane[l][p].mulx = float(v)+float(v1)/float(1000);
            plane[l][p].regsx = (int)(max_sx*plane[l][p].mulx)/max_regnumx;
            if (plane[l][p].regsx==0) plane[l][p].regsx=32767;
            if (!get_dec(v,c)) FAILURE (F_levelsntx,l);
            c++;
            if (!get_dec(v1,c)) FAILURE (F_levelsntx,l);
            plane[l][p].muly = float(v*1000+v1)/float(1000);
            plane[l][p].regsy = (int)(max_sy*plane[l][p].muly)/max_regnumy;
            if (plane[l][p].regsy==0) plane[l][p].regsy=32767;
            if (!get_dec(v,c)) FAILURE (F_levelsntx,l);
            if ((v<4)||(v>=320)) FAILURE (F_levelsntx,l);
            plane[l][p].gridx = v;
            if (!get_dec(v,c)) FAILURE (F_levelsntx,l);
            if ((v<4)||(v>=200)) FAILURE (F_levelsntx,l);
            plane[l][p].gridy = v;
            if (!get_dec(v,c)) FAILURE (F_levelsntx,l);
            if (v)
            {
              int r,ll=l,pp=p;
              if ((r=v)>(max_size-size)) FAILURE (F_levelsntx,l);
              plane[ll][pp].reserved=r;
              plane[ll][pp].size=r;
              memset(&o,0,sizeof(o));
              for(v=0; v<r; v++)
                object[plane[ll][pp].begin+v]=o;
              pp++;
              while (ll<max_levels)
              {
                while (pp<max_planes)
                  plane[ll][(pp++)].begin+=r;
                pp=1;
                ll++;
              }
              size+=r;
            }
          }
        }
      }
    }
    if (!levels_defined)
      FAILURE (F_nolevdef);

    for (l=1; l<max_levels; l++)
      for (p=1; p<max_planes; p++)
      {
        char plane_label[80];
        sprintf (plane_label,"level%d_plane%d",l,p);
        if(t.size(plane_label)&&lloaded[l]&&defined[l])
        {
          int k;
          int pos=plane[l][p].begin+plane[l][p].size;
          for (k=0; k<t.size(plane_label); k++)
          {
            char *c=t.string(k);
            char sprname[80];
            if(!get_string(sprname,c)) FAILURE (F_objsntx,l,p);
            for (v=0; (v<sprites_num)&&strcmpi(sprname,names[v]); v++);
            if (v<sprites_num)
            {
              size++;
              if (size>=max_size) FAILURE (F_noroom);
              memmove (&object[pos+1], &object[pos], (size-pos)*sizeof(Lobject));
              plane[l][p].size++;
              i=p+1;
              for (j=l; j<max_levels; j++)
              {
                while (i<max_planes)
                  plane[j][(i++)].begin++;
                i=1;
              }

              Lobject *o=&object[pos];
              o->sprite=(unsigned short)v;
              if (!get_dec(v,c)) FAILURE (F_objsntx,l,p); // faza
              o->phase=(unsigned short)v;
              if (!get_dec(v,c)) FAILURE (F_objsntx,l,p); // lustro
              o->mirror=(unsigned short)v;
              if (!get_dec(v,c)) FAILURE (F_objsntx,l,p); // pozycja x
              o->x=v;
              if (!get_dec(v,c)) FAILURE (F_objsntx,l,p); // pozycja y
              o->y=v;
              if (!get_dec(v,c)) FAILURE (F_objsntx,l,p); // link
              olinks[pos*2+0]=(unsigned short)v;
              if (!get_dec(v,c)) FAILURE (F_objsntx,l,p); // lpos
              olinks[pos*2+1]=(unsigned short)v;
              o->link=0;
              if (!get_dec(v,c)) FAILURE (F_objsntx,l,p); // spec
              o->special=(unsigned short)v;

              skip_spaces(c);
              int num=0;
              while((num<types_num)&&strcmpi(c,type[num])) num++;
              if (num==types_num) FAILURE (F_uobjtype,c,l,p);

              o->type=(unsigned short)num;
              o->status=status[num];
              o->aux1=aux1[num];
              o->aux2=aux2[num];
              o->active=1;

              if (sprite.is_loaded[o->sprite]==Levsprite::if_used)
                sprite.is_loaded[o->sprite]=Levsprite::loaded;

              pos++;
            }
            else
            {
              for (v=0; (v<toremovenum)&&strcmpi(sprname,toremove[v]); v++);
              if (v==toremovenum)
              {
                if (!level_remove_sprite(sprname))
                  FAILURE (F_uobjspr,sprname,l,p);
                strcpy(toremove[toremovenum],sprname);
                toremovenum++;
              }
            }
          }
        }
      }

    {
      int r=0,g=0,b=0;
      if (t.size("back_color"))
      {
        char *c=t.string();
        get_dec(r,c); get_dec(g,c); get_dec(b,c);
      }
      backcolor=Color(r,g,b);
    }

    get_links(olinks,*this);
    Heap::free(olinks,FILE_LINE);

    t.find_unused_labels=1;
    t.free();

    c=(char*)object;
    if (level_inled)
      object=(Lobject*)Heap::alloc(sizeof(Lobject)*max_size,trace_leveldef);
    else
      object=(Lobject*)Heap::alloc(sizeof(Lobject)*size,trace_leveldef);
    memcpy(object,c,size*sizeof(Lobject));
    Heap::free(c,FILE_LINE);

    name=strcpy((char*)Heap::alloc(strlen(filename)+1,trace_leveldef),filename);

    for (i=0; i<sprites_num; i++)
    {
      Level::progress(i*100/sprites_num);
      if ((sprite.is_loaded[i]==Levsprite::if_used)&&(!level_inled))
      {
        if(!Comm::production) MESSAGE("sprite %d (%s) not loaded (unused)",i,names[i]);
      }
      else if ((sprite.is_loaded[i]==Levsprite::led_only)&&(!level_inled))
      {
        if(!Comm::production) MESSAGE("sprite %d (%s) not loaded ('-' flag)",i,names[i]);
      }
      else
      {
        int num;
        for (num=0; num<i; num++)
          if (strcmpi(names[i],names[num])==0)
            break;
        if ((num<i)&&(sprite.is_loaded[num]==Levsprite::loaded))
        {
          sprite.sprite[i]=sprite[num];
          if(!Comm::production) MESSAGE("sprite %d (%s) cloned from sprite %d",
            i,names[i],num);
        }
        else
        {
          File::area(filename);
          sprite.sprite[i].load(names[i]);
          File::endarea();
          really_loaded++;
        }
      }
    }

    if (level_inled)
      regnode=(Regnode*)Heap::alloc(sizeof(Regnode)*max_size,"[lev] regnodes");
    else
      regnode=(Regnode*)Heap::alloc(sizeof(Regnode)*size,"[lev] regnodes");
    update(0);
  }
  catch (Failure)
  {
    FAILURE (F_cantread,filename);
  }

  used_memory=umem-(int)Heap::mem_avail();
  MESSAGE("...%s[%s]: %d(-%d) sprites, %d objects, %d bytes",
    filename,l2load,sprites_num,sprites_num-really_loaded,size,used_memory);
}

int Level::oplane(int num)
{
  int p=1,l=1;
  while(plane[l][p].begin+plane[l][p].size<=num)
  {
    p++;
    if (p==max_planes)
    {
      p=0;
      l++;
    }
  }
  return(p);
}

int Level::olevel(int num)
{
  int p=1,l=1;
  while(plane[l][p].begin+plane[l][p].size<=num)
  {
    p++;
    if (p==max_planes)
    {
      p=0;
      l++;
    }
  }
  return(l);
}

void Level::update (int num)
{
  if (num==0)
  {
    // updatuj wszystkie obiekty
    //
    for (int l=1; l<max_levels; l++)
      for (int p=1; p<max_planes; p++)
        if(plane[l][p].reginfo)
          for (int i=0; i<max_regnum; i++)
          {
            plane[l][p].reginfo[i].maxl=plane[l][p].reginfo[i].maxu=0;
            plane[l][p].reginfo[i].first=reserved_first;
          }
    int i;
    for (i=1; i<size; i++)
    {
      regnode[i].next=regnode[i].prev=0;
      regnode[i].handle=reserved_handle;
    }
    for (i=1; i<size; i++)
      update(i);
  }
  else
  {
    // odłącz obiekt od starego regionu
    //
#if HI_DEBUG
  if ((num<1)||(num>=size))
    FAILURE (F_invupnum,num);
  if (now_building&&(num!=now_building))
    FAILURE (F_buildmod,num,now_building);
#endif
    int p=oplane(num);
    int l=olevel(num);
    Reginfo *reginfo=plane[l][p].reginfo;
    Regnode &oldreg=regnode[num];
    
    if (oldreg.handle!=reserved_handle)
    {
      int oldhandle=oldreg.handle;
      unsigned short oldnext=oldreg.next;
      regnode[oldreg.next].prev=oldreg.prev;
      regnode[oldreg.prev].next=oldreg.next;
      oldreg.next=oldreg.prev=0;
      oldreg.handle=reserved_handle;

      if (reginfo[oldhandle].first==num)
        if (oldnext!=num)
          reginfo[oldhandle].first=oldnext;
        else
          reginfo[oldhandle].first=reserved_first;
    }

    if (object[num].active)
    {
      Lobject *o =&object[num];
      Phase *sp  =&sprite[o->sprite][o->phase];
      int le=o->x+sp->l;
      int re=o->x+sp->r;
      int ue=o->y+sp->u;
      int de=o->y+sp->d;
      int rsx=plane[l][p].regsx;
      int rsy=plane[l][p].regsy;
#if HI_DEBUG
      if (((le/rsx)>=max_regnumx)||((ue/rsy)>=max_regnumy)||
      ((le/rsx)<0)||((ue/rsy)<0))
        FAILURE (F_outreg,
          type[o->type], sprite[o->sprite].name, o->x, o->y);
#endif

      // znajdź regiony w które włazi sprajt
      //
      int fromy=ue/rsy; if(fromy<0) fromy=0;
      int fromx=le/rsx; if(fromx<0) fromx=0;
      int toy=de/rsy;   if(toy>=max_regnumy) toy=max_regnumy-1;
      int tox=re/rsx;   if(tox>=max_regnumx) tox=max_regnumx-1;

      for (int y=fromy; y<=toy; y++)
        for (int x=fromx; x<=tox; x++)
        {
          int r=(y*max_regnumx) + x;
          if (reginfo[r].maxu<y-ue/rsy) reginfo[r].maxu=(char)(y-ue/rsy);
          if (reginfo[r].maxl<x-le/rsx) reginfo[r].maxl=(char)(x-le/rsx);
        }

      // sortuj obiekt wewnątrz regionu
      //
      int newhandle=(ue/rsy)*max_regnumx+(le/rsx);
      DBG_CHECK(newhandle<max_regnum);

      unsigned short &currfirst=reginfo[newhandle].first;

      if (currfirst==reserved_first)
      {
        currfirst=(unsigned short)num;
        regnode[num].next=(unsigned short)num;
        regnode[num].prev=(unsigned short)num;
        regnode[num].handle=newhandle;
      }
      else
      {
        int i=currfirst;
        if (num>currfirst)
        {
          while ((regnode[i].next!=currfirst)&&(regnode[i].next<num))
            i=regnode[i].next;
          i=regnode[i].next;
        }

        // dołącz do regionu w znalezionym miejscu
        //
        Regnode &newreg=regnode[num];

        newreg.next=(unsigned short)i;
        newreg.prev=regnode[i].prev;
        regnode[i].prev=(unsigned short)num;
        regnode[newreg.prev].next=(unsigned short)num;

        newreg.handle=newhandle;

        if (num<currfirst)
          currfirst=(unsigned short)num;
      }
    }
  }
}

int lgcompare (void const *a, void const *b)
{
  return ((int)(*(unsigned short*)a)-(int)(*(unsigned short*)b));
}

void Level::build (int num, int l, int p)
{
  Buildable::bo_num=num;
#if HI_DEBUG
  now_building=num;
#endif
  Lobject *o=&object[num];

  Buildable::bo=o;
  Buildable::bo_spr=&sprite[o->sprite];
  Buildable::bo_plane=p;
  Buildable::bo_level=l;
  Buildable::bo_master=this;
  Buildable::bo_mustupdate=0;
#if HI_DEBUG
  if (!builder[o->type])
    FAILURE (F_nobuild,type[o->type]);
#endif
  builder[o->type]->builder();
  if (Buildable::bo_mustupdate)
    Buildable::bo_master->update(Buildable::bo_num);
#if HI_DEBUG
  now_building=0;
#endif
}

void Level::look_at (int wx, int wy, int wsx, int wsy, char *gp, int byes, int oyes)
{
  int i,l,aplane,lminx,lminy,lmaxx,lmaxy;
  char *gplanes=gp;

  l=(*(gplanes++))-'0';
  aplane=1;
  lsize=1;

  for (aplane=1; (aplane<max_buf_planes)&&(*gplanes!=0); aplane++)
  {
    if (*gplanes==' ')
    {
      buf[aplane].orgx=(short)wx;
      buf[aplane].orgy=(short)wy;
      buf[aplane].object=&buffer[0];
      gplanes++;
    }
    else
    {
      int olsize=lsize;
      int p;
      if (*gplanes=='*')
      {
        gplanes++;
        l=(*(gplanes++))-'0';
      }
      p=(*(gplanes++))-'0';
      Reginfo *reginfo=plane[l][p].reginfo;

      lminx=(int)(wx*plane[l][p].mulx);
      lminy=(int)(wy*plane[l][p].muly);
      lmaxx=lminx+wsx;
      lmaxy=lminy+wsy;

      buf[aplane].object=&buffer[lsize];
      buf[aplane].orgx=(short)-lminx;
      buf[aplane].orgy=(short)-lminy;

      int regsy=plane[l][p].regsy;
      int regsx=plane[l][p].regsx;
      int toy=lminy/regsy;
      int otoy=toy;
      int tox=lminx/regsx;
      int otox=tox;
      for (int ry=lmaxy/regsy; ry>=toy; ry--)
        for (int rx=lmaxx/regsx; rx>=tox; rx--)
        {
          if ((rx>=0)&&(rx<max_regnumx)&&(ry>=0)&&(ry<max_regnumy))
          {
            int ibeg=(ry*max_regnumx) + rx;
            if ((ry>=otoy)&&(ry-reginfo[ibeg].maxu<toy))
              toy=ry-reginfo[ibeg].maxu;
            if ((rx>=otox)&&(rx-reginfo[ibeg].maxl<tox))
              tox=rx-reginfo[ibeg].maxl;

            i=reginfo[ibeg].first;
            while (reginfo[ibeg].first!=reserved_first)
            {
              int next=regnode[i].next;
#if HI_DEBUG
              if ((next<1)||(next>=size))
                FAILURE (F_badlink,i);
#endif
              Lobject *o=&object[i];
              Phase *sp=&sprite[o->sprite][o->phase];
              if (
                (sp->l+o->x<lmaxx)&&
                (sp->u+o->y<lmaxy)&&
                (sp->r+o->x>=lminx)&&
                (sp->d+o->y>=lminy))
              {
                if (builder[o->type]&&byes)
                  build(i,l,p);
                if(o->active)
                {
                  if (oyes)
                  {
                    buffer[lsize]=(unsigned short)i;
                    lsize++;
                  }
                }
              }

              if (i>=next)
                break;
              i=next;
            }
          }
        }
        msvc4_qsort(&buffer[olsize],lsize-olsize,sizeof(short),lgcompare);  // was qsort (compat/msvc4.h)
      buffer[lsize] = 0;
      lsize++;
      CHECK(lsize<buffer_size);
    }
  }
}

int Level::typenum (char *name)
{
  int num=0;
  while((num<types_num)&&strcmpi(name,type[num])) num++;
  if (num==types_num) return(0);
  return(num);
}

int typecomp (char *t1, char *t2)
{
  while (*t2)
  {
    if (*t1!='?')
      if (tolower(*t1)!=tolower(*t2)) return (0);
    t1++;
    t2++;
  }
  return(1);
}

int Level::bregister (char *type_name, Buildable &buildable)
{
  char name [max_typename+1];
  memset(name,'?',sizeof(name));
  int num;
  for (num=0; (num<sizeof(name))&&(type_name[num])&&(type_name[num]!='*'); num++)
    name[num]=type_name[num];
  int found=0;
  for (num=1; num<types_num; num++)
    if (typecomp(name,type[num]))
      builder[(found=num)]=&buildable;
  return(found);
}

int      Buildable::bo_mustupdate;
Lobject *Buildable::bo;
int      Buildable::bo_num;
int      Buildable::bo_plane;
int      Buildable::bo_level;
Level   *Buildable::bo_master;
Sprite  *Buildable::bo_spr;

Level::Level ()
{
  memset (this,0,sizeof(*this));
}

Level::~Level ()
{
  free();
}

void Level::free(void)
{
  if (regnode) Heap::free (regnode,FILE_LINE);
  for (int l=1; l<max_levels; l++)
    for (int p=1; p<max_planes; p++)
      if(plane[l][p].reginfo)
      {
        Heap::free (plane[l][p].reginfo,FILE_LINE);
        plane[l][p].reginfo=NULL;
      }
  if (object) Heap::free (object,FILE_LINE);
  if (name) Heap::free (name,FILE_LINE);
  regnode=NULL;
  object=NULL;
  name=NULL;
  ok=0;
}
