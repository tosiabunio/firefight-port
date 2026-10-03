#ifndef _EEM_DEMO_H_INCLUDED
#define _EEM_DEMO_H_INCLUDED

struct Demo_header
{
  unsigned control_sum;                               
  unsigned data_version;
  unsigned store_method;
  unsigned size;
  unsigned blocks;
  unsigned players_num;
};

#define store_Plain 1
#define store_LZW   2
#define store_RLE8  3

class Demo_recorder 
{
  static char *main_buffer;
  static char *cprs_buffer;
  static Demo_header header;
  static unsigned buf_size;
  static unsigned version_of_data;
  static Bitflag status;
  static char rec_file_name[_MAX_PATH]; 
public:
  static void  init   (unsigned data_version = 0);
  static void  quit   (void);
  static void  open   (char *file_name = NULL, char *user_data = NULL, 
                       unsigned user_data_size = 0, unsigned store_method = store_LZW, 
                       int players_num=1, unsigned max_size=1000000);
  static BOOL  record (char *buffer, unsigned size);
  static void  close  (void);
};

class Demo_player
{
  static char *main_buffer;
  static char *cprs_buffer;
  static Demo_header header;
  static unsigned pointer;
  static unsigned block_pointer;
  static unsigned version_of_data;
  static Bitflag status;
  static BOOL read_disk_file (char *file_name);
  static BOOL read_volume_file (char *file_name);
public:
  static void  init   (unsigned data_version = 0);
  static void  quit   (void);
  static BOOL  open   (char *file_name, unsigned *players, 
                       char *user_data = NULL, unsigned *size = NULL);
  static BOOL  play   (char *buffer, unsigned *size);
  static void  close  (void);
};

#endif