// Microsoft C runtime extensions for non-MSVC builds (see crt.h).

#include <compat/crt.h>

#ifndef _MSC_VER

#include <dirent.h>
#include <fnmatch.h>

#include <memory>
#include <string>
#include <vector>

int strcmpi(const char *a, const char *b)
{
  return strnicmp(a, b, (size_t)-1);
}

int stricmp(const char *a, const char *b)
{
  return strnicmp(a, b, (size_t)-1);
}

// Locale-independent, like the MSVC "C" locale: only ASCII letters fold.
int strnicmp(const char *a, const char *b, size_t n)
{
  for (size_t i = 0; i < n; i++)
  {
    const int ca = tolower((unsigned char)a[i]);
    const int cb = tolower((unsigned char)b[i]);
    if (ca != cb || ca == 0)
      return ca - cb;
  }
  return 0;
}

char *strupr(char *s)
{
  for (char *p = s; *p; p++)
    *p = (char)toupper((unsigned char)*p);
  return s;
}

char *strlwr(char *s)
{
  for (char *p = s; *p; p++)
    *p = (char)tolower((unsigned char)*p);
  return s;
}

static void copy_part(char *out, const char *begin, const char *end)
{
  if (!out)
    return;
  const size_t len = (size_t)(end - begin);
  memcpy(out, begin, len);
  out[len] = 0;
}

void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext)
{
  const char *p = path;
  if (p[0] && p[1] == ':')
  {
    copy_part(drive, p, p + 2);
    p += 2;
  }
  else
    copy_part(drive, p, p);

  const char *last_sep = nullptr;
  for (const char *q = p; *q; q++)
    if (*q == '/' || *q == '\\')
      last_sep = q;
  const char *name = last_sep ? last_sep + 1 : p;
  copy_part(dir, p, name);

  const char *dot = strrchr(name, '.');
  const char *end = name + strlen(name);
  if (!dot)
    dot = end;
  copy_part(fname, name, dot);
  copy_part(ext, dot, end);
}

int _open(const char *name, int flags, ...)
{
  int mode = 0;
  if (flags & O_CREAT)
  {
    va_list args;
    va_start(args, flags);
    mode = va_arg(args, int);
    va_end(args);
  }
  return open(name, flags, mode);
}

int _creat(const char *name, int mode)
{
  return creat(name, (mode_t)mode);
}

long _filelength(int fd)
{
  struct stat st;
  if (fstat(fd, &st) != 0)
    return -1;
  return (long)st.st_size;
}

int _commit(int fd)
{
  return fsync(fd);
}

namespace {

struct Find_state
{
  std::string dir;
  std::string pattern;
  DIR *handle = nullptr;
};

std::vector<std::unique_ptr<Find_state>> find_states;

bool find_next_match(Find_state &state, _finddata_t *data)
{
  while (dirent *entry = readdir(state.handle))
  {
    if (fnmatch(state.pattern.c_str(), entry->d_name, FNM_CASEFOLD) != 0)
      continue;
    const std::string full = state.dir + entry->d_name;
    struct stat st;
    if (stat(full.c_str(), &st) != 0)
      continue;
    memset(data, 0, sizeof(*data));
    data->attrib = S_ISDIR(st.st_mode) ? 0x10u : 0u;
    data->time_create = st.st_ctime;
    data->time_access = st.st_atime;
    data->time_write = st.st_mtime;
    data->size = (uint32_t)st.st_size;
    snprintf(data->name, sizeof(data->name), "%s", entry->d_name);
    return true;
  }
  return false;
}

} // namespace

intptr_t _findfirst(const char *spec, _finddata_t *data)
{
  std::string s(spec);
  for (char &c : s)
    if (c == '\\')
      c = '/';
  const size_t sep = s.rfind('/');
  auto state = std::make_unique<Find_state>();
  state->dir = (sep == std::string::npos) ? std::string() : s.substr(0, sep + 1);
  state->pattern = (sep == std::string::npos) ? s : s.substr(sep + 1);
  if (state->pattern == "*.*")
    state->pattern = "*";
  state->handle = opendir(state->dir.empty() ? "." : state->dir.c_str());
  if (!state->handle)
    return -1;
  if (!find_next_match(*state, data))
  {
    closedir(state->handle);
    return -1;
  }
  for (size_t i = 0; i < find_states.size(); i++)
    if (!find_states[i])
    {
      find_states[i] = std::move(state);
      return (intptr_t)i;
    }
  find_states.push_back(std::move(state));
  return (intptr_t)(find_states.size() - 1);
}

int _findnext(intptr_t handle, _finddata_t *data)
{
  if (handle < 0 || (size_t)handle >= find_states.size() || !find_states[(size_t)handle])
    return -1;
  return find_next_match(*find_states[(size_t)handle], data) ? 0 : -1;
}

int _findclose(intptr_t handle)
{
  if (handle < 0 || (size_t)handle >= find_states.size() || !find_states[(size_t)handle])
    return -1;
  closedir(find_states[(size_t)handle]->handle);
  find_states[(size_t)handle].reset();
  return 0;
}

intptr_t _execvp(const char *file, const char *const *argv)
{
  return execvp(file, const_cast<char *const *>(argv));
}

#endif
