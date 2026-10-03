#include "1io_hdrs.h"
#include <filesystem>
#include <system_error>

#define CLOSED 0
#define READ   1
#define WRITE  2
#define VOLUME 3

int           File::handle;
int           File::mode=CLOSED;
Text          File::source_directory;
File_Info*    File::volume_directory;
unsigned      File::source_files;
unsigned      File::volume_files;
char          File::areaid[Text::max_area+1];
int           File::arealen;
File_Info     File::info;
int           File::access=0;
Volume_Header File::header;
void*         File::volume_memory[MAX_VOLUMES];
File_Info*    File::entry;
char*         File::read_ptr;
unsigned      File::to_read;

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
  if(source_directory.ok) 
  {
    source_directory.area((char*)name);
  }
  else 
  {
    if(name==0) 
    {
      areaid[0]=0;
      arealen=0;
    } 
    else 
    {
      if(name[0]==Text::area_char) 
      {
        name++;
        arealen=0;
      }
      int l=strlen(name);
      memcpy(areaid+arealen,name,l);
      arealen+=l;
      areaid[(arealen++)]=Text::area_char;
      areaid[arealen]=0;
    }
  }
}
//-----------------------------------------------------------------------------
void File::endarea(void)
{
  DBG_CHECK(access);
  if(source_directory.ok) 
  {
    source_directory.endarea();
  } 
  else 
  {
    if (arealen>0)
    {
      arealen--;
      while ((areaid[arealen-1]!=Text::area_char)&&arealen)
        arealen--;
      areaid[arealen]=0;
    }
  }
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
void File::open(char const* long_name)
{
  DBG_CHECK(access);
  if(mode!=CLOSED) 
    FAILURE("another file is already open [File::open(%s)]",long_name);
  if(source_directory.ok) 
  {
    if(parse_label(long_name)) 
      FAILURE2(XIO_ERROR,"file not found in directory [File::open(%s)]",source_directory.missing_label);
    handle=_open(info.real_name,_O_BINARY|_O_RDONLY);
    if(handle==-1)
      FAILURE2(XIO_ERROR,"file open failed [File::open(%s [%s])]",info.long_name,info.real_name);
    _finddata_t fileinfo;
    intptr_t find=_findfirst(info.real_name,&fileinfo);
    _findclose(find);
    info.date=fileinfo.time_write;
    info.size=fileinfo.size;
    mode=READ;
  } 
  else 
  {
    entry=find_file(long_name);
    if(entry==NULL) 
      FAILURE2(XIO_ERROR,"file not found in volumes [File::open(%s)]",info.long_name);
    info=*entry;
    if(info.offset==0) 
      FAILURE("file removed from memory [File::open(%s)]",info.long_name);
    read_ptr=(char*)info.offset;
    to_read=info.size;
    mode=VOLUME;
  }
}
//-----------------------------------------------------------------------------
void File::create(char const* long_name)
{
  DBG_CHECK(access);
  if(mode!=CLOSED) 
    FAILURE("another file is already open [File::create(%s)]",long_name);
  if(source_directory.ok) 
  {
    if(parse_label(long_name))
      FAILURE2(XIO_ERROR,"file not found in directory [File::create(%s)]",source_directory.missing_label);
    // The sprite cache directories are not in the repository; create them on demand.
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(info.real_name).parent_path(),ec);
    handle=_open(info.real_name,_O_BINARY|_O_WRONLY|_O_CREAT|_O_TRUNC,_S_IWRITE);
    if(handle==-1)
      FAILURE2(XIO_ERROR,"file create failed [File::create(%s [%s])]",info.long_name,info.real_name);
    mode=WRITE;
  } 
  else 
  {
    FAILURE("writing to volume is not supported [File::create(%s)]",long_name);
  }
}
//-----------------------------------------------------------------------------
void File::close(void)
{
  BOOL virtual_free_result;
  switch(mode) 
  {
    case READ:
    case WRITE:
      _close(handle); break;
    case VOLUME:
      virtual_free_result=VirtualFree((void*)entry->offset,entry->size,MEM_DECOMMIT);
      DBG_CHECK(virtual_free_result);
      entry->offset=0;
      break;
    default:
      break;
  }
  mode=CLOSED;
  Comm::process_messages();
}
//-----------------------------------------------------------------------------
void File::read(char* buffer, unsigned size)
{
  DBG_CHECK(access);
  int result;
  switch(mode) 
  {
    case READ:
      result=_read(handle,(void*)buffer,size);
      if(result==-1) 
        FAILURE2(XIO_ERROR,"read error [File::read]");
      if((unsigned)result<size) 
        FAILURE2(XIO_ERROR,"reading past EOF [File::read]");
      break;
    case WRITE:
      FAILURE("file open for writing [File::read]");
    case VOLUME:
      if(size>to_read) 
        FAILURE2(XIO_ERROR,"reading past EOF [File::read]");
      memcpy(buffer,read_ptr,size);
      read_ptr+=size;
      to_read-=size;
      break;
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
      if(_lseek(handle,size,SEEK_CUR)==-1) 
        FAILURE("seek error [File::ignore]");
      break;
    case WRITE:
      FAILURE("file open for writing [File::ignore]");
    case VOLUME:
      if(size>to_read) 
        FAILURE2(XIO_ERROR,"ignoring past EOF [File::ignore]");
      read_ptr+=size;
      to_read-=size;
      break;
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
      if(_write(handle,(void*)buffer,size)==-1) 
        FAILURE2(XIO_ERROR,"write error [File::write]");
      break;
    case VOLUME:
      FAILURE("writing to volume is not supported [File::write]");
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
    case VOLUME:
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
  if(source_directory.ok) 
    return source_directory.exist((char*)long_name);
  else
    return find_file(long_name)!=NULL;
}
//-----------------------------------------------------------------------------
void File::get_info(char const* long_name)
{
  DBG_CHECK(access);
  if(source_directory.ok) 
  {
    if(parse_label(long_name))
      FAILURE2(XIO_ERROR,"file not found in directory [File::get_info(%s)]",source_directory.missing_label);
#if 1
    _finddata_t fileinfo;
    intptr_t find=_findfirst(info.real_name,&fileinfo);
    _findclose(find);
    if(find==-1)
      FAILURE2(XIO_ERROR,"file info retrieve failed [File::get_info(%s [%s])]",info.long_name,info.real_name);
    info.date=fileinfo.time_write;
    info.size=fileinfo.size;
#else
    SECURITY_ATTRIBUTES sa;
    sa.nLength=sizeof(sa);
    sa.lpSecurityDescriptor=NULL;
    sa.bInheritHandle=0;
    HANDLE handle=CreateFile(info.real_name,GENERIC_READ,FILE_SHARE_READ,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(handle==INVALID_HANDLE_VALUE)
      FAILURE2(XIO_ERROR,"file info retrieve failed [File::get_info(%s [%s])] (LE=%d)",info.long_name,info.real_name,GetLastError());
    FILETIME cd,la,lw;
    if(!GetFileTime(handle,&cd,&la,&lw))
      FAILURE2(XIO_ERROR,"file info retrieve failed [File::get_info(%s [%s])] (LE=%d)",info.long_name,info.real_name,GetLastError());
    CloseHandle(handle);
    info.date
#endif
  } 
  else 
  {
    entry=find_file(long_name);
    if(entry==NULL) 
      FAILURE2(XIO_ERROR,"file info retrieve failed [File::get_info(%s)]",info.long_name);
    info=*entry;
  }
}
//-----------------------------------------------------------------------------
void File::get_flags(char const* long_name)
{
  if(source_directory.ok) 
  {
    if(parse_label(long_name))
      FAILURE2(XIO_ERROR,"file not found in directory [File::get_flags(%s)]",source_directory.missing_label);
  } 
  else 
  {
    entry=find_file(long_name);
    if(entry==NULL) FAILURE2(XIO_ERROR,"file not found in volume [File::get_flags(%s)]",info.long_name);
    info=*entry;
  }
}
//-----------------------------------------------------------------------------
unsigned File::exists(char const* long_name)
{
  DBG_CHECK(access);
  if(source_directory.ok) 
  {
    if(parse_label(long_name)) return 0;
    _finddata_t fileinfo;
    intptr_t find=_findfirst(info.real_name,&fileinfo);
    _findclose(find);
    if(find==-1) return 0;
    return fileinfo.size;
  } 
  else 
  {
    return find_file(long_name)!=NULL;
  }
}
//-----------------------------------------------------------------------------
int File::volume(void)
{
  DBG_CHECK(access);
  return !source_directory.ok;
}
//-----------------------------------------------------------------------------
File_Info* File::find_file(char const* long_name)
{
  if (*long_name==Text::area_char) 
  {
    strcpy(info.long_name,long_name+1);
  } 
  else 
  {
    strcpy(info.long_name,long_name);
    if(strchr(info.long_name,Text::area_char)==NULL)  
    {
      strcpy(info.long_name,areaid);
      strcat(info.long_name,long_name);
    }
  }
  File_Info* walker=volume_directory;
  while(walker!=NULL) 
  {
    if(strcmpi(info.long_name,walker->long_name)==0) 
      break;
    walker=walker->next;
  }
  return walker;
}
//-----------------------------------------------------------------------------
void File::free_volume_memory(void)
{
  BOOL virtual_free_result;
  for(int i=0;i<MAX_VOLUMES;i++) 
  {
    if(volume_memory[i]!=NULL) 
    {
      virtual_free_result=VirtualFree(volume_memory[i],0,MEM_RELEASE);
      DBG_CHECK(virtual_free_result);
    }
  }
}
//-----------------------------------------------------------------------------
void File::link_entries(File_Info* first,unsigned files,uintptr_t base)
{
  File_Info* next=NULL;
  for(int i=(int)files-1;i>=0;i--) 
  {
    first[i].next=next;
    first[i].offset+=base;
    next=&first[i];
  }
}
//-----------------------------------------------------------------------------
static char access_tmp[260];
static char tmp_path[260];

void File::access_on(char* volumenames,char* patch_path,int mode,Progress progress)
{
  int pass;
  DBG_CHECK(!access);
  char eol[]="\r\n";
  char vol_names[1024];
  DBG_CHECK(strlen(volumenames)<1023);
  char filename[260];
  _finddata_t fileinfo;
  intptr_t find=_findfirst("*.dir",&fileinfo);
  _findclose(find);
  if(find==-1) // there are no "*.dir" files in current directory, load volumes not directories
  {
    try 
    {
      unsigned total_volumes_size=0;
      memset(volume_memory,0,sizeof(volume_memory));
      int volumes=0;
      volume_files=0;
      File_Info* last=NULL;
      MESSAGE("checking volume header(s)");
      strcpy(vol_names,volumenames);
      // process all volumes
      char* token=strtok(vol_names,";,");
      while(token!=NULL) 
      {
        DBG_CHECK(strlen(token)<259);
        if(volumes==MAX_VOLUMES) 
          FAILURE("maximum number (%d) of supported volumes exceeded [File::access_on]",MAX_VOLUMES);
        if(mode&ACCESS_LORES) 
          pass=1;
        else 
          pass=2;
header_again:
        int volume=-1;
        if(patch_path!=NULL)
        {
          if(pass==1) 
            sprintf(filename,"%s\\%s.vol",patch_path,token);
          else 
            sprintf(filename,"%s\\%sh.vol",patch_path,token);
          volume=_open(filename,_O_BINARY|_O_RDONLY);
        }
        if(volume==-1) 
        {
          if(pass==1) 
            sprintf(filename,"%s.vol",token);
          else 
            sprintf(filename,"%sh.vol",token);
          volume=_open(filename,_O_BINARY|_O_RDONLY);
        }
//        if(volume==-1) FAILURE2(XIO_ERROR,"volume file open failed: %s [File::access_on]",filename);
        if(volume==-1) 
          goto header_cont;
        // read volume header
        if(_lseek(volume,-1*(signed)sizeof(header),SEEK_END)==-1)
          FAILURE2(XIO_ERROR,"error seeking to header of volume: %s [File::access_on]",filename);
        if(_read(volume,&header,sizeof(header))<sizeof(header))
          FAILURE2(XIO_ERROR,"error reading header of volume: %s [File::access_on]",filename);
        if((header.id!=COMPRESSED_VOLUME_ID)&&(header.id!=VOLUME_ID))
          FAILURE2(XIO_ERROR,"bad header in volume: %s [File::access_on]",filename);
        // calculate total size of all volumes for progress calculation purposes
        total_volumes_size+=header.total_size;
        _close(volume);
header_cont:
        pass++;
        if((pass==2)&&(mode&ACCESS_HIRES)) goto header_again;
        token=strtok(NULL,";,");
      }
      DBG_CHECK((total_volumes_size%0x4000)==0);
      unsigned progress_count=0;
      unsigned progress_prev=0;
      strcpy(vol_names,volumenames);
      token=strtok(vol_names,";,");
      while(token!=NULL) 
      {
        DBG_CHECK(strlen(token)<259);
        int pass;
        if(mode&ACCESS_LORES) pass=1;
        else pass=2;
volume_again:
        int volume=-1;
        if(patch_path!=NULL)
        {
          if(pass==1) 
            sprintf(filename,"%s\\%s.vol",patch_path,token);
          else 
            sprintf(filename,"%s\\%sh.vol",patch_path,token);
          volume=_open(filename,_O_BINARY|_O_RDONLY);
        }
        if(volume==-1) 
        {
          if(pass==1) 
            sprintf(filename,"%s.vol",token);
          else 
            sprintf(filename,"%sh.vol",token);
          volume=_open(filename,_O_BINARY|_O_RDONLY);
        }
//        if(volume==-1) FAILURE2(XIO_ERROR,"volume file open failed: %s [File::access_on]",filename);
        if(volume==-1) goto volume_cont;
        MESSAGE("processing volume: %s",filename);
        // read volume header
        if(_lseek(volume,-1*(signed)sizeof(header),SEEK_END)==-1)
          FAILURE2(XIO_ERROR,"error seeking to header of volume: %s [File::access_on]",filename);
        if(_read(volume,&header,sizeof(header))<sizeof(header))
          FAILURE2(XIO_ERROR,"error reading header of volume: %s [File::access_on]",filename);
        if(header.total_size==0) 
        {
          MESSAGE("volume is empty");
        } 
        else 
        {
          MESSAGE("allocating %dkB...",kilo(header.total_size));
          // allocate virtual memory for volume (decompressed size)
          if((volume_memory[volumes]=VirtualAlloc(NULL,header.total_size,MEM_COMMIT,PAGE_READWRITE))==NULL)
            FAILURE2(XIO_OUT_OF_MEMORY,"memory allocate failed for volume: %s [File::access_on]",filename);
          Comm::process_messages();
          if(_lseek(volume,0,SEEK_SET)==-1) FAILURE2(XIO_ERROR,"error reading volume: %s [File::access_on]",filename);
          if(header.id==VOLUME_ID) 
          {
            FAILURE("uncompressed volumes no longer supported");
          } 
          else if(header.id==COMPRESSED_VOLUME_ID) 
          {
            // read compressed volume
            MESSAGE("reading and decompressing %dkB...",kilo(header.file_size));
            unsigned to_read=header.file_size;
            unsigned char* dst_ptr=(unsigned char*)volume_memory[volumes];
            unsigned char buffer[0x4000+0x1000];
            unsigned short block;
            unsigned new_size;
            while(to_read>0) {
              // read compressed block header
              if(_read(volume,&block,sizeof(block))<sizeof(block))
                FAILURE2(XIO_ERROR,"error reading volume: %s [File::access_on]",filename);
              to_read-=sizeof(block);
              if(block==0) // block is not compressed, read it straight (doesn't work so there is no such blocks in volume)
                FAILURE("not compressed blocks not supported [File::access_on]");
              // block is compressed, read it and decompress
              if(_read(volume,buffer,block)<block) 
                FAILURE2(XIO_ERROR,"error reading volume: %s [File::access_on]",filename);
              to_read-=block;
              new_size=Xio::decompress(dst_ptr,buffer,block);
              // the size of uncompressed block is always supposed to be exactly 0x4000
              DBG_CHECK(new_size==0x4000);
              progress_count+=new_size;
              dst_ptr+=new_size;
              // calculate progress values and call progress routine
              unsigned percent=(progress_count*100)/total_volumes_size;
              DBG_CHECK(percent<=100);
              if(percent!=progress_prev)
              {
                 progress_prev=percent;
                 if(progress!=NULL) progress(percent);
              }
              Comm::process_messages();
            }
          }
          File_Info* new_directory=(File_Info*)((char*)volume_memory[volumes]+header.directory_offset);
          if(volumes==0) volume_directory=new_directory;
          else last->next=new_directory;
          // link recently read directory with previous one and link directory entries
          link_entries(new_directory,header.volume_files,(uintptr_t)volume_memory[volumes]);
          last=&new_directory[header.volume_files-1];
          volume_files+=header.volume_files;
          volumes++;
          MESSAGE("%s: %d file(s)",filename,header.volume_files);
        }
        _close(volume);
volume_cont:
        pass++;
        if((pass==2)&&(mode&ACCESS_HIRES)) goto volume_again;
        token=strtok(NULL,";,");
      }
      MESSAGE("%d volume(s) loaded, total %d file(s) in volume(s), %dkB used",volumes,volume_files,kilo(total_volumes_size));
    } 
    catch(Failure) 
    {
      free_volume_memory();
      FAILURE("volumes read failed [File::access_on]");
    }
  } 
  else 
  {
    // there are "*.dir" files in current directory, load directories
    try{
      source_files=0;
      GetTempPath(260,tmp_path);
      GetTempFileName(tmp_path,"VOL",0,access_tmp);
      int all=_open(access_tmp,_O_BINARY|_O_APPEND|_O_CREAT|_O_WRONLY,_S_IWRITE);
      if(all==-1) 
        FAILURE2(XIO_ERROR,"temporary directory file create failed: %s [File::access_on]",access_tmp);
      strcpy(vol_names,volumenames);
      char* token=strtok(vol_names,";,");
      while(token!=NULL) 
      {
        DBG_CHECK(strlen(token)<259);
        sprintf(filename,"%s.dir",token);
        strlwr(filename);  // the manifests on disk are lowercase
        MESSAGE("reading directory: %s",filename);
        int src=_open(filename,_O_BINARY|_O_RDONLY);
        if(src==-1) 
          FAILURE2(XIO_ERROR,"directory file open failed: %s [File::access_on]",filename);
        Text test;
        test.dir_load(src,filename);
        test.find_unused_labels=0;
        test.free();
        _close(src);
        src=_open(filename,_O_BINARY|_O_RDONLY);
        if(src==-1) 
          FAILURE2(XIO_ERROR,"directory file open failed: %s [File::access_on]",filename);
        unsigned size=_filelength(src);
        void* buffer=Heap::alloc(size,"[cio] directory merge buffer");
        _read(src,buffer,size);
        _close(src);
        _write(all,buffer,size);
        _write(all,&eol,2);
        Heap::free(buffer,FILE_LINE);
        token=strtok(NULL,";,");
        Comm::process_messages();
      }
      _close(all);
      all=_open(access_tmp,_O_BINARY|_O_RDONLY);
      if(all==-1) 
        FAILURE2(XIO_ERROR,"temporary directory file open failed: %s [File::access_on]",access_tmp);
      source_directory.dir_load(all,access_tmp);
      source_directory.find_unused_labels=0;
      _close(all);
      Comm::process_messages();
      remove(access_tmp);
      source_files=source_directory.groups_num;
      MESSAGE("%d file(s) in source directory",source_files);
    } 
    catch(Failure) 
    {
      remove(access_tmp);
      FAILURE("directories read failed [File::access_on]");
    }
  }
  access=1;
}
//-----------------------------------------------------------------------------
void File::access_off(int dump_unused_files)
{
  DBG_CHECK(access);
  if((dump_unused_files)&&(!source_directory.ok)) 
  {
    int uf=0;
    File_Info* walker=volume_directory;
    while(walker!=NULL) 
    {
      if(walker->offset!=0) 
      {
        if(uf==0) 
          MESSAGE("unused files found:");
        MESSAGE("%s [%s]",walker->long_name,walker->real_name);
        uf++;
      }
      walker=walker->next;
    }
    if(uf>0) 
      MESSAGE("%d unused files found",uf);
  }
  source_directory.free();
  free_volume_memory();
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
