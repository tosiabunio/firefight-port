// Microsoft C runtime extensions for non-MSVC builds (see crt.h).

#include <compat/crt.h>

#ifndef _MSC_VER

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

#endif
