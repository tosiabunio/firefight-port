#include "1rg_hdrs.h"
#include <compat/browser.h>
#include <filesystem>
#include <map>
#include <string>
#include <system_error>


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
//=============================================================================
// Registry methods
//=============================================================================
// Port: the registry is a settings file, <preferences>/settings.ini, read at init and written
// back at quit (in the browser, at every change). Each registry key below the application key is
// an INI section and each value an entry, tagged with its registry type:
//
//   [spr]
//   hires mode=dword:1
//   [Player:default]
//   easy=hex:0b1a...
//
// Section and entry names are case-insensitive, as in the registry. The machine-wide part
// (mode_Machine, used only for the patch directory) is always empty. Values the original
// launcher always wrote, and for which neither the code nor cwe.ini has a default, are built in
// (launcher_defaults).
namespace {

// Value types, as in the registry.
enum { type_dword, type_string, type_binary };

struct Reg_value
{
  std::string name;
  DWORD       type;
  std::string data;
};

struct Reg_section
{
  std::string name;
  std::map<std::string, Reg_value> values;  // by lowercase name
};

typedef std::map<std::string, Reg_section> Reg_store;  // by lowercase name

// Never destroyed: Registry::quit runs among the final quits, after static destructors (see
// Comm_init in 1lg.h).
Reg_store &user_store = *new Reg_store;
Reg_store &machine_store = *new Reg_store;
bool      machine_mode = false;
bool      dirty        = false;

struct Launcher_default
{
  const char *section;
  const char *entry;
  unsigned    value;
};

// Matches RegData's defaults: 640x480 hires and 320x240 lores.
const Launcher_default launcher_defaults[] = {
  {"spr", "hires mode", 1},
  {"spr", "lores mode", 1},
};

std::string lower(const char *s)
{
  std::string result(s ? s : "");
  for (char &c : result)
    c = (char)tolower((unsigned char)c);
  return result;
}

Reg_store &store()
{
  return machine_mode ? machine_store : user_store;
}

std::string settings_file()
{
  return std::string(Comm::pref_path) + "settings.ini";
}

const Reg_value *find_value(const char *section, const char *entry)
{
  Reg_store &s = store();
  auto sec = s.find(lower(section));
  if (sec == s.end())
    return nullptr;
  auto val = sec->second.values.find(lower(entry));
  return (val == sec->second.values.end()) ? nullptr : &val->second;
}

void put_value(const char *section, const char *entry, DWORD type, const void *data, size_t size)
{
  Reg_section &sec = store()[lower(section)];
  if (sec.name.empty())
    sec.name = section ? section : "";
  Reg_value &val = sec.values[lower(entry)];
  if (val.name.empty())
    val.name = entry;
  else if (val.type == type && val.data.size() == size && memcmp(val.data.data(), data, size) == 0)
    return;  // unchanged
  val.type = type;
  val.data.assign((const char *)data, size);
  if (!machine_mode)
    dirty = true;
}

void load_settings()
{
  user_store.clear();
  FILE *file = fopen(settings_file().c_str(), "rb");
  if (!file)
    return;
  char line[4096];
  std::string section;
  bool have_section = false;
  while (fgets(line, sizeof(line), file))
  {
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
      line[--len] = 0;
    if (len == 0 || line[0] == ';')
      continue;
    if (line[0] == '[' && line[len-1] == ']')
    {
      section.assign(line + 1, len - 2);
      have_section = true;
      continue;
    }
    char *eq = strchr(line, '=');
    if (!have_section || !eq)
      continue;
    *eq = 0;
    const char *entry = line;
    const char *value = eq + 1;
    if (strncmp(value, "dword:", 6) == 0)
    {
      DWORD v = (DWORD)strtoul(value + 6, NULL, 10);
      put_value(section.c_str(), entry, type_dword, &v, sizeof(v));
    }
    else if (strncmp(value, "str:", 4) == 0)
      put_value(section.c_str(), entry, type_string, value + 4, strlen(value + 4) + 1);
    else if (strncmp(value, "hex:", 4) == 0)
    {
      std::string bytes;
      for (const char *h = value + 4; h[0] && h[1]; h += 2)
      {
        char pair[3] = {h[0], h[1], 0};
        bytes += (char)strtoul(pair, NULL, 16);
      }
      put_value(section.c_str(), entry, type_binary, bytes.data(), bytes.size());
    }
  }
  fclose(file);
  dirty = false;
}

void save_settings()
{
  if (!dirty)
    return;
  std::string path = settings_file();
  std::string temp = path + ".new";
  FILE *file = fopen(temp.c_str(), "wb");
  if (!file)
  {
    WARNING("cannot write settings file %s", (char*)path.c_str());
    return;
  }
  fputs("; Fire Fight settings (the original kept these in the Windows registry)\n", file);
  for (auto &sec : user_store)
  {
    fprintf(file, "[%s]\n", sec.second.name.c_str());
    for (auto &v : sec.second.values)
    {
      const Reg_value &val = v.second;
      if (val.type == type_dword && val.data.size() == sizeof(DWORD))
      {
        DWORD d;
        memcpy(&d, val.data.data(), sizeof(d));
        fprintf(file, "%s=dword:%u\n", val.name.c_str(), (unsigned)d);
      }
      else if (val.type == type_string)
        fprintf(file, "%s=str:%s\n", val.name.c_str(), val.data.c_str());
      else
      {
        fprintf(file, "%s=hex:", val.name.c_str());
        for (unsigned char c : val.data)
          fprintf(file, "%02x", c);
        fputs("\n", file);
      }
    }
  }
  bool ok = (fclose(file) == 0);
  std::error_code ec;
  if (ok)
    std::filesystem::rename(temp, path, ec);
  if (!ok || ec)
    WARNING("cannot write settings file %s", (char*)path.c_str());
  else
  {
    dirty = false;
    browser_pref_written();
  }
}

// A page in a browser is closed rather than quit, and the quits don't run then, so the browser
// build writes the file at every change.
void changed()
{
#ifdef FF_BROWSER
  save_settings();
#endif
}

} // namespace
//-----------------------------------------------------------------------------
void Registry::quit (void)
{
  save_settings();
  status.reset(reg_Initialized);
}
//-----------------------------------------------------------------------------
void Registry::init (char *application_name, char *application_sub_name, int mode, int version)
{
  DBG_CHECK(!status.is(reg_Initialized));
  DBG_CHECK(application_name!=NULL);
  DBG_CHECK(strlen(application_name)<(REG_APP_NAME_LEN-1));
  DBG_CHECK((application_sub_name==NULL)||(strlen(application_sub_name)<(REG_APP_NAME_LEN-1)));
  strcpy(s_application_key, application_name);
  if (application_sub_name!=NULL)
    strcpy(s_application_sub_key, application_sub_name);
  else
    s_application_sub_key[0] = 0; 
  load_settings();
  MESSAGE("settings: %s", (char*)settings_file().c_str());
  if ((mode != mode_User)&&(mode != mode_Machine))
    FAILURE("Registry::init - invalid mode");
  machine_mode = (mode == mode_Machine);
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
  if ((mode != mode_User)&&(mode != mode_Machine))
    FAILURE("Registry::set_mode - invalid mode");
  machine_mode = (mode == mode_Machine);
  if (version!=0)
  {
    unsigned ver = get_int (NULL, (char *)s_version_key, 0);
    if (ver < (unsigned)version)
      delete_application_key();
    set_int(NULL, (char*)s_version_key, version);
  }
}
//-----------------------------------------------------------------------------
unsigned Registry::get_int    (char *section, char *entry,	unsigned def)
{
  DBG_CHECK(status.is(reg_Initialized));
  DBG_CHECK(entry!=NULL);
  const Reg_value *val = find_value(section, entry);
  if (val && val->type == type_dword && val->data.size() == sizeof(DWORD))
  {
    DWORD d;
    memcpy(&d, val->data.data(), sizeof(d));
    return (unsigned)d;
  }
  if (!val && !machine_mode)
    for (const Launcher_default &d : launcher_defaults)
      if ((lower(section) == d.section) && (lower(entry) == d.entry))
        return d.value;
  return def;
}
//-----------------------------------------------------------------------------
const char *Registry::get_string (char *section, char *entry,	char *def)
{
  DBG_CHECK(status.is(reg_Initialized));
  DBG_CHECK(entry!=NULL);
  const Reg_value *val = find_value(section, entry);
  if (val && val->type == type_string && val->data.size() <= REG_BUFFER_LEN)
  {
    memcpy(buffer, val->data.data(), val->data.size());
    buffer[REG_BUFFER_LEN-1] = 0;
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
  const Reg_value *val = find_value(section, entry);
  if (!val || val->type != type_binary)
    return FALSE;
  *size = (unsigned)val->data.size();
  if (data)
  {
    if (req_size < val->data.size())
      return FALSE;
    memcpy(data, val->data.data(), val->data.size());
  }
  return TRUE;
}
//-----------------------------------------------------------------------------
BOOL Registry::set_int    (char *section, char *entry,	unsigned value)
{
  DBG_CHECK(status.is(reg_Initialized));
  DBG_CHECK(entry!=NULL);
  DWORD d = value;
  put_value(section, entry, type_dword, &d, sizeof(d));
  changed();
  return TRUE;
}
//-----------------------------------------------------------------------------
BOOL Registry::set_string (char *section, char *entry,	char *value)
{
  DBG_CHECK(status.is(reg_Initialized));
  DBG_CHECK(entry!=NULL);
  DBG_CHECK(value!=NULL);
  put_value(section, entry, type_string, value, strlen(value)+1);
  changed();
  return TRUE;
}
//-----------------------------------------------------------------------------
BOOL Registry::set_binary (char *section, char *entry,	char* data, unsigned size)
{
  DBG_CHECK(status.is(reg_Initialized));
  DBG_CHECK(entry!=NULL);
  DBG_CHECK(data!=NULL);
  DBG_CHECK(size);
  put_value(section, entry, type_binary, data, size);
  changed();
  return TRUE;
}
//-----------------------------------------------------------------------------
BOOL Registry::delete_entry   (char *section, char *entry)
{
  DBG_CHECK(status.is(reg_Initialized));
  DBG_CHECK(entry!=NULL);
  Reg_store &s = store();
  auto sec = s.find(lower(section));
  if (sec == s.end() || sec->second.values.erase(lower(entry)) == 0)
    return FALSE;
  if (!machine_mode)
    dirty = true;
  changed();
  return TRUE;
}
//-----------------------------------------------------------------------------
BOOL Registry::delete_section (char *section)
{
  DBG_CHECK(status.is(reg_Initialized));
  DBG_CHECK(section!=NULL);
  if (store().erase(lower(section)) == 0)
    return FALSE;
  if (!machine_mode)
    dirty = true;
  changed();
  return TRUE;
}
//-----------------------------------------------------------------------------
BOOL Registry::delete_application_key(void)
{
  DBG_CHECK(status.is(reg_Initialized));
  store().clear();
  if (!machine_mode)
    dirty = true;
  changed();
  return TRUE;
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
