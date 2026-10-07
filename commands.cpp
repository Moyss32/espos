#include "commands.h"
#include "config.h"
#include "filesystem.h"
#include "path.h"
#include "shell.h"
#include "system.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

static bool resolveArg(const char* cmd, const char* arg, char* out) {
  if (pathResolve(shellCwd(), arg, out, MAX_PATH_LENGTH)) return true;
  shellError(cmd, "invalid path");
  return false;
}

// Se dstArg é diretório, o destino final é dstArg/<nome da origem>.
static bool resolveTarget(const char* cmd, const char* src, const char* dstArg, char* dst) {
  if (!resolveArg(cmd, dstArg, dst)) return false;
  if (fsIsDir(dst)) {
    char joined[MAX_PATH_LENGTH];
    if (!pathJoin(dst, pathBase(src), joined, sizeof joined)) { shellError(cmd, "invalid path"); return false; }
    strcpy(dst, joined);
  }
  return true;
}

static int cmd_pwd(int, char**) {
  Print& o = shellOut();
  o.print(shellCwd());
  o.print('\n');
  return 0;
}

static bool lsCb(const char* name, bool isDir, void* ctx) {
  Print* o = (Print*)ctx;
  o->print(name);
  if (isDir) o->print('/');
  o->print('\n');
  return true;
}

static int cmd_ls(int argc, char** argv) {
  char p[MAX_PATH_LENGTH];
  if (!resolveArg("ls", argc > 1 ? argv[1] : ".", p)) return 1;
  if (!fsExists(p)) { shellError("ls", "directory not found"); return 1; }
  Print& o = shellOut();
  if (!fsIsDir(p)) { o.print(pathBase(p)); o.print('\n'); return 0; }
  fsEach(p, lsCb, &o);
  return 0;
}

static int cmd_cd(int argc, char** argv) {
  char p[MAX_PATH_LENGTH];
  if (!resolveArg("cd", argc > 1 ? argv[1] : "/home/user", p)) return 1;
  if (!fsExists(p)) { shellError("cd", "directory not found"); return 1; }
  if (!fsIsDir(p)) { shellError("cd", "not a directory"); return 1; }
  shellSetCwd(p);
  return 0;
}

// mkdir/touch só criam caminhos novos; um caminho protegido sempre já existe,
// então a checagem de existência já impede qualquer alteração dele.
static int cmd_mkdir(int argc, char** argv) {
  if (argc < 2) { shellError("mkdir", "missing operand"); return 1; }
  int rc = 0;
  for (int i = 1; i < argc; i++) {
    char p[MAX_PATH_LENGTH];
    if (!resolveArg("mkdir", argv[i], p)) { rc = 1; continue; }
    if (fsExists(p)) { shellError("mkdir", fsIsDir(p) ? "directory already exists" : "file exists"); rc = 1; continue; }
    if (!fsParentIsDir(p) || !fsMkdir(p)) { shellError("mkdir", "invalid path"); rc = 1; }
  }
  return rc;
}

static int cmd_touch(int argc, char** argv) {
  if (argc < 2) { shellError("touch", "missing operand"); return 1; }
  int rc = 0;
  for (int i = 1; i < argc; i++) {
    char p[MAX_PATH_LENGTH];
    if (!resolveArg("touch", argv[i], p)) { rc = 1; continue; }
    if (fsExists(p)) continue;                      // nunca destrói o existente
    if (!fsParentIsDir(p) || !fsCreateFile(p)) { shellError("touch", "invalid path"); rc = 1; }
  }
  return rc;
}

static bool endsWithDot(const char* a) {
  const char* b = pathBase(a);
  return !strcmp(b, ".") || !strcmp(b, "..");
}

