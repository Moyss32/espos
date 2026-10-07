#pragma once
#include <stddef.h>

// Árvore estrutural. Índice 0 é "/". Usada por fsCreateBaseTree e isProtectedPath.
extern const char* const kBaseDirs[];
extern const size_t kBaseDirCount;

// Resolve `in` (absoluto ou relativo a `cwd`) para um caminho absoluto normalizado.
// Trata ".", ".." e "//"; ".." nunca sai da raiz. cwd pode ser nullptr (= "/").
bool pathResolve(const char* cwd, const char* in, char* out, size_t outSize);

bool pathJoin(const char* dir, const char* name, char* out, size_t outSize);
const char* pathBase(const char* path);                   // último componente
bool pathIsInside(const char* path, const char* dir);     // path == dir ou dentro dele

// Única fonte de verdade da proteção. Normaliza antes de comparar.
bool isProtectedPath(const char* path);
