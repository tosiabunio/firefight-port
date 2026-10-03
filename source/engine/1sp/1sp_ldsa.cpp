#include "1sp_hdrs.h"
#include "1sp_ldsa.h"

// Port: was the cache file format (load_prepared/save_prepared). The definitions now stay in
// memory: keep_prepared copies them out of the build buffer into one heap block, in the layout
// the cache file had (all phases back to back, mirrored definitions after them), and points the
// phases into it.

static int block_size (const unsigned char *def)
{
  int size;
  memcpy(&size,def,sizeof(size));  // each compressed definition starts with its byte length
  return(size);
}

void Lsprite::keep_prepared(void)
{
  int i,s=0,ms=0;
  for (i=0; i<phases; i++)
    s+=block_size(phase[i].def[0]);
  if (mirrors)
    for (i=0; i<phases; i++)
      ms+=block_size(phase[i].def[1]);

  rledata_size=s+ms;
  rledata=(unsigned char*)Heap::alloc(rledata_size,trace_name);
  Phase *kept=(Phase*)Heap::alloc(phases*sizeof(Phase),"[spr] tmp phases");

  unsigned char *out=rledata;
  for (i=0; i<phases; i++)
  {
    int size=block_size(phase[i].def[0]);
    memcpy(out,phase[i].def[0]+sizeof(int),size);
    kept[i]=phase[i];
    kept[i].def[0]=kept[i].def[1]=out;
    out+=size;
  }
  if (mirrors)
    for (i=0; i<phases; i++)
    {
      int size=block_size(phase[i].def[1]);
      memcpy(out,phase[i].def[1]+sizeof(int),size);
      kept[i].def[1]=out;
      out+=size;
    }

  Heap::free(phase,FILE_LINE);
  phase=kept;
}
