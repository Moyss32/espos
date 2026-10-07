#pragma once
#include <stddef.h>

#define MINIOS_NAME    "MiniOS ESP32"
#define MINIOS_VERSION "1.0"

constexpr unsigned long SERIAL_BAUD = 115200;
constexpr size_t MAX_COMMAND_LENGTH = 256;  // linha digitada (com NUL)
constexpr size_t MAX_ARGUMENTS      = 16;
constexpr size_t MAX_PATH_LENGTH    = 256;  // caminho normalizado (com NUL)
constexpr size_t MAX_NAME_LENGTH    = 64;   // nome de entrada ao listar/remover
constexpr int    MAX_TREE_DEPTH     = 8;    // limite de recursão (tree, rm -r)
