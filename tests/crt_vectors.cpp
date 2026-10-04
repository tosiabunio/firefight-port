// Checks the MSVC 4 rand and qsort clones (source/compat/msvc4.h) against the test vectors
// that tools/archive/crt_vectors.py took from the original exe.
//
//   crt_vectors <crt_rand.txt> <crt_qsort.txt>

#include <compat/msvc4.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<unsigned char> hex_bytes(const std::string &hex)
{
  std::vector<unsigned char> out;
  if (hex == "-")
    return out;
  for (size_t i = 0; i + 1 < hex.size(); i += 2)
    out.push_back((unsigned char)std::stoi(hex.substr(i, 2), nullptr, 16));
  return out;
}

// The vectors' comparator: keys[a] - keys[b], with a and b the first bytes of the elements.
const unsigned char *keys;

int compare(const void *a, const void *b)
{
  return keys[*(const unsigned char *)a] - keys[*(const unsigned char *)b];
}

int check_rand(const char *path)
{
  std::ifstream in(path);
  if (!in)
  {
    fprintf(stderr, "cannot open %s\n", path);
    return 1;
  }
  msvc4_srand(0);
  std::string line;
  int count = 0;
  while (std::getline(in, line))
  {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream values(line);
    int expected;
    while (values >> expected)
    {
      const int actual = msvc4_rand();
      if (actual != expected)
      {
        fprintf(stderr, "rand call %d: %d, expected %d\n", count + 1, actual, expected);
        return 1;
      }
      count++;
    }
  }
  printf("rand: %d values match\n", count);
  return count == 1024 ? 0 : 1;
}

int check_qsort(const char *path)
{
  std::ifstream in(path);
  if (!in)
  {
    fprintf(stderr, "cannot open %s\n", path);
    return 1;
  }
  std::string line;
  int count = 0, failed = 0;
  while (std::getline(in, line))
  {
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream fields(line);
    size_t width;
    std::string key_hex, order_hex;
    if (!(fields >> width >> key_hex >> order_hex))
    {
      fprintf(stderr, "bad line: %s\n", line.c_str());
      return 1;
    }
    const std::vector<unsigned char> key_list = hex_bytes(key_hex);
    const std::vector<unsigned char> order = hex_bytes(order_hex);
    const size_t n = key_list.size();
    std::vector<unsigned char> buf(n * width, 0);
    for (size_t i = 0; i < n; i++)
      buf[i * width] = (unsigned char)i;
    keys = key_list.data();
    msvc4_qsort(buf.data(), n, width, compare);
    for (size_t i = 0; i < n; i++)
      if (buf[i * width] != order[i])
      {
        fprintf(stderr, "qsort, width %zu, keys %s: order differs at %zu\n", width,
                key_hex.c_str(), i);
        failed++;
        break;
      }
    count++;
  }
  printf("qsort: %d of %d vectors match\n", count - failed, count);
  return failed || count == 0 ? 1 : 0;
}

} // namespace

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "usage: crt_vectors <crt_rand.txt> <crt_qsort.txt>\n");
    return 2;
  }
  const int rand_failed = check_rand(argv[1]);
  const int qsort_failed = check_qsort(argv[2]);
  return rand_failed || qsort_failed ? 1 : 0;
}
