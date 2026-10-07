#pragma once
#include <Arduino.h>

void shellPrompt();
void shellPoll();                       // chamar no loop(); não bloqueia
Print& shellOut();                      // saída do comando (terminal ou arquivo redirecionado)
void shellError(const char* cmd, const char* msg);   // sempre vai ao terminal
const char* shellCwd();
void shellSetCwd(const char* path);
