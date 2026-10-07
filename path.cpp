#include "path.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

const char* const kBaseDirs[] = {
  "/", "/bin", "/etc", "/home", "/home/user", "/tmp",
  "/usr", "/usr/bin", "/usr/lib", "/var", "/var/log", "/var/data"
};
const size_t kBaseDirCount = sizeof(kBaseDirs) / sizeof(kBaseDirs[0]);

// `out` já contém um caminho absoluto normalizado; aplica os segmentos de `s` sobre ele.
static bool applySegments(const char* s, char* out, size_t cap) {
  size_t len = strlen(out);
  while (*s) {
    while (*s == '/') s++;
    if (!*s) break;
    const char* e = s;
    while (*e && *e != '/') e++;
    size_t n = (size_t)(e - s);

    if (n == 1 && s[0] == '.') {
      // ignora
    } else if (n == 2 && s[0] == '.' && s[1] == '.') {
      if (len > 1) {
        while (out[len - 1] != '/') len--;
        if (len > 1) len--;
        out[len] = '\0';
      }
    } else {
      size_t sep = (len > 1) ? 1 : 0;
      if (len + sep + n + 1 > cap) return false;
      if (sep) out[len++] = '/';
      memcpy(out + len, s, n);
      len += n;
      out[len] = '\0';
    }
    s = e;
  }
  return true;
}

bool pathResolve(const char* cwd, const char* in, char* out, size_t outSize) {
  if (!in || !out || outSize < 2) return false;
  out[0] = '/';
  out[1] = '\0';
  if (in[0] != '/' && cwd && !applySegments(cwd, out, outSize)) return false;
  return applySegments(in, out, outSize);
}

bool pathJoin(const char* dir, const char* name, char* out, size_t outSize) {
  int n = snprintf(out, outSize, "%s%s%s", dir, strcmp(dir, "/") ? "/" : "", name);
  return n > 0 && (size_t)n < outSize;
}

const char* pathBase(const char* path) {
  const char* b = strrchr(path, '/');
  return b ? b + 1 : path;
}

bool pathIsInside(const char* path, const char* dir) {
  size_t n = strlen(dir);
  if (n == 1 && dir[0] == '/') return true;
  return strncmp(path, dir, n) == 0 && (path[n] == '\0' || path[n] == '/');
}

bool isProtectedPath(const char* path) {
  char norm[MAX_PATH_LENGTH];
  if (!path || !pathResolve(nullptr, path, norm, sizeof norm)) return true;  // na dúvida, protege
  for (size_t i = 0; i < kBaseDirCount; i++)
    if (strcmp(norm, kBaseDirs[i]) == 0) return true;
  return false;
}
