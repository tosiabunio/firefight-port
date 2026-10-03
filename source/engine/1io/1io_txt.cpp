#include "1io_hdrs.h"

static inline char *align_up (char *p, size_t a)
{
  return (char*)(((uintptr_t)p+(a-1))&~(uintptr_t)(a-1));
}

#define OLD_LOGGING_STYLE 0
#define NEW_LOGGING_STYLE (1-OLD_LOGGING_STYLE)

#define is_space(c) (((c>=0x09)&&(c<=0x0D))||(c==0x20))

char *Text::last=(char*)-1;
char Text::area_char = '/';
char Text::missing_label[Text::max_area+1];

static char E_labtoolong[] = "line %d: label too long, max=%d chars";
static char E_noendsect[]  = "line %d: missing 'endsection'";
static char E_syntax[]     = "line %d: syntax error";
static char E_syntax2[]    = "line %d-%d: syntax error";
static char E_wintxt[]     = "line %d: mixed area styles";
static char E_morelab[]    = "label '%s' appears more than once";
static char E_noroom[]     = "no room for new label '%s'";
static char E_areanc[]     = "'%s' area without 'endarea'";
static char E_loadfail[]   = "loading '%s' failed [Text::load]";

static char E_nolabel[]    = "'%s:/%s' no such label [Text]";
static char E_nolast[]     = "last text is not available [Text]";
static char F_onotinit[]   = "object not initialized [Text::%s]";

static const char CR = 0x0d;
static const char LF = 0x0a;

int Text::find_unused_labels_disable;

static const int max_active=8;
static char *active_sections[max_active];
static int loaded_sections[max_active];

static const int hstat_size=6;

static Text *now_loading;

static int missed [hstat_size];

static int lowercase_tab[256];
static inline int lowercase (int c) { return(lowercase_tab[c]); }
struct Lc_init { Lc_init() { for (int i=0; i<256; i++) lowercase_tab[i]=tolower(i); } } static lc_init;

//-----------------------------------------------------------------------------

static inline int find_char (char *&src, int &len, int &lines)
{
  int rem=0;
  while (len>0)
  {
    switch (*src)
    {
      case ';' :
      case '#' :
        rem=1;
        break;
      case LF  :
        if (rem)
          rem=0;
        lines++;
        break;
      case ' ' :
      case '\t':
      case '\v':
      case CR  :
      case 0x1a:
        break;
      default  :
        if (!rem)
          return (1);
    }
    src++;
    len--;
  }
  return(0);
}

static inline int get_label (char *&dst, char *&src, int &len, int lines)
{
  int end=0;
  if (now_loading->arealen)
  {
    strcpy(dst,now_loading->areaid);
    int l=strlen(dst);
    dst+=l;
  }

  int l;
  for (l=0; (l<Text::max_label+2)&&(!end)&&(len>0); l++)
  {
    char c=*src;
    if ((c=='_') || ((c>='a')&&(c<='z')) || ((c>='A')&&(c<='Z')) || ((c>='0')&&(c<='9')))
    {
      *(dst++)=*src;
      src++;
      len--;
    }
    else
      end++;
  }

  if (l==Text::max_label+2)
    FAILURE(E_labtoolong,lines,Text::max_label);
  *(dst++)=0;
  return(1);
}

static inline int skip_eoln (char *&src, int &len, int &lines)
{
  while ((len>0) && (*src!=LF))
  {
    src++;
    len--;
  }
  if (len>0)
  {
    src++;
    len--;
    lines++;
    return(1);
  }
  else
    return (0);
}

