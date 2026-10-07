#pragma once
#include <stddef.h>

struct Command {
  const char* name;
  const char* help;                     // nulo = não aparece no help
  int (*handler)(int argc, char** argv);
};

extern const Command kCommands[];
extern const size_t kCommandCount;
