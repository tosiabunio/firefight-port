#include "1lg_hdrs.h"
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

static const info_size=10000;
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

  int cwd=GetEnvironmentVariable("cwdiags",path,MAX_PATH+MAX_COMPUTERNAME_LENGTH);
  if(strcmpi(path,"extended")==0)
    Comm::production=0;

  sprintf(name,"%s",_name);
  strupr(name);

  int fh=_creat(name,_S_IWRITE);
  if(fh!=-1) _close( fh );

  write_string("Exec info:       command '");
  write_string(GetCommandLine());

  if ( !GetCurrentDirectory( MAX_PATH, path )) strcpy( path, unknown);
  write_string("' working in '");
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

  write_string("System info:     ");

  unsigned long i=MAX_COMPUTERNAME_LENGTH;
  if ( !GetUserName( path, &i )) strcpy( path, unknown);
  write_string("user ");
  write_string(path);
  write_string(", ");
  
  MEMORYSTATUS memStat;
  GlobalMemoryStatus( &memStat );

  DWORD dwResult = GetVersion(); 
  sprintf(path,"Windows version %d.%d, ",LOBYTE(LOWORD(dwResult)),HIBYTE(LOWORD(dwResult)));
  write_string(path);

  int pmem=memStat.dwTotalPhys/(1024*1024/10);
  sprintf(path,"physical memory: %d.%dMB\n",pmem/10,pmem%10);
  write_string(path);

  write_string("DirectX info:    ");
  write_string("dplay ");
  if(Comm::check_directx(Comm::dplay_ok)) 
    write_string("ok");
  else
    write_string("failed");

  write_string(", ddraw ");
  if(Comm::check_directx(Comm::ddraw_ok)) 
    write_string("ok");
  else
    write_string("failed");

  write_string(", dsound ");
  if(Comm::check_directx(Comm::dsound_ok)) 
    write_string("ok");
  else
    write_string("failed");

  write_string(", status ");
  if(Comm::check_directx()) 
    write_string("ok");
  else
    write_string("failed");
  write_string("\n");

  write_string("\n---log begin---- ");

  time_t t=time(NULL);
  write_string(ctime(&t));
  write_string("\n",1);

  Comm::quit_me(quit,"*log","");
}

static const char *argv[100];

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
    if((!Comm::spawn_name)||(!Comm::production))
    {
      char s[1024]; 
      sprintf(s,"\nSee %s for details.",name);
      add_to_info(msginfo,s);
      HWND hwnd=GetWindowLong(Comm::hwnd,GWL_ID) ?Comm::hwnd :NULL;
      MessageBox(NULL,msginfo,"chaos works engine",MB_ICONINFORMATION|MB_OK|MB_TASKMODAL);
    }
  }

  if(Comm::spawn_name)
  {
    if((critical_error_occurred)&&(Comm::exit_code==0))
      Comm::exit_code=-1;

    char error_param[100];
    sprintf(error_param,"error=%d",Comm::exit_code);

    argv[0]=Comm::spawn_name;
    for(int i=0; Comm::spawn_argv[i]; i++)
      argv[i+1]=Comm::spawn_argv[i];
    argv[i+1]=error_param;
    argv[i+2]=NULL;

    write_string("Executing '");
    for(i=0; argv[i]; i++)
    {
      write_string((char*)argv[i]);
      if(argv[i+1])
        write_string(" ");
      else
        write_string("'\n");
    }

    int res = _execvp(argv[0], argv);
    write_string("<<< EXECUTION FAILED >>>\n",1);
  }
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
  int fh=_open(name,_O_APPEND|_O_WRONLY|_O_TEXT);
  if(fh!=-1)
  {
    OutputDebugString(c);
    _write(fh,c,strlen(c));
    if ((flush)&&(!Comm::production)) 
      _commit(fh);
    _close(fh);
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
