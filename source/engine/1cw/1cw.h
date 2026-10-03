# ifndef _1CW_H_INCLUDED
# define _1CW_H_INCLUDED

# ifndef _1LG_H_INCLUDED
#   include <1lg.h>
# endif
# ifndef _1MM_H_INCLUDED
#   include <1mm.h>
# endif
# ifndef _1BA_H_INCLUDED
#   include <1ba.h>
# endif
# ifndef _1IO_H_INCLUDED
#   include <1io.h>
# endif
# ifndef _1SP_H_INCLUDED
#   include <1sp.h>
# endif
# ifndef _1SS_H_INCLUDED
#   include <1ss.h>
# endif
# ifndef _1EE_H_INCLUDED
#   include <1ee.h>
# endif
# ifndef _1RG_H_INCLUDED
#   include <1rg.h>
# endif

#ifndef EXCLUDE_LIBS
#ifdef _DEBUG
#pragma comment(lib,"1cwd.lib")
#else
#pragma comment(lib,"1cw.lib")
#endif
#endif

struct Cwe_error
{
  enum
  {
    general = 150,
    not_enough_memory,
    ini_corrupted,
  };
};

struct Cwe_param
{
  HINSTANCE hinstance;
  HWND      hwnd;
  char*     command_line;
  char*     class_name;
  char*     window_name;
  char*     ini_file;
  char*     log_file;
  HICON     icon_handle;
  Cwe_param() {memset(this, 0, sizeof(Cwe_param));};
};


class Cwe
{
private:
  static Text   text;
	static BOOL   application_ready;
  static void quit(void);
public:
	static void init (Cwe_param* param);
};

# endif