static inline int get_text (char *&dst, char *&src, int &len)
{
  src++;
  len--;
  while (((*src!='"')||(*(src+1)=='"'))&&(len>0))
  {
    if (*src==CR)
      return(0);
    if (*src==LF)
      return(0);
    if (*src=='\\')
    {
      src++;
      len--;
      switch (lowercase (*src))
      {
        case 'a' : *dst=0x07; break;
        case 'b' : *dst=0x08; break;
        case 'f' : *dst=0x0C; break;
        case 'n' : *dst=0x0A; break;
        case 'r' : *dst=0x0D; break;
        case 't' : *dst=0x09; break;
        case 'v' : *dst=0x0B; break;
        default  : *dst=*src;
      }
    }
    else
    {
      if ((*src=='"')&&(*(src+1)=='"'))
      {
        *dst='"';
        src++;
        len--;
      }
      else
        *dst=*src;
    }
    dst++;
    src++;
    len--;
  }
  if (!(len>0))
    return(0);
  *(dst++)=0;
  src++;
  len--;
  return(1);
}

static inline int get_formated_text (char *&dst, char *&src, int &len, int &lines)
{
  if (!skip_eoln(src,len,lines)) return (0);
  while ((*src!='}')&&(len>0))
  {
    if (*src==LF)
      lines++;
    if (*src=='{')
      return(0);
    if (*src!=CR)
    {
      *dst=*src;
      dst++;
    }
    src++;
    len--;
  }
  if (!(len>0))
    return(0);
  src++;
  len--;
  if (*(dst-1)==LF) dst--;
  *(dst++)=0;
  return(1);
}

static inline int get_menu_text (char *&dst, char *&src, int &len, int &lines, int *&tnum)
{
  int str_len;
  src++;
  len--;
  if (!find_char(src,len,lines)) return (0);
  do
  {
    while (((*src==' ')||(*src=='\t'))&&(len>0)) {src++; len--;}
    if (len==0) return (*tnum);
    str_len=0;
    while ((*src!=';')&&(*src!='#')&&(*src!=CR)&&(*src!=LF)&&(*src!=0x1a)&&(len>0))
    {
      *(dst++)=*(src)++;
      len--;
      str_len++;
    }
    dst--;
    while(is_space(*dst))
    {
      dst--;
      str_len--;
    }
    dst++;
    if (str_len==0) return (1);
    *(dst++)=0;
    (*tnum)++;
    if (len==0) return (1);
  } while (skip_eoln(src,len,lines));
  return (1);
}

static inline int get_line_text (char *&dst, char *&src, int &len)
{
  int str_len;
  src++;
  len--;
  while (((*src==' ')||(*src=='\t'))&&(len>0)) {src++; len--;}
  if (len==0) return (0);
  str_len=0;
  while ((*src!=';')&&(*src!='#')&&(*src!=CR)&&(*src!=LF)&&(*src!=0x1a)&&(len>0))
  {
    *(dst++)=*(src)++;
    len--;
    str_len++;
  }
  dst--;
  while(is_space(*dst))
  {
    dst--;
    str_len--;
  }
  dst++;
  if (str_len==0) return (0);
  *(dst++)=0;
  return (1);
}

