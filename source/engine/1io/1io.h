#ifndef _1IO_HPP_INCLUDED
#define _1IO_HPP_INCLUDED

#ifndef EXCLUDE_LIBS
#ifdef _DEBUG
#pragma comment(lib,"1iod.lib")
#else
#pragma comment(lib,"1io.lib")
#endif
#endif

#define XIO_ERROR 250
#define XIO_OUT_OF_MEMORY 251

class Xio 
{
public:
  static void init(void);
  static unsigned compress(void* destination,void* source,unsigned source_len);
  static unsigned decompress(void* destination,void* source,unsigned source_len);
private:
  static void quit(void);
};

class Text: public Heap_object
{
public:

  enum { max_label = 24, max_area = 256 };

  int            ok;
  int            find_unused_labels;
  static int     find_unused_labels_disable;

                 Text();
                ~Text();
  static void    init (void);
  static void    quit (void);

  char          *sections;
  void           load (char *file_name);
  void           free (void);

  int            size      (char *label);
  int            exist     (char *label) {return(size(label));}
  void           check     (char *label) {string(label);}
  int            width     (char *label);
  char          *string    (char *label, int strnum=0);
  int            value     (char *label, int strnum=0);
  float          fp_value  (char *label, int strnum=0);

  int            size      (void) {return(size(last));};
  int            exist     (void) {return(exist(last));};
  void           check     (void) {check(last);};
  int            width     (void) {return(width(last));};
  char          *string    (int strnum=0) {return(string(last,strnum));};
  int            value     (int strnum=0) {return(value(last,strnum));};
  float          fp_value  (int strnum=0) {return(fp_value(last,strnum));};

  void           area (char *name=0);
  void           endarea (void);

  struct Group
  {
    char *label;
    int texts_num;
    int max_width;
    int used;
    char **texts;
  };

  Group *group;
  int groups_num;
  int strings_num;

  int used_memory;
  char *data_adr;

  char           areaid[max_area+1]; 
  int            arealen;             
  static char    area_char;  

  void           *(*mem_alloc) (unsigned int size, char const *name);
  void           (*mem_free)  (void *ptr, char const *name);

  // tylko dla xio!
  void           process (char* src,unsigned len,char* current_name);
  void           dir_load(int handle,char* name); 
  static char    missing_label[max_area+1];

private:

  friend class xio;
  friend class File;

  static char *last;

  char *name;
  int *hash_table;
  int hash_table_size;
  int last_result;

  int find_label (char *label);
  void add_group (int group_num);
  int compile_texts (char *dst, char *src);
  void display_unused (void);
};

// volume access mode

#define ACCESS_HIRES 1
#define ACCESS_LORES 2

// file attributes for internal use only
#define NONE '_'
#define LINK '^'
#define EXCLUDE '-'

// volume consts for internal use only
#define VOLUME_ID 0x42024202
#define COMPRESSED_VOLUME_ID 0x43024202
#define XIO_BLOCK (256*1024)

//max volume files supported in access_on
#define MAX_VOLUMES 50

typedef struct VH {
  unsigned total_size;
  unsigned file_size;
  unsigned volume_files;
  unsigned directory_offset;
  unsigned id;
} Volume_Header;

typedef struct FI{
  char        long_name[260];
  char        real_name[260];
  char        flags[32];
  unsigned    size;
  unsigned    date;
  uintptr_t   offset;  // a pointer once the volume is loaded (was unsigned)
  char        attrib;
  FI*         next;
} File_Info;

class File
{
public:
//types
  typedef void (*Progress)(int);
//methods
                          File() {}
                          ~File() {close();}
  static void             access_on(char* volume_names,char* patch_path=NULL,int mode=ACCESS_HIRES|ACCESS_LORES,Progress progress=NULL);
  static void             access_off(int dump_unused_files=0);
  static int              volume(void);
  static void             area(char const* area);
  static void             endarea(void);
  static void             close(void);
  static void             open(char const* long_name);
  static void             create(char const* long_name);
  static void             read(char* buffer, unsigned size);
  static inline void      read(unsigned char* buffer, unsigned size) {read((char*)buffer,size);}
  static inline void      read(signed char* buffer, unsigned size) {read((char*)buffer,size);}
  static inline void      read(void* buffer, unsigned size) {read((char*)buffer,size);}
  static inline void      read(int* buffer, unsigned size) {read((char*)buffer,size);}
  static inline void      read(unsigned* buffer, unsigned size) {read((char*)buffer,size);}
  static void             write(char* buffer, unsigned size);
  static inline void      write(unsigned char* buffer, unsigned size) {write((char*)buffer,size);}
  static inline void      write(signed char* buffer, unsigned size) {write((char*)buffer,size);}
  static inline void      write(void* buffer, unsigned size) {write((char*)buffer,size);}
  static inline void      write(int* buffer, unsigned size) {write((char*)buffer,size);}
  static inline void      write(unsigned* buffer, unsigned size) {write((char*)buffer,size);}
  static void             ignore(unsigned size);
  static void             get_info(char const* long_name);
  static void             get_flags(char const* long_name);
  static unsigned         size(void);
  static unsigned         size(char const* long_name);
  static int              specified(char const* long_name);
	static unsigned         date(char const* long_name);
  static unsigned         exists(char const* long_name);
  static inline unsigned  exist(char const* long_name) {return exists(long_name);}
//variables
  static File_Info        info;
private:                  
//methods
  static int              parse_label(int label_num);
  static int              parse_label(char const* label);
  static File_Info*       find_file(char const* long_name);
  static void             free_volume_memory(void);
  static void             link_entries(File_Info* first,unsigned files,uintptr_t base);
//variables
  static int              access;
  static Text             source_directory;
  static File_Info*       volume_directory;
  static File_Info*       entry;
  static unsigned         source_files;
  static unsigned         volume_files;
  static int              handle;
  static char             areaid[Text::max_area+1];
  static int              arealen;
  static int              mode;
  static Volume_Header    header;
  static void*            volume_memory[MAX_VOLUMES];
  static char*            read_ptr;
  static unsigned         to_read;
  friend class Xio;
  friend class Text;
  friend class Avm;
};                        
                          
#endif                    
                          
//TODO:                          
//  static void             open(int handle) {(void)handle;} 
//  static void             append(char const* long_name) {(void)long_name;} 
                          
                          
                          
                          
                          
                          
                          
                          