#include "shell.h"
#include "commands.h"
#include "config.h"
#include "filesystem.h"
#include "path.h"
#include <string.h>

// Converte '\n' em "\r\n" para o terminal; arquivos recebem '\n' puro.
class TermOut : public Print {
 public:
  using Print::write;
  size_t write(uint8_t c) override {
    if (c == '\n') Serial.write((uint8_t)'\r');
    return Serial.write(c);
  }
  size_t write(const uint8_t* b, size_t n) override {
    size_t i = 0;
    while (i < n) {
      size_t j = i;
      while (j < n && b[j] != '\n') j++;
      if (j > i) Serial.write(b + i, j - i);
      if (j < n) { Serial.write((const uint8_t*)"\r\n", 2); j++; }
      i = j;
    }
    return n;
  }
};

static TermOut g_term;
static Print*  g_out = &g_term;
static char    g_cwd[MAX_PATH_LENGTH] = "/";
static char    g_line[MAX_COMMAND_LENGTH];
static size_t  g_len = 0;
static bool    g_lastCR = false;

Print& shellOut() { return *g_out; }
const char* shellCwd() { return g_cwd; }
void shellSetCwd(const char* p) { snprintf(g_cwd, sizeof g_cwd, "%s", p); }
void shellPrompt() { g_term.printf("[%s] > ", g_cwd); }

void shellError(const char* cmd, const char* msg) {
  g_term.print(cmd);
  g_term.print(": ");
  g_term.print(msg);
  g_term.print('\n');
}

// Separa argumentos in-place. Aspas simples/duplas agrupam. -1 = muitos args, -2 = aspa aberta.
static int tokenize(char* s, char** argv, bool* quoted) {
  int argc = 0;
  for (;;) {
    while (*s == ' ' || *s == '\t') s++;
    if (!*s) break;
    if ((size_t)argc >= MAX_ARGUMENTS) return -1;
    if (*s == '"' || *s == '\'') {
      char q = *s++;
      argv[argc] = s;
      quoted[argc] = true;
      while (*s && *s != q) s++;
      if (!*s) return -2;
      *s++ = '\0';
    } else {
      argv[argc] = s;
      quoted[argc] = false;
      while (*s && *s != ' ' && *s != '\t') s++;
      if (*s) *s++ = '\0';
    }
    argc++;
  }
  return argc;
}

static void execute(char* line) {
  char* argv[MAX_ARGUMENTS];
  bool quoted[MAX_ARGUMENTS];
  int argc = tokenize(line, argv, quoted);
  if (argc == -1) { shellError("sh", "too many arguments"); return; }
  if (argc == -2) { shellError("sh", "unterminated quote"); return; }
  if (argc == 0) return;

  // Redirecionamento: ">" ou ">>" (sem aspas) seguido do arquivo, no fim da linha.
  const char* target = nullptr;
  bool append = false;
  for (int i = 0; i < argc; i++) {
    if (quoted[i]) continue;
    bool gt = !strcmp(argv[i], ">"), gtgt = !strcmp(argv[i], ">>");
    if (!gt && !gtgt) continue;
    if (i == 0 || i + 1 >= argc) { shellError("sh", "syntax error"); return; }
    if (i + 2 != argc) { shellError("sh", "unexpected token after file name"); return; }
    target = argv[i + 1];
    append = gtgt;
    argc = i;
    break;
  }

  const Command* cmd = nullptr;
  for (size_t i = 0; i < kCommandCount; i++)
    if (!strcmp(argv[0], kCommands[i].name)) { cmd = &kCommands[i]; break; }
  if (!cmd) {
    g_term.printf("unknown command: %s\n", argv[0]);
    return;
  }

  File rf;
  if (target) {
    char p[MAX_PATH_LENGTH];
    if (!pathResolve(g_cwd, target, p, sizeof p)) { shellError("sh", "invalid path"); return; }
    if (isProtectedPath(p)) { shellError("sh", "protected system path"); return; }
    if (fsIsDir(p)) { shellError("sh", "is a directory"); return; }
    if (!fsParentIsDir(p)) { shellError("sh", "invalid path"); return; }
    rf = fsOpenWrite(p, append);
    if (!rf) { shellError("sh", "cannot open file"); return; }
    g_out = &rf;
  }

  cmd->handler(argc, argv);

  if (target) { rf.close(); g_out = &g_term; }
}

void shellPoll() {
  while (Serial.available()) {
    int c = Serial.read();

    if (c == '\r' || c == '\n') {
      if (c == '\n' && g_lastCR) { g_lastCR = false; continue; }   // CRLF = uma linha só
      g_lastCR = (c == '\r');
      g_term.print('\n');
      g_line[g_len] = '\0';
      g_len = 0;
      if (g_line[0]) execute(g_line);
      shellPrompt();
      continue;
    }
    g_lastCR = false;

    if (c == 8 || c == 127) {                       // backspace (apaga um caractere UTF-8 inteiro)
      if (g_len == 0) continue;
      do { g_len--; } while (g_len > 0 && ((uint8_t)g_line[g_len] & 0xC0) == 0x80);
      Serial.write((const uint8_t*)"\b \b", 3);
    } else if (c >= 32) {
      if (g_len < MAX_COMMAND_LENGTH - 1) {
        g_line[g_len++] = (char)c;
        Serial.write((uint8_t)c);                   // eco local
      } else {
        Serial.write((uint8_t)7);                   // linha cheia
      }
    }
  }
}