static inline void convert_texts(char *dst, char *src, int len, int &groups_num, int &strings_num)
{
  int lines=1;
  char *dsts=dst;
  int ready=0;
  int *tnum;
  int area_cnt=0,section_cnt=0;
  int win_area_opened=0;
  int txt_area_opened=0;

  tnum=NULL;
  while (!ready)
  {
    if (find_char(src,len,lines)||(len==0))
    {
      if (((*src!='"')&&(*src!='{')&&(*src!=':')&&(*src!='=')) || (len==0))
      {
        char str[128];
        str[0]=0;
        if (len) sscanf(src,"%s",&str[0]);
        if (strcmpi(str,"section")==0)
        {
          int sectline=lines;
          len-=strlen(str);
          src+=strlen(str);
          char sectname[256];
          sscanf(src,"%s",&sectname[0]);
          skip_eoln(src,len,lines);
          if (strstr(src,"endsection")==NULL)
            FAILURE(E_noendsect,sectline);
          int sect_found=0;
          for (int i=0; i<max_active; i++)
            if (active_sections[i])
              if (strcmpi(sectname,active_sections[i])==0)
              {
                sect_found++;
                loaded_sections[i]++;
              }
          if (!sect_found)
          {
            while(src!=strstr(src,"endsection"))
            {
              skip_eoln(src,len,lines);
              find_char(src,len,lines);
            }
            skip_eoln(src,len,lines);
          }
          section_cnt++;
        }
        else if (strcmpi(str,"endsection")==0)
        {
          if (!section_cnt)
            FAILURE(E_syntax,lines);
          section_cnt--;
          skip_eoln(src,len,lines);
        }
        else if (str[0]=='[')
        {
          if (txt_area_opened)
            FAILURE(E_wintxt,lines);

          len--;
          src++;
          char sectname[256];
          sscanf(src,"%s",&sectname[0]);
          int sl=strlen(sectname);
          if (sectname[sl-1]!=']')
            FAILURE(E_syntax,lines);
          sectname[sl-1]=0;

          skip_eoln(src,len,lines);
          now_loading->area();
          now_loading->area(sectname);
          win_area_opened=1;
        }
        else if (strcmpi(str,"area")==0)
        {
          if (win_area_opened)
            FAILURE(E_wintxt,lines);
          len-=strlen(str);
          src+=strlen(str);
          char sectname[256];
          sscanf(src,"%s",&sectname[0]);
          skip_eoln(src,len,lines);
          now_loading->area(sectname);
          area_cnt++;
          txt_area_opened=1;
        }
        else if (strcmpi(str,"endarea")==0)
        {
          if (win_area_opened)
            FAILURE(E_wintxt,lines);
          if (!area_cnt)
            FAILURE(E_syntax,lines);
          area_cnt--;
          skip_eoln(src,len,lines);
          now_loading->endarea();
        }
        else
        {
          if ((tnum!=NULL)&&(*tnum))
          {
            groups_num++;
            strings_num+=(*tnum);
          }
          if (len==0)
          {
            ready++;
          }
          else
          {
            int labelline=lines;
            if (get_label(dst,src,len,lines))
            {
              tnum=(int*)dst;
              *tnum=0;
              dst+=sizeof(int);
              find_char(src,len,lines);
              if (((*src!='"')&&(*src!='{')&&(*src!=':')&&(*src!='=')) || (len==0))
                FAILURE(E_syntax,labelline);
            }
//            else
//              ready++;
          }
        }
      }
      else
      {
        int openline=lines;
        if (tnum==NULL)
          FAILURE(E_syntax,openline);
        if (*src=='"') // jeśli to string w cudzysłowie
        {
          if (get_text(dst,src,len)==0)
            FAILURE(E_syntax2,openline,lines);
          (*tnum)++;
        }
        else if (*src=='{') // jeśli to string w nawiasach
        {
          if (get_formated_text(dst,src,len,lines)==0)
            FAILURE(E_syntax2,openline,lines);
          (*tnum)++;
        }
        else if (*src==':') // etykieta zakończona dwukropkiem
        {
          if (get_menu_text(dst,src,len,lines,tnum)==0)
            FAILURE(E_syntax2,openline,lines);
        }
        else if (*src=='=') // etykieta zakończona równasiem
        {
          if (get_line_text(dst,src,len)==0)
            FAILURE(E_syntax,lines);
          (*tnum)++;
        }
      }
    }
  }
  if (win_area_opened)
    now_loading->area();
}

static char **sorted;
static int sorted_pos;
static int sorted_compare (void const *a, void const *b)
{
  return (strcmpi(*(char**)a,*(char**)b));
}


