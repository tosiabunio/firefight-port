#ifndef _LDS_HPP_INCLUDED
#define _LDS_HPP_INCLUDED

class Lsprite
{
public:
  struct Phase
  {
    unsigned char *def[2];   // adresy definicji
    short          ox;       // przesuniecie x normalne
    short          oy;       // przesuniecie y normalne
    short          sx;       // szerokosc
    short          sy;       // wysokosc

    void rebuild(int _ox, int _oy, int scale, int sens, int mirr);
  };

  Phase *phase;
  unsigned char *rledata;
  int rledata_size;

  char master_name[Text::max_area];
  char source_name[Text::max_area];
  int  source_datetime;
  char *trace_name;

  char name[Text::max_area];
  int scale;
  int specified;
  int wanted;
  int mirrors;
  int ready;
  int mem_used;
  int onecolor;

  struct Frame
  {
    int llen;
    unsigned char *buf;
    unsigned char *operator[] (int y) { return(&buf[llen*y]); }
  } frame;

  int sx,sy,frame_num;
  int phases;

  void rebuild (void);
  void measure (void);
  void keep_prepared (void);

  void free(void);
  ~Lsprite();

public:
  Lsprite (char *_name, char *_source,
    int _scale, char *rle_name, char *_master, int _wanted, int _onecolor);
  void load(void);
};

#endif
