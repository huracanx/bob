#pragma once

#ifndef gamefuncs
#define gamefuncs

#include "pch.h"

void iPrintLn(const char* text);
void iPrintLnBold(const char* text);
void SendCommand(const char* command);

cg::cmd_function_s* Cmd_FindCommand(const char* name);
cg::cmd_function_s* Cmd_AddCommand(char* cmdname, void(__cdecl* function)());
cg::cmd_function_s* Cmd_RemoveCommand(const char* cmdname);

void Cbuf_AddText(const char* text, int localClientNum);


void OpenMenu_f();

void Cmd_Init();
#endif