int Text::compile_texts (char *dst, char *src)
{
  int len=0;

  if (dst==NULL)
  {
    len+=4;
    int i,j=0;
    for (i=groups_num*2; ; i++)
    {
      for (j=3; (j<i) && (i%j!=0); j++);
      if(j==i) break;
    }
    if (j<11) j=11;
    hash_table_size=j;

    len+=hash_table_size*sizeof(int)+groups_num*sizeof(Group);
  }
  else
  {
    sorted=(char**)mem_alloc(groups_num*sizeof(char*),"sorted");
    sorted_pos=0;

    *(int*)dst=*(int*)"txts";
    dst+=4;
    hash_table=(int*)dst;
    dst+=hash_table_size*sizeof(int);
    for (int i=0; i<hash_table_size; i++)
      hash_table[i]=-1;

    group=(Group*)dst;
    dst+=groups_num*sizeof(Group);
  }

  for (int gn=0; gn<groups_num; gn++)
  {
    if (dst!=NULL)
    {
      group[gn].label=dst;
      sorted[(sorted_pos++)]=dst;
    }

    if (arealen>0)
    {
      char *asrc=&areaid[0];
      do
      {
        if (dst==NULL)
          len++;
        else
          *(dst++)=(*asrc);
        asrc++;
      }
      while(*(asrc)!=0);
    }

    do
    {
      if (dst==NULL)
        len++;
      else
        *(dst++)=(*src);
      src++;
    }
    while(*(src-1)!=0);

    if (dst!=NULL)
      add_group (gn);

    int tnum;
    memcpy(&tnum,src,sizeof(int));  // unaligned in the parsed source
    src+=sizeof(int);

    // Port: the pointer array and the int value slots are aligned (64-bit pointers); the
    // sizing pass reserves the worst-case padding.
    if (dst==NULL)
      len+=sizeof(char*)-1+sizeof(char*)*tnum;
    else
    {
      dst=align_up(dst,sizeof(char*));
      group[gn].texts_num=tnum;
      group[gn].texts=(char**)dst;
      group[gn].used=0;
      dst+=sizeof(char*)*tnum;
    }

    int maxw=0;

    for (int t=0; t<tnum; t++)
    {
      if (dst==NULL)
      {
        len+=sizeof(int)-1+sizeof(int);
        int w=strlen(src);
        len+=w+1;
        src+=w+1;
      }
      else
      {
        dst=align_up(dst,sizeof(int));
        *(int*)dst=0; // value
        *dst=*src; // niezainicjalizowane
        dst+=sizeof(int);

        group[gn].texts[t]=dst;
        int w=strlen(src);
        memcpy(dst,src,w);
        dst[w]=0;
        dst+=w+1;
        src+=w+1;

        if (w>maxw)
          maxw=w;
      }
    }

    if (dst!=NULL)
      group[gn].max_width=maxw;
  }

  if (dst!=NULL)
  {
    qsort(&sorted[0],groups_num,sizeof(char*),sorted_compare);
//    for (int gn=0; gn<groups_num; gn++)
//      DBG_MESSAGE("label %04d: %s",gn,sorted[gn]);
    mem_free(sorted,"sorted");
  }

  return(len);
}

void Text::dir_load(int handle,char* name)
{
  unsigned len=_filelength(handle);
  if(len==-1) FAILURE("unable to load: %s",name);
  char* src=(char*)((*mem_alloc)(len+1,"[txt] tmp src"));
  if(_read(handle,src,len)==-1) FAILURE("unable to load: %s");
  _close(handle);
  src[len]=0;
  process(src,len,name);
  if(src) mem_free(src,FILE_LINE);
}

void Text::load (char *file_name)
{
  char* src=NULL;
  try {
    char filename[Text::max_label+1];
    DBG_CHECK(strlen(file_name)<=Text::max_label);
    strcpy(filename,file_name);
    strupr(filename);
#if OLD_LOGGING_STYLE
    MESSAGE("loading text %s...",filename);
#endif
    File::open(file_name);
    unsigned len=File::size();
    src=(char*)((*mem_alloc)(len+1,"[txt] tmp src"));
    File::read(src,len);
    File::close();
    src[len]=0;
    process(src,len,file_name);
#if OLD_LOGGING_STYLE
    MESSAGE("...%s: %d labels, %d strings, %d bytes",filename,groups_num,strings_num,used_memory);
#else
    if(!Comm::production) MESSAGE("text %s, %d labels, %d strings, %d bytes",filename,groups_num,strings_num,used_memory);
#endif
    if (sections!=NULL)
      for (int i=0; i<max_active; i++)
        if (active_sections[i])
          if (loaded_sections[i])
            if(!Comm::production) MESSAGE("section %s found %d times",active_sections[i],loaded_sections[i]);
          else
            if(!Comm::production) MESSAGE("section %s not found",active_sections[i]);
  } catch(Failure) {
    if(src) mem_free(src,FILE_LINE);
    FAILURE(E_loadfail,file_name);
  }
  mem_free(src,FILE_LINE);
}