static int cmd_rm(int argc, char** argv) {
  bool rec = false;
  int first = 1;
  while (first < argc && argv[first][0] == '-' && argv[first][1]) {
    if (!strcmp(argv[first], "-r") || !strcmp(argv[first], "-R") || !strcmp(argv[first], "-rf")) rec = true;
    else { shellError("rm", "unknown option"); return 1; }
    first++;
  }
  if (first >= argc) { shellError("rm", "missing operand"); return 1; }

  int rc = 0;
  for (int i = first; i < argc; i++) {
    char p[MAX_PATH_LENGTH];
    if (endsWithDot(argv[i])) { shellError("rm", "refusing to remove '.' or '..'"); rc = 1; continue; }
    if (!resolveArg("rm", argv[i], p)) { rc = 1; continue; }
    if (isProtectedPath(p)) { shellError("rm", "protected system path"); rc = 1; continue; }
    if (!fsExists(p)) { shellError("rm", "file not found"); rc = 1; continue; }
    if (pathIsInside(shellCwd(), p)) { shellError("rm", "cannot remove current directory"); rc = 1; continue; }
    if (fsIsDir(p) && !rec && !fsDirIsEmpty(p)) { shellError("rm", "directory not empty"); rc = 1; continue; }
    if (!fsRemove(p, rec)) { shellError("rm", "operation failed"); rc = 1; }
  }
  return rc;
}

static int cmd_echo(int argc, char** argv) {
  Print& o = shellOut();
  for (int i = 1; i < argc; i++) {
    if (i > 1) o.print(' ');
    o.print(argv[i]);
  }
  o.print('\n');
  return 0;
}

static int cmd_cat(int argc, char** argv) {
  if (argc < 2) { shellError("cat", "missing operand"); return 1; }
  int rc = 0;
  for (int i = 1; i < argc; i++) {
    char p[MAX_PATH_LENGTH];
    if (!resolveArg("cat", argv[i], p)) { rc = 1; continue; }
    if (!fsExists(p)) { shellError("cat", "file not found"); rc = 1; continue; }
    if (fsIsDir(p)) { shellError("cat", "is a directory"); rc = 1; continue; }
    if (!fsStream(p, shellOut())) { shellError("cat", "read error"); rc = 1; }
  }
  return rc;
}

static int cmd_clear(int argc, char** argv) {
  Print& o = shellOut();
  if (argc > 1 && !strcmp(argv[1], "-f")) { for (int i = 0; i < 40; i++) o.print('\n'); }  // sem ANSI
  else o.print("\x1b[2J\x1b[H");
  return 0;
}

static int cmd_reboot(int, char**) { sysReboot(); return 0; }

static int cmd_help(int, char**) {
  Print& o = shellOut();
  o.print("MiniOS commands:\n\n");
  for (size_t i = 0; i < kCommandCount; i++)
    if (kCommands[i].help) o.printf("%-9s %s\n", kCommands[i].name, kCommands[i].help);
  return 0;
}

static int cmd_uname(int argc, char** argv) {
  sysUname(shellOut(), argc > 1 && !strcmp(argv[1], "-a"));
  return 0;
}

struct TreeCtx { char path[MAX_PATH_LENGTH]; int depth; };

static bool treeCb(const char* name, bool isDir, void* ctx) {
  TreeCtx* t = (TreeCtx*)ctx;
  Print& o = shellOut();
  for (int i = 0; i < t->depth; i++) o.print("  ");
  o.print(name);
  if (isDir) o.print('/');
  o.print('\n');
  if (isDir && t->depth < MAX_TREE_DEPTH) {
    size_t len = strlen(t->path);
    int n = snprintf(t->path + len, sizeof(t->path) - len, "%s%s", len > 1 ? "/" : "", name);
    if (n > 0 && (size_t)n < sizeof(t->path) - len) {
      t->depth++;
      fsEach(t->path, treeCb, t);
      t->depth--;
    }
    t->path[len] = '\0';                            // um único buffer para toda a recursão
  }
  return true;
}

static int cmd_tree(int argc, char** argv) {
  TreeCtx t;
  if (!resolveArg("tree", argc > 1 ? argv[1] : ".", t.path)) return 1;
  if (!fsIsDir(t.path)) { shellError("tree", "directory not found"); return 1; }
  Print& o = shellOut();
  o.print(t.path);
  o.print('\n');
  t.depth = 1;
  fsEach(t.path, treeCb, &t);
  return 0;
}

