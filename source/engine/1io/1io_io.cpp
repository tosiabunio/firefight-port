#include "1io_hdrs.h"
#include <filesystem>
#include <system_error>

// Port: the files are loose files named by the *.dir manifests, read and written with stdio.
// The original could also read packed .vol volumes (loaded whole into memory); the port data
// has none, so that path is gone. The manifests are merged in memory (was a temporary file).
#define CLOSED 0
#define READ   1
#define WRITE  2

FILE*         File::stream=NULL;
int           File::mode=CLOSED;
Text          File::source_directory;
unsigned      File::source_files;
File_Info     File::info;
int           File::access=0;

//-----------------------------------------------------------------------------
// clas xio
//-----------------------------------------------------------------------------
void Xio::init(void)
{
  try 
  {
    Text::init();
    Comm::quit_me(quit,"xio","log mmu");
  } 
  catch(Failure) 
  {
    FAILURE("xio init failed [Xio::init]");
  }
}
//-----------------------------------------------------------------------------
void Xio::quit(void)
{
  Text::quit();
}
//-----------------------------------------------------------------------------
// class File
//-----------------------------------------------------------------------------
void File::area(char const* name)
{
  DBG_CHECK(access);
  source_directory.area((char*)name);
}
//-----------------------------------------------------------------------------
void File::endarea(void)
{
  DBG_CHECK(access);
  source_directory.endarea();
}
//-----------------------------------------------------------------------------
int File::parse_label(int label_num)
{
  char* label_txt=source_directory.group[label_num].label;
  return parse_label(label_txt);
}
//-----------------------------------------------------------------------------
int File::parse_label(char const* label)
{
  memset(&info,0,sizeof(File_Info));
  strcpy(info.long_name,label);
  if(source_directory.exist((char*)label)) 
  {
    char* real_name=source_directory.string((char*)label);
    strcpy(info.long_name,label);
    strupr(info.long_name);
    char at=*real_name;
    if((at==LINK)||(at==EXCLUDE)) 
    {
      info.attrib=at;
      real_name++;
    } 
    else 
    {
      info.attrib=NONE;
    }
    sscanf(real_name,"%260s %32s",info.real_name,info.flags);
    // Manifests hold mixed-case DOS paths; the data on disk is lowercase (was strupr).
    strlwr(info.real_name);
    for(char* c=info.real_name; *c; c++)
      if(*c=='\\') *c='/';
    strupr(info.long_name);
    return 0;
  }
  return 1;
}
//-----------------------------------------------------------------------------
// Size and modification time (seconds, truncated to 32 bits as before) of info.real_name.
static int stat_real_name(unsigned &size, unsigned &date)
{
  struct stat st;
  if(stat(File::info.real_name,&st)!=0)
    return 0;
  size=(unsigned)st.st_size;
  date=(unsigned)st.st_mtime;
  return 1;
}
//-----------------------------------------------------------------------------
void File::open(char const* long_name)
{
  DBG_CHECK(access);
  if(mode!=CLOSED) 
    FAILURE("another file is already open [File::open(%s)]",long_name);
  if(parse_label(long_name)) 
    FAILURE2(XIO_ERROR,"file not found in directory [File::open(%s)]",source_directory.missing_label);
  stream=fopen(info.real_name,"rb");
  if(stream==NULL)
    FAILURE2(XIO_ERROR,"file open failed [File::open(%s [%s])]",info.long_name,info.real_name);
  stat_real_name(info.size,info.date);
  mode=READ;
}
//-----------------------------------------------------------------------------
void File::create(char const* long_name)
{
  DBG_CHECK(access);
  if(mode!=CLOSED) 
    FAILURE("another file is already open [File::create(%s)]",long_name);
  if(parse_label(long_name))
    FAILURE2(XIO_ERROR,"file not found in directory [File::create(%s)]",source_directory.missing_label);
  // The sprite cache directories are not in the repository; create them on demand.
  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::path(info.real_name).parent_path(),ec);
  stream=fopen(info.real_name,"wb");
  if(stream==NULL)
    FAILURE2(XIO_ERROR,"file create failed [File::create(%s [%s])]",info.long_name,info.real_name);
  mode=WRITE;
}
//-----------------------------------------------------------------------------
void File::close(void)
{
  if(stream!=NULL)
  {
    fclose(stream);
    stream=NULL;
  }
  mode=CLOSED;
  Comm::process_messages();
}
//-----------------------------------------------------------------------------
void File::read(char* buffer, unsigned size)
{
  DBG_CHECK(access);
  switch(mode) 
  {
    case READ:
      if(fread(buffer,1,size,stream)<size) 
      {
        if(ferror(stream))
          FAILURE2(XIO_ERROR,"read error [File::read]");
        FAILURE2(XIO_ERROR,"reading past EOF [File::read]");
      }
      break;
    case WRITE:
      FAILURE("file open for writing [File::read]");
    default:
      FAILURE("file not open [File::read]");
  }
}
//-----------------------------------------------------------------------------
void File::ignore(unsigned size)
{
  DBG_CHECK(access);
  switch(mode) 
  {
    case READ:
      if(fseek(stream,(long)size,SEEK_CUR)!=0) 
        FAILURE("seek error [File::ignore]");
      break;
    case WRITE:
      FAILURE("file open for writing [File::ignore]");
    default:
      FAILURE("file not open [File::ignore]");
  }
}
//-----------------------------------------------------------------------------
void File::write(char* buffer, unsigned size)
{
  DBG_CHECK(access);
  switch(mode) 
  {
    case READ:
      FAILURE("file open for reading [File::write]");
    case WRITE:
      if(fwrite(buffer,1,size,stream)<size) 
        FAILURE2(XIO_ERROR,"write error [File::write]");
      break;
    default:
      FAILURE("file not open [File::write]");
  }
}
//-----------------------------------------------------------------------------
unsigned File::size(void)
{
  DBG_CHECK(access);
  switch(mode) 
  {
    case READ:
      return info.size;
      break;
    case WRITE:
      FAILURE("file open for writing [File::size]");
    default:
      FAILURE("file not open [File::size]");
  }
  return 0;
}
//-----------------------------------------------------------------------------
unsigned File::size(char const* long_name)
{
  DBG_CHECK(access);
  get_info(long_name);
  return info.size;
}
//-----------------------------------------------------------------------------
unsigned File::date(char const* long_name)
{
  DBG_CHECK(access);
  get_info(long_name);
  return info.date;
}
//-----------------------------------------------------------------------------
int File::specified(char const* long_name)
{
  return source_directory.exist((char*)long_name);
}
//-----------------------------------------------------------------------------
void File::get_info(char const* long_name)
{
  DBG_CHECK(access);
  if(parse_label(long_name))
    FAILURE2(XIO_ERROR,"file not found in directory [File::get_info(%s)]",source_directory.missing_label);
  if(!stat_real_name(info.size,info.date))
    FAILURE2(XIO_ERROR,"file info retrieve failed [File::get_info(%s [%s])]",info.long_name,info.real_name);
}
//-----------------------------------------------------------------------------
void File::get_flags(char const* long_name)
{
  if(parse_label(long_name))
    FAILURE2(XIO_ERROR,"file not found in directory [File::get_flags(%s)]",source_directory.missing_label);
}
//-----------------------------------------------------------------------------
unsigned File::exists(char const* long_name)
{
  DBG_CHECK(access);
  if(parse_label(long_name)) return 0;
  unsigned size,date;
  if(!stat_real_name(size,date)) return 0;
  return size;
}
//-----------------------------------------------------------------------------
int File::volume(void)
{
  DBG_CHECK(access);
  return 0;  // there are no volumes any more
}
//-----------------------------------------------------------------------------
// Reads the manifests <name>.dir for each name in the list (separated by ',' or ';') and merges
// them into one directory. patch_path and mode only applied to volumes.
void File::access_on(char* volumenames,char* patch_path,int mode,Progress progress)
{
  (void)patch_path;
  (void)mode;
  (void)progress;
  DBG_CHECK(!access);
  static const char eol[]="\r\n";
  char vol_names[1024];
  DBG_CHECK(strlen(volumenames)<1023);
  char filename[260];
  char* merged=NULL;
  try
  {
    source_files=0;
    unsigned merged_len=0;
    strcpy(vol_names,volumenames);
    for(char* token=strtok(vol_names,";,"); token!=NULL; token=strtok(NULL,";,")) 
    {
      DBG_CHECK(strlen(token)<259);
      sprintf(filename,"%s.dir",token);
      strlwr(filename);  // the manifests on disk are lowercase
      MESSAGE("reading directory: %s",filename);
      // Each manifest must parse on its own, as in the original.
      Text test;
      test.dir_load(filename);
      test.find_unused_labels=0;
      test.free();
      FILE* src=fopen(filename,"rb");
      if(src==NULL) 
        FAILURE2(XIO_ERROR,"directory file open failed: %s [File::access_on]",filename);
      fseek(src,0,SEEK_END);
      unsigned size=(unsigned)ftell(src);
      fseek(src,0,SEEK_SET);
      char* grown=(char*)Heap::alloc(merged_len+size+2+1,"[cio] directory merge buffer");
      if(merged!=NULL)
      {
        memcpy(grown,merged,merged_len);
        Heap::free(merged,FILE_LINE);
      }
      merged=grown;
      size_t got=fread(merged+merged_len,1,size,src);
      fclose(src);
      if(got!=size)
        FAILURE2(XIO_ERROR,"directory file read failed: %s [File::access_on]",filename);
      merged_len+=size;
      memcpy(merged+merged_len,eol,2);
      merged_len+=2;
      Comm::process_messages();
    }
    if(merged==NULL)
      FAILURE2(XIO_ERROR,"no directory files given [File::access_on]");
    merged[merged_len]=0;
    source_directory.process(merged,merged_len,"directory");
    source_directory.find_unused_labels=0;
    Heap::free(merged,FILE_LINE);
    merged=NULL;
    Comm::process_messages();
    source_files=source_directory.groups_num;
    MESSAGE("%d file(s) in source directory",source_files);
  } 
  catch(Failure) 
  {
    if(merged!=NULL)
      Heap::free(merged,FILE_LINE);
    FAILURE("directories read failed [File::access_on]");
  }
  access=1;
}
//-----------------------------------------------------------------------------
void File::access_off(int dump_unused_files)
{
  (void)dump_unused_files;  // listed unused volume files only
  DBG_CHECK(access);
  source_directory.free();
  access=0;
}
//-----------------------------------------------------------------------------
#define MAX_BLOCK 0x4000

// 4096 pointers for __compress, plus room to align them (was 4*4096+3 for 32-bit pointers).
static unsigned char hash_table[sizeof(unsigned char*)*4096+sizeof(unsigned char*)-1];

unsigned Xio::compress(void* destination,void* source,unsigned source_len)
{
  unsigned dst_len;
  DBG_CHECK(source_len<=MAX_BLOCK);
  __compress(hash_table,(unsigned char*)source,source_len,(unsigned char*)destination,&dst_len);
  return dst_len;
}
//-----------------------------------------------------------------------------
unsigned Xio::decompress(void* destination,void* source,unsigned source_len)
{
  unsigned dst_len;
  __decompress(hash_table,(unsigned char*)source,source_len,(unsigned char*)destination,&dst_len);
  return dst_len;
}
//-----------------------------------------------------------------------------
