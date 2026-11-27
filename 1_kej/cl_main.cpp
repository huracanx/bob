#include "pch.h"

void cg::CL_Disconnect(int localClientNum)
{

	if (clientUI[0].connectionState != CA_DISCONNECTED) { //gets called in the loading screen in 1.7
		Com_Printf(CON_CHANNEL_CONSOLEONLY, "removing hooks because player disconnected\n");
		Cmd_RemoveCommand("1_kej_openmenu");
		CG_RemoveHooks();
		cworld.terrain.clip_points.clear();
		cworld.terrain.clip_points.resize(0);
		r::ResetImFont = true;



		glob_pm = nullptr;
		glob_pml = nullptr;
	}


	return CL_Disconnect_f(localClientNum);
}
void cg::CL_PlayDemo()
{
	CL_PlayDemo_f();

	if (clc.demoplaying != NULL) {

		Com_Printf(CON_CHANNEL_CONSOLEONLY, "Loading 1_kej features for a demo..\n");
		std::cout << "Loading 1_kej features for a demo..\n";

		cg::CG_InitHooks();

	}

}
void cg::xCL_PlayDemo(char* name, int isTimeDemo, int a3)
{
	xCL_PlayDemo_f(name, isTimeDemo, a3);

	if (clc.demoplaying != NULL) {

		Com_Printf(CON_CHANNEL_CONSOLEONLY, "Loading 1_kej features for a demo..\n");
		std::cout << "Loading 1_kej features for a demo..\n";

		cg::CG_InitHooks();

	}
}

int cg::GetKeyBinding(const char* command)
{
	unsigned int keyIndex; // ebx
	const char** v1; // esi
	char* key; // eax
	const char* action; // [esp-4h] [ebp-10h]

	keyIndex = 0;
	v1 = (const char**)0x8F1DCC;

	do {
		if (*v1)
		{
			if (**v1)
			{
				char* keystr;
				__asm
				{
					mov eax, keyIndex;
					xor edi, edi;
					mov esi, 0x4677C0;
					call esi;
					mov keystr, eax;
				}
				action = *v1;

				if (!strcmp(command, action))
					return keyIndex;

				//Com_Printf(CON_CHANNEL_OBITUARY, "%s \"%s\"\n", keystr, action);
			}
		}
		v1 += 3;
		++keyIndex;

	} while ((DWORD)v1 < (DWORD)historyEditLines);

	return 0;
}


// IW3SR Movements::Bhop (GPL-3.0), adapted for the outgoing command.
// Never modify a prediction-only pmove command or the player's physics state.
void __cdecl cg::CL_FinishMove(usercmd_s* cmd)
{
    CL_FinishMove_f(cmd);
    ++bhopCommandCount;
    if (cmd && !r::inputCaptured && !clc.demoplaying && v::mod_strafebot.evar) {
        const auto& state = cgs->predictedPlayerState;
        const bool airborne = state.pm_type == PM_NORMAL && state.groundEntityNum == ENTITYNUM_NONE &&
            !(state.pm_flags & (PMF_MANTLE | PMF_LADDER | PMF_DUCKED | PMF_PRONE));
        const int side = Mod_A_AutoStrafeSide(airborne);
        if (side && cmd->forwardmove > 0 && cmd->rightmove == 0)
            cmd->rightmove = static_cast<char>(side * 127);
    }
    if (!cmd || !v::mod_bhop.evar || !v::mod_bhop.isEnabled() ||
        r::inputCaptured || clc.demoplaying || !(GetAsyncKeyState(VK_SPACE) & 0x8000))
        return;
    const auto& ps = cgs->predictedPlayerState;
    if (ps.pm_type != PM_NORMAL || (ps.pm_flags & (PMF_MANTLE | PMF_LADDER | PMF_DUCKED | PMF_PRONE)) ||
        (ps.mantleState.flags & 8))
        return;
    constexpr int jump = 1 << 10;
    if (ps.groundEntityNum != ENTITYNUM_NONE &&
        static_cast<long long>(cmd->serverTime) - ps.jumpTime >= 500)
    {
        cmd->buttons |= jump;
        ++bhopJumpCount;
    }
    else
        cmd->buttons &= ~jump;
}

static bool IsExecutableEntry(DWORD addr)
{
    MEMORY_BASIC_INFORMATION info{};
    if (!addr || !VirtualQuery(reinterpret_cast<void*>(addr), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD))
        return false;
    const DWORD prot = info.Protect & 0xff;
    if (prot != PAGE_EXECUTE_READ && prot != PAGE_EXECUTE_READWRITE && prot != PAGE_EXECUTE_WRITECOPY)
        return false;
    const BYTE first = *reinterpret_cast<const BYTE*>(addr);
    // Reject empty, breakpoint and immediate-return entries.
    return first != 0 && first != 0xcc && first != 0xc3 && first != 0xc2;
}

void cg::InstallBhopHook()
{
    if (bhopHookInstalled) return;
    bhopCommandCount = bhopJumpCount = 0;
    bhopHookAddress = 0;
    bhopHookError = ERROR_NOT_FOUND;
    hook h;
    // The uploaded IW3SR supports CoD4X >=21.3. Its DLL signature may not
    // exist in 21.1. Try the documented original IW3 entry as a fallback.
    const DWORD candidate = GetModuleHandleA("cod4x_021.dll")
        ? static_cast<DWORD>(h.find_pattern("cod4x_021.dll", "?? ?? ?? ?? ?? 15 ?? ?? ?? ?? 8B 44 24 10 88 50 14 8B 15")) : 0;
    const DWORD addresses[] = { candidate, 0x463A60 };
    for (DWORD addr : addresses)
    {
        if (!IsExecutableEntry(addr)) continue;
        CL_FinishMove_f = reinterpret_cast<decltype(CL_FinishMove_f)>(addr);
        bhopHookError = h.install(reinterpret_cast<PVOID*>(&CL_FinishMove_f), CL_FinishMove);
        if (bhopHookError == NO_ERROR)
        {
            bhopHookAddress = addr;
            bhopHookInstalled = true;
            break;
        }
        CL_FinishMove_f = nullptr;
    }
}

void cg::RemoveBhopHook()
{
    if (!bhopHookInstalled) return;
    hook h;
    if (h.remove(reinterpret_cast<PVOID*>(&CL_FinishMove_f), CL_FinishMove) == NO_ERROR)
    {
        bhopHookInstalled = false;
        CL_FinishMove_f = nullptr;
        bhopHookAddress = 0;
    }
}