void Text::process (char* src,unsigned len,char* current_name)
{
  ok=1;
  now_loading=this;

  areaid[0]=0;
  arealen=groups_num=strings_num=0;

  char *buf;
  try
  {
    memset(active_sections,0,sizeof(active_sections));
    memset(loaded_sections,0,sizeof(loaded_sections));
    if (sections)
    {
      char ase[8][80];
      memset(ase,0,sizeof(ase));
      sscanf(sections,"%s %s %s %s %s %s %s %s",
        ase[0],ase[1],ase[2],ase[3],ase[4],ase[5],ase[6],ase[7]);
      for (int i=0; i<8; i++)
        if (strlen(ase[i]))
          active_sections[i]=ase[i];
    }

    buf=(char*)((*mem_alloc)(len*2,"[txt] tmp buf"));
    convert_texts(buf,src,len,groups_num,strings_num);

    len=compile_texts(NULL,buf);
    int name_len=strlen(current_name)+1;
    used_memory=len+name_len;
    data_adr=(char*)((*mem_alloc)(used_memory,"[txt] data"));
    compile_texts(data_adr,buf);
    name=strcpy(&data_adr[len],current_name);
    mem_free(buf,FILE_LINE);
  }
  catch (Failure)
  {
    ok=0;
    if (buf) mem_free(buf,FILE_LINE);
    FAILURE("processing aborted [Text::process]");
  }

  if (arealen) FAILURE(E_areanc,areaid);

  find_unused_labels=1;
}

//-----------------------------------------------------------------------------

static inline int hash (char *label)
{
  int res=0,i=0;
  char *c=label;
  while (*c)
  {
    res=(res<<1)+lowercase(*(c++));
    i++;
  }
  res^=i;
  return (abs(res));
}

int Text::find_label (char *label)
{
  static int firsttime=1;

  if (label==last)
    if ((firsttime)||(last_result==-1))
    {
      find_unused_labels=0;
      FAILURE (E_nolast);
    }
    else
      return (last_result);
  firsttime=0;

  last_result=-1;
  int m=0;
  int h=hash(label)%hash_table_size;
  int start=h;
  int step=3;

  do
  {
    int n=hash_table[h];
    if (n==-1)
      break;
    if (strcmpi(label,group[n].label)==0)
    {
      if (m>hstat_size-1)
        m=hstat_size-1;
      if(group[n].used==0)
        missed[m]++;
      group[n].used++;
      last_result=n;
      return(n);
    }
    m++;
    h=(h+step)%hash_table_size;
    if(step+step+1<hash_table_size)
      step+=step+1;
  } while (h!=start);

  strcpy(missing_label,label);
  strupr(missing_label);
  return(-1);
}

void Text::add_group (int group_num)
{
  char *label=group[group_num].label;
  int h=hash(label)%hash_table_size;
  int start=h;
  int step=3;
  do
  {
    int n=hash_table[h];
    if (n==-1)
    {
      hash_table[h]=group_num;
      return;
    }
    else
      if (strcmpi(label,group[n].label)==0)
        FAILURE(E_morelab,label);
    h=(h+step)%hash_table_size;
    if(step+step+1<hash_table_size)
      step+=step+1;
  } while (h!=start);
  FAILURE(E_noroom,label);
}

//-----------------------------------------------------------------------------

char *Text::string (char *l, int strnum)
{
  DBG_CHECK(ok);

  char *label=l;
  if (l!=last)
    label= (l[0]==area_char) ? l+1 : strcat(areaid,l);
  int n=find_label(label);
  if (n==-1)
  {
    find_unused_labels=0;
    FAILURE(E_nolabel,name,label);
  }
  areaid[arealen]=0;
  return(group[n].texts[strnum]);
}

