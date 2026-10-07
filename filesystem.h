#pragma once
#include <Arduino.h>
#include <FS.h>

bool fsMount();
int  fsCreateBaseTree();                 // retorna quantos itens foram criados

bool fsExists(const char* path);
bool fsIsDir(const char* path);
bool fsParentIsDir(const char* path);
bool fsDirIsEmpty(const char* path);

// Percorre um diretório. cb retorna false para parar. Retorna false se não abriu.
typedef bool (*FsVisitor)(const char* name, bool isDir, void* ctx);
bool fsEach(const char* dir, FsVisitor cb, void* ctx);

bool fsMkdir(const char* path);
bool fsCreateFile(const char* path);     // cria vazio
bool fsRemove(const char* path, bool recursive);   // recusa caminhos protegidos
bool fsRename(const char* from, const char* to);   // recusa caminhos protegidos
bool fsCopy(const char* from, const char* to);     // recusa destino protegido
bool fsStream(const char* path, Print& out);       // lê em blocos de 64 bytes
File fsOpenWrite(const char* path, bool append);

size_t fsTotalBytes();
size_t fsUsedBytes();
