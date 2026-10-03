#include "1io_hdrs.h"

#pragma warning (disable: 4244)

#define fast_copy(src,dst,cnt) memmove(dst,src,cnt)

#define U(X) ((unsigned) X)

/* The following define defines the length of the copy flag that appears at   */
/* the start of the compressed file. I have decided on four bytes so as to    */
/* make the source and destination longword aligned in the case where a copy  */
/* operation must be performed.                                               */
/* The actual flag data appears in the first byte. The rest are zero.         */
#define FLAG_BYTES    4     /* How many bytes does the flag use up?           */

/* The following defines define the meaning of the values of the copy         */
/* flag at the start of the compressed file.                                  */
#define FLAG_COMPRESS 0     /* Signals that output was result of compression. */
#define FLAG_COPY     1     /* Signals that output was simply copied over.    */

/******************************************************************************/

#define PS *p++!=*p_src++  /* Body of inner unrolled matching loop.     */
#define ITEMMAX 18         /* Max number of bytes in an expanded item.  */
#define TOPWORD 0xFFFF0000

void __compress(unsigned char* p_wrk_mem,unsigned char* p_src_first,unsigned src_len,unsigned char* p_dst_first,unsigned* p_dst_len)
/* Input  : Specify input block using p_src_first and src_len.          */
/* Input  : Point p_dst_first to the start of the output zone (OZ).     */
/* Input  : Point p_dst_len to a unsigned to receive the output length. */
/* Input  : Input block and output zone must not overlap.               */
/* Output : Length of output block written to *p_dst_len.               */
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
/* Output : May write in OZ=Mem[p_dst_first..p_dst_first+src_len+288-1].*/
/* Output : Upon completion guaranteed *p_dst_len<=src_len+FLAG_BYTES.  */
{
  register unsigned char *p_src=p_src_first,*p_dst=p_dst_first;
  unsigned char *p_src_post=p_src_first+src_len,*p_dst_post=p_dst_first+src_len;
  unsigned char *p_src_max1,*p_src_max16;

  /* The following longword aligns the hash table in the working memory. */
  register unsigned char **hash= (unsigned char **) (( ((unsigned) p_wrk_mem) +3) & 0xFFFFFFFC);
  unsigned char *p_control;
  register unsigned control=TOPWORD;

  p_src_max1=p_src_post-ITEMMAX;
  p_src_max16=p_src_post-16*ITEMMAX;
  *p_dst=FLAG_COMPRESS;
  {
    unsigned short i;
    for (i=1;i<FLAG_BYTES;i++)
      p_dst[i]=0;
  }
  p_dst+=FLAG_BYTES;
  p_control=p_dst;
  p_dst+=2;
  for(;;)
  {
    register unsigned char *p,**p_entry;
    register unsigned short unroll=16;
    register unsigned offset;
    if (p_dst>p_dst_post) goto overrun;
    if (p_src>p_src_max16)
    {
      unroll=1;
      if (p_src>p_src_max1)
      {
        if (p_src==p_src_post) break;
        goto literal;
      }
    }
    begin_unrolled_loop:
        p_entry=&hash
          [((40543*((((p_src[0]<<4)^p_src[1])<<4)^p_src[2]))>>4) & 0xFFF];
        p=*p_entry;
        *p_entry=p_src;
        offset=p_src-p;
        if (offset>4095 || p<p_src_first || offset==0 || PS || PS || PS)
        {
          p_src=*p_entry;
          literal:
          *p_dst++=*p_src++;
          control&=0xFFFEFFFF;
        }
        else
        {
          (void)(PS || PS || PS || PS || PS || PS || PS || PS ||
          PS || PS || PS || PS || PS || PS || PS || p_src++);
          *p_dst++=((offset&0xF00)>>4)|(--p_src-*p_entry-3);
          *p_dst++=offset&0xFF;
        }
        control>>=1;
//    end_unrolled_loop:
    if (--unroll) goto begin_unrolled_loop;
    if ((control&TOPWORD) == 0)
    {
      *p_control=control&0xFF;
      *(p_control+1)=(control>>8)&0xFF;
      p_control=p_dst;
      p_dst+=2;
      control=TOPWORD;
    }
  }
  while (control&TOPWORD)
    control>>=1;
  *p_control++=control&0xFF;
  *p_control++=control>>8;
  if (p_control==p_dst) p_dst-=2;
  *p_dst_len=p_dst-p_dst_first;
  return;
  overrun:
  fast_copy(p_src_first,p_dst_first+FLAG_BYTES,src_len);
  *p_dst_first=FLAG_COPY;
  *p_dst_len=src_len+FLAG_BYTES;
}

/******************************************************************************/

void __decompress(unsigned char* wrk_mem,unsigned char* p_src_first,unsigned src_len,unsigned char* p_dst_first,unsigned* p_dst_len)
/* Input  : Specify input block using p_src_first and src_len.          */
/* Input  : Point p_dst_first to the start of the output zone.          */
/* Input  : Point p_dst_len to a unsigned to receive the output length. */
/* Input  : Input block and output zone must not overlap. User knows    */
/* Input  : upperbound on output block length from earlier compression. */
/* Input  : In any case, maximum expansion possible is nine times.      */
/* Output : Length of output block written to *p_dst_len.               */
/* Output : Output block in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
/* Output : Writes only  in Mem[p_dst_first..p_dst_first+*p_dst_len-1]. */
{
  register unsigned char *p_src=p_src_first+FLAG_BYTES, *p_dst=p_dst_first;
  unsigned char *p_src_post=p_src_first+src_len;
  unsigned char *p_src_max16=p_src_first+src_len-(16*2);
  register unsigned control=1;

  if (*p_src_first==FLAG_COPY)
  {
    fast_copy(p_src_first+FLAG_BYTES,p_dst_first,src_len-FLAG_BYTES);
    *p_dst_len=src_len-FLAG_BYTES;
    return;
  }
  while (p_src!=p_src_post)
  {
    register unsigned short unroll;
    if (control==1)
    {
      control=0x10000|*p_src++; control|=(*p_src++)<<8;
    }
    unroll= p_src<=p_src_max16 ? 16 : 1;
    while (unroll--)
    {
      if (control&1)
      {
        register unsigned short lenmt;
        register unsigned char *p;
        lenmt=*p_src++;
        p=p_dst-(((lenmt&0xF0)<<4)|*p_src++);
        *p_dst++=*p++;
        *p_dst++=*p++;
        *p_dst++=*p++;
        lenmt&=0xF;
        while (lenmt--)
          *p_dst++=*p++;
      }
      else
        *p_dst++=*p_src++;
      control>>=1;
    }
  }
  *p_dst_len=p_dst-p_dst_first;
  (void*)wrk_mem;
}

#pragma warning (default: 4244)