int Text::value (char *l, int strnum)
{
  char *src=string(l,strnum);
  int *val=(int*)(src-sizeof(int));

  if(*(src-sizeof(int))==*src)
  {
    if ((strcmpi(src,"yes")==0)||(strcmpi(src,"on")==0)||(strcmpi(src,"true")==0))
      val[0]=1;
    else if ((strcmpi(src,"no")==0)||(strcmpi(src,"off")==0)||(strcmpi(src,"false")==0))
      val[0]=0;
    else
    {
      if ((strstr(src,"0x")!=NULL)||(strstr(src,"0X")!=NULL))
        sscanf(src,"%x",val);
      else
        sscanf(src,"%d",val);
    }
  }
  return(val[0]);
}

float Text::fp_value (char *l, int strnum)
{
  char *src=string(l,strnum);
  float result;

  sscanf(src,"%f",&result);
  return(result);
}

int Text::size (char *l)
{
  DBG_CHECK(ok);

  char *label=l;
  if (l!=last)
    label= (l[0]==area_char) ? l+1 : strcat(areaid,l);
  int n=find_label(label);
  areaid[arealen]=0;
  if (n==-1)
    return(0);
  else
    return(group[n].texts_num);
}

int Text::width (char *l)
{
  DBG_CHECK(ok);

  char *label=l;
  if (l!=last)
    label= (l[0]==area_char) ? l+1 : strcat(areaid,l);
  int n=find_label(label);
  areaid[arealen]=0;
  if (n==-1)
    return(0);
  else
    return(group[n].max_width);
}

//-----------------------------------------------------------------------------

void Text::display_unused(void)
{
  if (find_unused_labels&&(!find_unused_labels_disable))
  {
    int n=0;
    int i;
    for (i=0; i<groups_num; i++)
      if (group[i].used==0)
        n++;
    if (n)
    {
      MESSAGE("unused labels in %s:",name);
      for (i=0; i<groups_num; i++)
        if (group[i].used==0)
          MESSAGE("- %s",group[i].label);
    }
    find_unused_labels=0;
  }
}

Text::Text ()
{
  find_unused_labels=0;
  areaid[0]=0;
  arealen=0;
  data_adr=NULL;
  sections=NULL;
  ok=0;
  mem_alloc=Heap::alloc;
  mem_free=Heap::free;
}

void Text::free(void)
{
  if (data_adr)
  {
    display_unused();
    mem_free(data_adr,FILE_LINE);
  }
  data_adr=NULL;
  ok=0;
}

Text::~Text ()
{
  free();
}

void Text::area(char *name)
{
  if (!ok) FAILURE (F_onotinit,"area");
  if (name==0)
  {
    areaid[0]=0;
    arealen=0;
  }
  else
  {
    if (name[0]==area_char)
    {
      name++;
      arealen=0;
    }
    if (name[0])
    {
      int l=strlen(name);
      memcpy(areaid+arealen,name,l);
      arealen+=l;
      areaid[(arealen++)]=area_char;
      areaid[arealen]=0;
    }
  }
}

void Text::endarea(void)
{
  if (!ok) FAILURE (F_onotinit,"endarea");
  if (arealen>0)
  {
    arealen--;
    while ((areaid[arealen-1]!=area_char)&&arealen)
      arealen--;
    areaid[arealen]=0;
  }
}

void Text::init (void)
{
  for (int i=0; i<hstat_size; i++)
    missed[i]=0;
}

void Text::quit (void)
{
  int n=0;
  for (int i=0; i<hstat_size; i++)
    if (missed[i])
      n+=missed[i];

  if (n)
  {
    char s[80]; s[0]=0;
    int e=0;
    for (int i=0; i<hstat_size; i++)
    {
      char t[80];
      sprintf(t,"%05.2f ",(double)missed[i]*100/n);
      strcat(s,t);
      e+=missed[i]*(i+1);
    }
    ENGINFO("efficiency %d%%, avg dist: %s(%%)",10000/(e*100/n),s);
  }
}

