#pragma once
#include <Arduino.h>

void sysReboot();
void sysUname(Print& out, bool full);
void sysFree(Print& out);
void sysUptime(Print& out);
uint32_t sysRamKB();