static int cmd_cp(int argc, char** argv) {
  if (argc != 3) { shellError("cp", "usage: cp SRC DST"); return 1; }
  char src[MAX_PATH_LENGTH], dst[MAX_PATH_LENGTH];
  if (!resolveArg("cp", argv[1], src)) return 1;
  if (!fsExists(src)) { shellError("cp", "file not found"); return 1; }
  if (fsIsDir(src)) { shellError("cp", "source is a directory"); return 1; }
  if (!resolveTarget("cp", src, argv[2], dst)) return 1;
  if (isProtectedPath(dst)) { shellError("cp", "protected system path"); return 1; }
  if (!strcmp(src, dst)) { shellError("cp", "same file"); return 1; }
  if (fsIsDir(dst)) { shellError("cp", "destination is a directory"); return 1; }
  if (!fsParentIsDir(dst)) { shellError("cp", "invalid path"); return 1; }
  if (!fsCopy(src, dst)) { shellError("cp", "operation failed"); return 1; }
  return 0;
}

static int cmd_mv(int argc, char** argv) {
  if (argc != 3) { shellError("mv", "usage: mv SRC DST"); return 1; }
  char src[MAX_PATH_LENGTH], dst[MAX_PATH_LENGTH];
  if (!resolveArg("mv", argv[1], src)) return 1;
  if (isProtectedPath(src)) { shellError("mv", "protected system path"); return 1; }
  if (!fsExists(src)) { shellError("mv", "file not found"); return 1; }
  if (!resolveTarget("mv", src, argv[2], dst)) return 1;
  if (isProtectedPath(dst)) { shellError("mv", "protected system path"); return 1; }
  if (!strcmp(src, dst)) return 0;
  bool srcDir = fsIsDir(src);
  if (srcDir && pathIsInside(dst, src)) { shellError("mv", "cannot move into itself"); return 1; }
  if (pathIsInside(shellCwd(), src)) { shellError("mv", "cannot move current directory"); return 1; }
  if (fsExists(dst) && (srcDir || fsIsDir(dst))) { shellError("mv", "destination exists"); return 1; }
  if (!fsParentIsDir(dst)) { shellError("mv", "invalid path"); return 1; }
  if (!fsRename(src, dst)) { shellError("mv", "operation failed"); return 1; }
  return 0;
}

static int cmd_free(int, char**) { sysFree(shellOut()); return 0; }

static int cmd_df(int, char**) {
  size_t total = fsTotalBytes(), used = fsUsedBytes();
  shellOut().printf("LittleFS: total %u KB, used %u KB, free %u KB\n",
                    (unsigned)(total / 1024), (unsigned)(used / 1024), (unsigned)((total - used) / 1024));
  return 0;
}

static int cmd_uptime(int, char**) { sysUptime(shellOut()); return 0; }

const Command kCommands[] = {
  {"pwd",    "Show current directory",   cmd_pwd},
  {"ls",     "List directory",           cmd_ls},
  {"cd",     "Change directory",         cmd_cd},
  {"mkdir",  "Create directory",         cmd_mkdir},
  {"touch",  "Create file",              cmd_touch},
  {"rm",     "Remove file/directory",    cmd_rm},
  {"echo",   "Print text",               cmd_echo},
  {"cat",    "Read file",                cmd_cat},
  {"clear",  "Clear terminal",           cmd_clear},
  {"reboot", "Restart system",           cmd_reboot},
  {"help",   "Show this help",           cmd_help},
  {"uname",  "System info (-a: more)",   cmd_uname},
  {"tree",   "Show directory tree",      cmd_tree},
  {"cp",     "Copy file",                cmd_cp},
  {"mv",     "Move/rename",              cmd_mv},
  {"free",   "Show RAM usage",           cmd_free},
  {"df",     "Show filesystem usage",    cmd_df},
  {"uptime", "Show time since boot",     cmd_uptime},
};
const size_t kCommandCount = sizeof(kCommands) / sizeof(kCommands[0]);
