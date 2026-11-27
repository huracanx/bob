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

// Adapted from IW3SR, Movements::Bhop (GPL-3.0); see IW3SR_PORT.txt.
void __cdecl cg::CL_FinishMove(usercmd_s* cmd)
{
    CL_FinishMove_f(cmd);
    if (!cmd || !v::mod_bhop.evar || !v::mod_bhop.isEnabled() ||
        r::inputCaptured || clc.demoplaying || !(GetAsyncKeyState(VK_SPACE) & 0x8000))
        return;

    auto& ps = cgs->predictedPlayerState;
    if (ps.pm_type != PM_NORMAL)
        return;

    constexpr int jump = 1 << 10;
    constexpr int sprint = 1 << 1;
    constexpr int stanceButtons = (1 << 8) | (1 << 9) | (1 << 12);
    if (ps.groundEntityNum != ENTITYNUM_NONE)
    {
        if (ps.pm_flags & (PMF_DUCKED | PMF_PRONE))
        {
            clients->stance = StanceState::CL_STANCE_STAND;
            cmd->buttons &= ~stanceButtons;
        }
        if (static_cast<long long>(cmd->serverTime) - ps.jumpTime >= 500)
            cmd->buttons |= jump;
        else
            cmd->buttons &= ~jump;
    }
    else if (!(ps.pm_flags & (PMF_MANTLE | PMF_LADDER)) && !(ps.mantleState.flags & 8))
    {
        cmd->buttons &= ~jump;
        if (!(ps.pm_flags & PMF_SPRINTING))
            cmd->buttons &= ~sprint;
    }
}
