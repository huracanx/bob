#pragma once

#ifndef clmain
#define clmain

#include "pch.h"

namespace cg
{
	inline void(*CL_Disconnect_f)(int);
	void CL_Disconnect(int localClientNum);
	
	inline DWORD CL_PlayDemo_Addr;
	inline void (*CL_PlayDemo_f)();
	void CL_PlayDemo();
	void xCL_PlayDemo(char* name, int isTimeDemo, int a3);
	inline void(*xCL_PlayDemo_f)(char* name, int isTimeDemo, int a3);

	inline bool bhopHookInstalled = false;
    inline bool pmoveHookInstalled = false;
    inline void (__cdecl* CL_FinishMove_f)(usercmd_s*) = nullptr;
    inline DWORD bhopHookAddress = 0;
    inline LONG bhopHookError = ERROR_NOT_FOUND;
    void __cdecl CL_FinishMove(usercmd_s* cmd);
    void InstallBhopHook();
    void RemoveBhopHook();
	inline unsigned long bhopCommandCount = 0;
	inline unsigned long bhopJumpCount = 0;

	int GetKeyBinding(const char* command);
}

#endif