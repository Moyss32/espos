#include "filesystem.h"
#include "config.h"
#include "path.h"
#include <LittleFS.h>
#include <string.h>
#include <stdio.h>

static const char kOsConf[] = "hostname=esp32\nshell=minish\nversion=" MINIOS_VERSION "\n";

bool fsMount() { return LittleFS.begin(true); }  // true = formata se falhar (1º boot)

int fsCreateBaseTree() {
  int created = 0;
  for (size_t i = 1; i < kBaseDirCount; i++) {
    if (!fsExists(kBaseDirs[i]) && LittleFS.mkdir(kBaseDirs[i])) created++;
  }
  if (!fsExists("/etc/os.conf")) {
    File f = LittleFS.open("/etc/os.conf", "w");
    if (f) { f.print(kOsConf); f.close(); created++; }
  }
  return created;
}

bool fsExists(const char* path) {
  return strcmp(path, "/") == 0 || LittleFS.exists(path);
}

bool fsIsDir(const char* path) {
  if (strcmp(path, "/") == 0) return true;
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  bool d = f.isDirectory();
  f.close();
  return d;
}

bool fsParentIsDir(const char* path) {
  char parent[MAX_PATH_LENGTH];
  size_t len = strlen(path);
  if (len == 0 || len >= sizeof parent) return false;
  memcpy(parent, path, len + 1);
  char* slash = strrchr(parent, '/');
  if (!slash) return false;
  if (slash == parent) parent[1] = '\0'; else *slash = '\0';
  return fsIsDir(parent);
}

bool fsEach(const char* dir, FsVisitor cb, void* ctx) {
  File d = LittleFS.open(dir, "r");
  if (!d || !d.isDirectory()) return false;
  File f = d.openNextFile();
  while (f) {
    if (!cb(f.name(), f.isDirectory(), ctx)) break;
    f = d.openNextFile();
  }
  return true;
}

static bool emptyCb(const char*, bool, void* ctx) { *(bool*)ctx = false; return false; }

bool fsDirIsEmpty(const char* path) {
  bool empty = true;
  return fsEach(path, emptyCb, &empty) && empty;
}

bool fsMkdir(const char* path) { return LittleFS.mkdir(path); }

bool fsCreateFile(const char* path) {
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  f.close();
  return true;
}

// ---- remoção recursiva sem carregar a árvore na RAM ----
struct FirstCtx { char name[MAX_NAME_LENGTH + 1]; bool isDir; bool found; };

static bool firstCb(const char* name, bool isDir, void* ctx) {
  FirstCtx* c = (FirstCtx*)ctx;
  snprintf(c->name, sizeof c->name, "%s", name);
  c->isDir = isDir;
  c->found = true;
  return false;
}

// Pega a primeira entrada, apaga, repete. Evita modificar o diretório durante a iteração.
static bool rmTree(const char* path, int depth) {
  if (depth > MAX_TREE_DEPTH) return false;
  for (;;) {
    FirstCtx c;
    c.found = false;
    if (!fsEach(path, firstCb, &c)) return false;
    if (!c.found) break;
    char child[MAX_PATH_LENGTH];
    int n = snprintf(child, sizeof child, "%s%s%s", path, strcmp(path, "/") ? "/" : "", c.name);
    if (n <= 0 || (size_t)n >= sizeof child) return false;
    if (c.isDir) { if (!rmTree(child, depth + 1)) return false; }
    else if (!LittleFS.remove(child)) return false;
  }
  return LittleFS.rmdir(path);
}

bool fsRemove(const char* path, bool recursive) {
  if (isProtectedPath(path)) return false;   // última barreira, independe do comando
  if (fsIsDir(path)) return recursive ? rmTree(path, 0) : LittleFS.rmdir(path);
  return LittleFS.remove(path);
}

bool fsRename(const char* from, const char* to) {
  if (isProtectedPath(from) || isProtectedPath(to)) return false;
  return LittleFS.rename(from, to);
}

bool fsCopy(const char* from, const char* to) {
  if (isProtectedPath(to)) return false;
  File in = LittleFS.open(from, "r");
  if (!in || in.isDirectory()) return false;
  File out = LittleFS.open(to, "w");
  if (!out) { in.close(); return false; }
  uint8_t buf[128];
  size_t n;
  bool ok = true;
  while ((n = in.read(buf, sizeof buf)) > 0) {
    if (out.write(buf, n) != n) { ok = false; break; }
  }
  in.close();
  out.close();
  return ok;
}

bool fsStream(const char* path, Print& out) {
  File f = LittleFS.open(path, "r");
  if (!f || f.isDirectory()) return false;
  uint8_t buf[64];
  size_t n;
  while ((n = f.read(buf, sizeof buf)) > 0) out.write(buf, n);
  f.close();
  return true;
}

File fsOpenWrite(const char* path, bool append) {
  return LittleFS.open(path, append ? "a" : "w");
}

size_t fsTotalBytes() { return LittleFS.totalBytes(); }
size_t fsUsedBytes()  { return LittleFS.usedBytes(); }
