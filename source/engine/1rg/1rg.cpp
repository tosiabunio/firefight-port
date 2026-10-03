#include "1rg_hdrs.h"


//=============================================================================
// string constants
//=============================================================================
const char Registry::s_software_key[] = "Software";
const char Registry::s_company_key[]  = "chaos works";
const char Registry::s_version_key[]  = "version";

const char Cmd_line::s_def_yes[]      = "yes";
const char Cmd_line::s_def_no[]       = "no";


//=============================================================================
// Cmd_line static variables
//=============================================================================
char    Cmd_line::command_line [Cmd_line::line_len];
char    Cmd_line::return_buffer[Cmd_line::line_len];
Bitflag Cmd_line::status(0);

//=============================================================================
// Registry constants
//=============================================================================
const int reg_Initialized = 0x00000001;
//=============================================================================
// Cmd_line constants
//=============================================================================
const int cmd_Initialized = 0x00000001;
//=============================================================================
// Registry static variables
//=============================================================================
Bitflag Registry::status(0);
char    Registry::s_application_key[REG_APP_NAME_LEN];
char    Registry::s_application_sub_key[REG_APP_NAME_LEN];
char    Registry::buffer[REG_BUFFER_LEN];
HKEY    Registry::base_key = HKEY_CURRENT_USER;
//=============================================================================
// Registry methods
//=============================================================================
void Registry::quit (void)
{
  status.reset(reg_Initialized);
}
//-----------------------------------------------------------------------------
void Registry::init (char *application_name, char *application_sub_name, int mode, int version)
{
  DBG_CHECK(!status.is(reg_Initialized));
  DBG_CHECK(application_name!=NULL);
  DBG_CHECK(strlen(application_name)<(REG_APP_NAME_LEN-1));
  DBG_CHECK((application_sub_name==NULL)||(strlen(application_sub_name)<(REG_APP_NAME_LEN-1)));
#if HI_DEBUG
  for (unsigned i=0; i<strlen(application_name); i++)
    if ((application_name[i]<32)||(application_name[i]>127))
       FAILURE("Registry::init - invalid application name \"%s\"", application_name);
#endif  
  strcpy(s_application_key, application_name);
  if (application_sub_name!=NULL)
    strcpy(s_application_sub_key, application_sub_name);
  else
    s_application_sub_key[0] = 0; 
  if (mode == mode_User)
    base_key = HKEY_CURRENT_USER;
  else if (mode == mode_Machine)
    base_key = HKEY_LOCAL_MACHINE;
  else
    FAILURE("Registry::init - invalid mode");
  status.set(reg_Initialized);
  if (version!=0)
  {
    unsigned ver = get_int (NULL, (char *)s_version_key, 0xFFFFFFFF);
    if (ver < (unsigned)version)
      delete_application_key();
    set_int(NULL, (char*)s_version_key, version);
  }
  Comm::quit_me(Registry::quit, "registry", "log mmu");
}
//-----------------------------------------------------------------------------
void Registry::set_mode (int mode, int version)
{
  if (mode == mode_User)
    base_key = HKEY_CURRENT_USER;
  else if (mode == mode_Machine)
    base_key = HKEY_LOCAL_MACHINE;
  else
    FAILURE("Registry::set_mode - invalid mode");
  if (version!=0)
  {
    unsigned ver = get_int (NULL, (char *)s_version_key, 0);
    if (ver < (unsigned)version)
      delete_application_key();
    set_int(NULL, (char*)s_version_key, version);
  }
}
//-----------------------------------------------------------------------------
HKEY Registry::lock_key (char *section)
{
  DBG_CHECK(status.is(reg_Initialized));
  HKEY h_soft_key    = NULL;
  HKEY h_company_key = NULL;
  HKEY h_app_key     = NULL;
  HKEY h_sub_app_key = NULL;
  HKEY h_section_key = NULL;
  DWORD dw;
  LONG res;
	res = RegOpenKeyEx(base_key, s_software_key, 0, KEY_WRITE|KEY_READ, &h_soft_key);
  if (res != ERROR_SUCCESS) 
    return(NULL);
  res = RegCreateKeyEx(h_soft_key, s_company_key, 0, REG_NONE, REG_OPTION_NON_VOLATILE, 
                       KEY_WRITE|KEY_READ, NULL,	&h_company_key, &dw);
  RegCloseKey(h_soft_key);
  if (res != ERROR_SUCCESS) 
    return(NULL);
  res = RegCreateKeyEx(h_company_key, s_application_key, 0, REG_NONE, 	REG_OPTION_NON_VOLATILE, 
                         KEY_WRITE|KEY_READ, NULL, &h_app_key, &dw);
  RegCloseKey(h_company_key);
  if (res != ERROR_SUCCESS) 
    return(NULL);

  if (strlen(s_application_sub_key)>0)
  {
    res = RegCreateKeyEx(h_app_key, s_application_sub_key, 0, REG_NONE, 	REG_OPTION_NON_VOLATILE, 
                         KEY_WRITE|KEY_READ, NULL, &h_sub_app_key, &dw);
    RegCloseKey(h_app_key);
    if (res != ERROR_SUCCESS) 
      return(NULL);
    if (section)
    {
      res = RegCreateKeyEx(h_sub_app_key, section, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, 
                     NULL, &h_section_key, &dw);
      RegCloseKey(h_sub_app_key);
      if (res != ERROR_SUCCESS) 
        return(NULL);
      else
        return(h_section_key);
    }
    else
      return(h_sub_app_key);
  }
  else
  {
    if (section)
    {
      res = RegCreateKeyEx(h_app_key, section, 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, 
                           NULL, &h_section_key, &dw);
      RegCloseKey(h_app_key);
      if (res != ERROR_SUCCESS) 
        return(NULL);
      else
        return(h_section_key);
    }
    else
      return (h_app_key);
  }
}
//-----------------------------------------------------------------------------
void Registry::unlock_key(HKEY hkey)
{
  DBG_CHECK(status.is(reg_Initialized));
  DBG_CHECK(hkey!=NULL);
  RegCloseKey(hkey);
}
//-----------------------------------------------------------------------------
unsigned Registry::get_int    (char *section, char *entry,	unsigned def)
{
  DBG_CHECK(status.is(reg_Initialized));
	DBG_CHECK(entry!=NULL);
	HKEY h_sec_key = lock_key(section);
	if (!h_sec_key)
		return def;
	DWORD dw_value;
	DWORD dw_type;
	DWORD dw_count = sizeof(DWORD);
	LONG res = RegQueryValueEx(h_sec_key, entry, NULL, &dw_type, (LPBYTE)&dw_value, &dw_count);
	unlock_key(h_sec_key);
	if (res == ERROR_SUCCESS)
	{
		DBG_CHECK(dw_type == REG_DWORD);
		DBG_CHECK(dw_count == sizeof(dw_value));
		return (UINT)dw_value;
	}
	return def;
}
//-----------------------------------------------------------------------------
const char *Registry::get_string (char *section, char *entry,	char *def)
{
  DBG_CHECK(status.is(reg_Initialized));
	DBG_CHECK(entry!=NULL);
	HKEY h_sec_key = lock_key(section);
	if (!h_sec_key)
		return def;
	DWORD dw_type;
  DWORD dw_count;
	LONG res = RegQueryValueEx(h_sec_key, entry, NULL, &dw_type, NULL, &dw_count);
	if (res == ERROR_SUCCESS)
	{
		DBG_CHECK(dw_type == REG_SZ);
		res = RegQueryValueEx(h_sec_key, entry, NULL, &dw_type,	(LPBYTE)buffer, &dw_count);
	}
	unlock_key(h_sec_key);
	if (res == ERROR_SUCCESS)
	{
		DBG_CHECK(dw_type == REG_SZ);
		return buffer;
	}
  return def;
}
//-----------------------------------------------------------------------------
BOOL Registry::get_string (char *section, char *entry,	char *def, char *destination, unsigned *length)
{
  const char *buf = get_string(section, entry, def);
  if (!buf)
  {
    return FALSE;
  }
  if (!destination)
  {
    *length = strlen(buf);
    return TRUE;
  }
  if (*length < (strlen(buf)+1))
    return FALSE;
  strcpy(destination, buf);
  return TRUE;
}
//-----------------------------------------------------------------------------
BOOL Registry::get_binary (char *section, char *entry,	char* data, unsigned* size)
{
  DBG_CHECK(status.is(reg_Initialized));
	DBG_CHECK(entry!=NULL);
	DBG_CHECK(size!=NULL);
  unsigned req_size = *size;
	*size = 0;
	LPBYTE ptr = NULL;
	HKEY h_sec_key = lock_key(section);
	if (!h_sec_key)
		return FALSE;
	DWORD dw_type;
  DWORD dw_count;
  BOOL return_value = TRUE;
	LONG res = RegQueryValueEx(h_sec_key, entry, NULL, &dw_type, NULL, &dw_count);
	if (res == ERROR_SUCCESS)
	{
  	*size = dw_count;
		DBG_CHECK(dw_type == REG_BINARY);
    if (data)
    {
      if (req_size>=dw_count)
      {
		    res = RegQueryValueEx(h_sec_key, entry, NULL, &dw_type, (LPBYTE)data, &dw_count);
	      if (res != ERROR_SUCCESS)
		      return_value =  FALSE;
      }
      else
        return_value = FALSE;
    } 
	}
  else
    return_value = FALSE;
	unlock_key (h_sec_key);
	return (return_value);
}
//-----------------------------------------------------------------------------
BOOL Registry::set_int    (char *section, char *entry,	unsigned value)
{
  DBG_CHECK(status.is(reg_Initialized));
	DBG_CHECK(entry!=NULL);
	HKEY h_sec_key = lock_key(section);
	if (!h_sec_key)
		return FALSE;
	LONG res = RegSetValueEx(h_sec_key, entry, NULL, REG_DWORD, (LPBYTE)&value, sizeof(value));
	unlock_key(h_sec_key);
	return (res == ERROR_SUCCESS);
}
//-----------------------------------------------------------------------------
BOOL Registry::set_string (char *section, char *entry,	char *value)
{
  DBG_CHECK(status.is(reg_Initialized));
	DBG_CHECK(entry!=NULL);
	DBG_CHECK(value!=NULL);
	HKEY h_sec_key = lock_key(section);
	if (!h_sec_key)
		return FALSE;
	LONG res = RegSetValueEx(h_sec_key, entry, NULL, REG_SZ, (LPBYTE)value, strlen(value)+1);
	unlock_key(h_sec_key);
	return (res == ERROR_SUCCESS);
}
//-----------------------------------------------------------------------------
BOOL Registry::set_binary (char *section, char *entry,	char* data, unsigned size)
{
  DBG_CHECK(status.is(reg_Initialized));
	DBG_CHECK(entry!=NULL);
	DBG_CHECK(data!=NULL);
	DBG_CHECK(size);
	HKEY h_sec_key = lock_key(section);
	if (!h_sec_key)
		return FALSE;
	LONG res = RegSetValueEx(h_sec_key, entry, NULL, REG_BINARY, (LPBYTE)data, size);
	unlock_key(h_sec_key);
	return (res == ERROR_SUCCESS);
}
//-----------------------------------------------------------------------------
BOOL Registry::delete_entry   (char *section, char *entry)
{
  DBG_CHECK(status.is(reg_Initialized));
	DBG_CHECK(entry!=NULL);
	HKEY h_sec_key = lock_key(section);
	if (!h_sec_key)
		return FALSE;
	LONG res = RegDeleteValue(h_sec_key, entry);
	unlock_key(h_sec_key);
	return (res == ERROR_SUCCESS);
}
//-----------------------------------------------------------------------------
BOOL Registry::delete_section (char *section)
{
  DBG_CHECK(status.is(reg_Initialized));
	DBG_CHECK(section!=NULL);
	HKEY h_app_key = lock_key();
	if (!h_app_key)
		return FALSE;
	LONG res = RegDeleteKey(h_app_key, section);
	unlock_key(h_app_key);
	return (res == ERROR_SUCCESS);
}
//-----------------------------------------------------------------------------
BOOL Registry::delete_application_key(void)
{
  DBG_CHECK(status.is(reg_Initialized));
  HKEY h_soft_key    = NULL;
  HKEY h_company_key = NULL;
  HKEY h_app_key     = NULL;
  HKEY h_sub_app_key = NULL;
  DWORD dw;
  LONG res;
	res = RegOpenKeyEx(base_key, s_software_key, 0, KEY_WRITE|KEY_READ, &h_soft_key);
  if (res != ERROR_SUCCESS) 
    return(FALSE);
  res = RegCreateKeyEx(h_soft_key, s_company_key, 0, REG_NONE, REG_OPTION_NON_VOLATILE, 
                       KEY_WRITE|KEY_READ, NULL,	&h_company_key, &dw);
  RegCloseKey(h_soft_key);
  if (res != ERROR_SUCCESS) 
    return(FALSE);
  if (strlen(s_application_sub_key)>0)
  {
    res = RegCreateKeyEx(h_company_key, s_application_key, 0, REG_NONE, 	REG_OPTION_NON_VOLATILE, 
                           KEY_WRITE|KEY_READ, NULL, &h_app_key, &dw);
    RegCloseKey(h_company_key);
    if (res != ERROR_SUCCESS) 
      return(FALSE);
    RegDeleteKey(h_app_key, s_application_sub_key); 
  	RegCloseKey(h_app_key);
    return(TRUE);
  }
  else 
  {
    RegDeleteKey(h_company_key, s_application_key); 
  	RegCloseKey(h_company_key);
    return(TRUE);
  }
}
//-----------------------------------------------------------------------------
void  Cmd_line::init (char *cmd_line)
{
  DBG_CHECK(!status.is(cmd_Initialized));
  // Port: the entry point passes the command line (was GetCommandLine when NULL).
  snprintf(command_line, line_len, "%s", cmd_line ? cmd_line : "");
  strupr(command_line);
  status.set(cmd_Initialized);
  Comm::quit_me(Cmd_line::quit, "cmd_line", "");
}
//-----------------------------------------------------------------------------
void  Cmd_line::quit ()
{
  status.reset(cmd_Initialized);
}
//-----------------------------------------------------------------------------
char *Cmd_line::get()
{
  DBG_CHECK(status.is(cmd_Initialized));
  strcpy(return_buffer, command_line);
  return(return_buffer);
}
//-----------------------------------------------------------------------------
char *Cmd_line::get_exe (int components)
{
  DBG_CHECK(status.is(cmd_Initialized));
  static char drive_buf    [_MAX_DRIVE];
  static char dir_buf      [_MAX_DIR];
  static char name_buf     [_MAX_FNAME];
  static char ext_buf      [_MAX_EXT];
  static char full_buf     [_MAX_PATH];
  int start = 0;
  char end = ' ';
  if (command_line[0] == '"')
  {
    start = 1;
    end = '"';
  }
  int i = start;
  int j = 0;
  while ((command_line[i])&&(command_line[i]!=end))
    full_buf[j++]=command_line[i++];
  full_buf[j] = '\0';
  _splitpath(full_buf, drive_buf, dir_buf, name_buf, ext_buf);
  return_buffer[0] = 0;
  Bitflag comp(components);
  if (comp.is(file_Drive))
    strcat(return_buffer, drive_buf);
  if (comp.is(file_Dir))
    strcat(return_buffer, dir_buf);
  if (comp.is(file_Name))
    strcat(return_buffer, name_buf);
  if (comp.is(file_Ext))
    strcat(return_buffer, ext_buf);
  return(return_buffer);
}
//-----------------------------------------------------------------------------
char *Cmd_line::get_string(char *label, char *def)
{
  DBG_CHECK(status.is(cmd_Initialized));
  char *return_ptr = return_buffer;
  if ((!label)||(strlen(label)==0))
    return(get());
  int llen = strlen(label);

  int start = 0;
  char end = separator_char;
  if (command_line[0] == '"')
  {
    start = 1;
    end = '"';
  }
  int i = start;
  while ((command_line[i])&&(command_line[i]!=end))
    i++;
  i++;
  char* ptr = &command_line[i];
  char *label_start;
  char *value_start;
  int   label_len;
  int   value_len;
  BOOL  quit = FALSE;
  do
  {
    while ((*ptr)&&(*ptr==separator_char))
      ptr++;
    if (!(*ptr))
    {
      return_ptr = def;
      quit = TRUE;
    }
    else
    {
      label_start = ptr;
      while ((*ptr)&&(*ptr!=separator_char)&&(*ptr!=assignment_char))
        ptr++;
      label_len = ptr-label_start;
      if ((*ptr==separator_char)||(!(*ptr)))
      {
        value_start = ptr;
        value_len   = 0;
      }
      else
      {
        ptr++;
        while ((*ptr)&&(*ptr==separator_char))
          ptr++;
        if (!(*ptr))
        {
          value_start = ptr;
          value_len   = 0;
        }
        else
        {
          BOOL quoted = FALSE;
          if (*ptr=='"')
          {
            ptr++;
            quoted = TRUE;
          }
          value_start = ptr;
          end = separator_char;
          if (quoted)
            end = '"';
          while ((*ptr)&&(*ptr!=end))
            ptr++;
          value_len = ptr-value_start;
          if ((*ptr)&&(quoted))
            ptr++;
        }
      }
      if (strnicmp(label, label_start, label_len)==0)
      {
        if (value_len)
        {
          strncpy(return_buffer, value_start, value_len);
          return_buffer[value_len] = 0;
        }
        else
          strcpy(return_buffer, s_def_yes);
        quit = TRUE;
      }
    }
  } while (!quit);
  return (return_ptr);
}
//-----------------------------------------------------------------------------
unsigned Cmd_line::get_int(char *label, int def)
{
  DBG_CHECK(status.is(cmd_Initialized));
  char *p = get_string(label, NULL);
  if ((!p)||(strlen(p)==0))
    return(def);
  else if ((strcmpi(p,"yes")==0)||
           (strcmpi(p,"on")==0)||
           (strcmpi(p,"true")==0)||
           (strcmpi(p,"enable")==0))
    return (1);
  else if ((strcmpi(p,"no")==0)||
           (strcmpi(p,"off")==0)||
           (strcmpi(p,"false")==0)||
           (strcmpi(p,"disable")==0))
    return (0);
  else
  {
    int val;
    int res = 0;
    if ((strstr(p,"0x")!=NULL)||(strstr(p,"0X")!=NULL))
      res = sscanf(p,"%x",&val);
    else
      res = sscanf(p,"%d",&val);
    if (res)
      return (val);
    else
      return (def);
  }
}
//-----------------------------------------------------------------------------
