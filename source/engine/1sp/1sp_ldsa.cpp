#include "1sp_hdrs.h"
#include "1sp_ldsa.h"

//----- komunikaty -----------------------------------------------------------

static char F_saveerr[] = "unable to save sprite";
static char id[]="CWE sprite";

struct
{
  int file_level, source_datetime, mirrors, onecolor, scale;
  int data_offset[2];
  int data_size;
  int phases;
} static hdr;

//----- wczytanie spreparowanego sprajta -------------------------------------

void Lsprite::load_prepared(void)
{
  try
  {
    if (!File::volume())
      if (!File::exist(name))
        throw must_rebuild("destination not found");

    File f;
    f.open(name);

    // identyfikator
    char _id[100];
    f.read(_id,strlen(id)+1);
    if(strcmp(id,_id)!=0)
      throw must_rebuild("invalid file");

    // naglowek
    int _hdrsize;
    f.read((char*)&_hdrsize,sizeof(_hdrsize));
    if(_hdrsize!=sizeof(hdr))
      throw must_rebuild("invalid header");
    f.read((char*)&hdr,sizeof(hdr));

    if(hdr.file_level!=spr_file_level)              throw must_rebuild("different file level");
    if (!File::volume())
      if(hdr.source_datetime!=int(source_datetime)) throw must_rebuild("source date changed");
    if(hdr.mirrors!=mirrors)                        throw must_rebuild("different mirror flag");
    if((!hdr.onecolor)!=(!onecolor))                throw must_rebuild("different onecolor flag");
    if(hdr.scale!=scale)                            throw must_rebuild("different scale factor");

    // dane rle
    int rledata_size=hdr.data_size;
    rledata=(unsigned char*)Heap::alloc(rledata_size,trace_name);
    f.read(rledata,rledata_size);

    // fazy
    phases=hdr.phases;
    phase=(Phase*)Heap::alloc(phases*sizeof(Phase),"[spr] tmp phases");
    f.read((char*)phase,phases*sizeof(Phase));

    int i;
    int offset=(int)(rledata+hdr.data_offset[0]);
    for (i=0; i<phases; i++)
    {
      phase[i].def[0]+=offset;
      phase[i].def[1]=phase[i].def[0];
    }
    if(mirrors)
    {
      offset=(int)(rledata+hdr.data_offset[1]);
      for (i=0; i<phases; i++)
        phase[i].def[1]+=offset;
    }

    // paleta
    if (!File::volume())
    {
      Color palette[256];
      f.read((char*)&palette[0],sizeof(palette));
      for(int i=Video::min_color; i<Video::max_color; i++)
        if((Video::palette[i])!=(palette[i]))
          throw must_rebuild("different palette");
    }

    f.close();
  }
  catch (must_rebuild)
  {
    if (phase) Heap::free(phase,FILE_LINE);
    if (rledata) Heap::free(rledata,FILE_LINE);
    phase=NULL;
    rledata=NULL;
    throw must_rebuild(must_rebuild::reason);
  }
}

//----- zapis spreparowanego sprajta -----------------------------------------

void Lsprite::save_prepared(void)
{
  try
  {
    File::create (name);

    // identyfikator
    File::write(id, strlen(id)+1);

    // naglowek
    hdr.file_level=spr_file_level;
    hdr.source_datetime=source_datetime;
    hdr.mirrors=mirrors;
    hdr.onecolor=onecolor;
    hdr.scale=scale;

    int i,s=0,ms=0;
    for (i=0; i<phases; i++)
      s+=*(int*)phase[i].def[0];
    hdr.data_offset[0]=0;

    if (mirrors)
    {
      for (i=0; i<phases; i++)
        ms+=*(int*)phase[i].def[1];
      hdr.data_offset[1]=s;
    }
    else
      hdr.data_offset[1]=0;

    hdr.data_size=s+ms;

    hdr.phases=phases;

    int v=sizeof(hdr);
    File::write(&v,sizeof(v));
    File::write(&hdr,v);

    // dane rle
    unsigned char *vorg=0;
    for (i=0; i<phases; i++)
    {
      unsigned char *pdata=phase[i].def[0];
      
      File::write(pdata+sizeof(int),*(int*)pdata);
      phase[i].def[0]=vorg;
      vorg+=*(int*)pdata;
    }

    vorg=0;
    if(mirrors)
      for (i=0; i<phases; i++)
      {
        unsigned char *pdata=phase[i].def[1];
        File::write(pdata+sizeof(int),*(int*)pdata);
        phase[i].def[1]=vorg;
        vorg+=*(int*)pdata;
      }

    // fazy
    File::write(&phase[0], sizeof(phase[0])*phases);

    // paleta
    File::write(&Video::palette[0], sizeof(Video::palette));
  }
  catch (Failure) { FAILURE (F_saveerr); }
  File::close();
}
