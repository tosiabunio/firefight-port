#include "1lg_hdrs.h"
#include <SDL.h>
#pragma init_seg(lib)

char formated_message[1024];
char Log::name[_MAX_PATH] = "";
int  critical_error_occurred=0;

char  Log::info_file_line[_MAX_PATH+16];
char  Log::last_file_line[_MAX_PATH+16];

char *format_message (char *message, ...)
{
  va_format_message(message);
  return(formated_message);
}

static const int info_size=10000;
static char sysinfo[info_size+1];
static char enginfo[info_size+1];
static char msginfo[info_size+1];
static char failinfo[info_size+1];

static char m_overflow[]="\n<buffer overflow>\n";

static void add_to_info (char *buf, char *msg)
{
  int bsize=strlen(buf);
  int msize=strlen(msg);
  if((bsize+msize)>=info_size)
  {
    memcpy(&buf[bsize],msg,msize-(info_size-bsize));
    memcpy(&buf[info_size-1-strlen(m_overflow)],m_overflow,strlen(m_overflow));
  }
  else
  {
    memcpy(&buf[bsize],msg,msize);
  }
}


//=============================================================================
// Log
//=============================================================================

static char unknown[]="<unknown>";

void Log::init (char *_name)
{
  char path[MAX_PATH+MAX_COMPUTERNAME_LENGTH];

  const char *diags=SDL_getenv("cwdiags");
  if(diags&&(strcmpi((char*)diags,"extended")==0))
    Comm::production=0;

  sprintf(name,"%s",_name);  // was upper-cased, which breaks paths on case-sensitive systems

  FILE *log=fopen(name,"w");  // was _creat
  if(log) fclose(log);

  if ( !GetCurrentDirectory( MAX_PATH, path )) strcpy( path, unknown);
  write_string("Exec info:       working in '");
  write_string(path);
  write_string("'\n");
  strcpy(Comm::working_path,path);
  if(Comm::working_path[strlen(Comm::working_path)-1]=='\\')
    Comm::working_path[strlen(Comm::working_path)-1]=0;

  write_string("Program info:    ");
#if HI_DEBUG
  write_string("not optimized, ");
#else
  write_string("optimized, ");
#endif
  if(Comm::production)
    write_string("limited ");
  else
    write_string("extended ");

  write_string("diags, compiled ");
  write_string(Comm::date_stamp);
  write_string("\n");

  // Port: was user name, Windows version, memory and a DirectX probe.
  SDL_version linked;
  SDL_GetVersion(&linked);
  sprintf(path,"System info:     %s, physical memory: %dMB, SDL %d.%d.%d\n",
    SDL_GetPlatform(),SDL_GetSystemRAM(),linked.major,linked.minor,linked.patch);
  write_string(path);

  write_string("\n---log begin---- ");

  time_t t=time(NULL);
  write_string(ctime(&t));
  write_string("\n",1);

  Comm::quit_me(quit,"*log","");
}

void Log::quit (void)
{
  write_string("\n");

  if(sysinfo[0])
  {
    write_string("System information\n\n");
    write_string(sysinfo);
    write_string("\n",1);
  }

  if(enginfo[0])
  {
    write_string("Engine information\n\n");
    write_string(enginfo);
    write_string("\n",1);
  }
  
  write_string("----log end----- ");
  time_t t=time(NULL);
  write_string(ctime(&t));
  write_string("\n",1);

  write_string("Program status:  ");
  if(critical_error_occurred)
  {
    write_string("execution failed\n");
    write_string(failinfo,1);
  }
  else
    write_string("completed successfully\n",1);
  write_string("\n");

  if(critical_error_occurred)
  {
    char s[1024];
    sprintf(s,"\nSee %s for details.",name);
    add_to_info(msginfo,s);
    Comm::show_message_box(msginfo,0);
  }

  // Port: the original re-launched the launcher (LOADER.EXE) here.
}

char *Log::info (char *_file,int _line)
{
  char drive [_MAX_DRIVE];
  char dir   [_MAX_DIR];
  char fname [_MAX_FNAME];
  char ext   [_MAX_EXT];
  _splitpath( _file, drive, dir, fname, ext );

  sprintf(info_file_line,"%s(%d)",fname,_line);

  return(info_file_line);
}

void Log::write_string (char *c, int flush)
{
  (void)flush;  // the file is closed after every write, so it is always flushed
  FILE *log=fopen(name,"a");  // was _open in text mode
  if(log)
  {
    fputs(c,stderr);  // was OutputDebugString
    fputs(c,log);
    fclose(log);
  }
}

void Log::write_message (char *msg, ...)
{
  va_format_message(msg);
  format_message("%-14s   %s\n",info_file_line,formated_message);
  write_string(formated_message,0);
}

void Log::write_warning (char *msg, ...)
{
  va_format_message(msg);
  format_message("%-14s - %s\n",info_file_line,formated_message);
  write_string(formated_message,1);
  add_to_info(failinfo,formated_message);
  add_to_info(msginfo,formated_message+15);
}

void Log::write_failure (char *msg, ...)
{
  va_format_message(msg);
  format_message("%-14s * %s\n",info_file_line,formated_message);
  write_string(formated_message,1);
  add_to_info(failinfo,formated_message);
  add_to_info(msginfo,formated_message+17);
}

void Log::write_sysinfo (char *msg, ...)
{
  va_format_message(msg);
  format_message("%-14s   %s\n",info_file_line,formated_message);
  add_to_info(sysinfo,formated_message);
}

void Log::write_enginfo (char *msg, ...)
{
  va_format_message(msg);
  format_message("%-14s   %s\n",info_file_line,formated_message);
  add_to_info(enginfo,formated_message);
}
