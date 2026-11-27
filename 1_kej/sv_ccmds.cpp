#include "pch.h"




void cg::SV_Map()
{

	const auto L_CreateCmd = [](const char* cmdname) -> void {
		
		if (Dvar_FindMalleableVar(cmdname)) 
			return;
		
		char buff[128];

		sprintf_s(buff, "seta %s 0\n", cmdname);

		Cbuf_AddText(buff, cgs->clientNum);

	};

	SV_Map_f();

	const dvar_s* sv_running = Dvar_FindMalleableVar("sv_running");

	if (sv_running) {

		if (sv_cmd_args->argc[sv_cmd_args->nesting] < 1)
			return;

		if (!sv_running->current.enabled) {
			Com_PrintError(CON_CHANNEL_CONSOLEONLY, "failed to [%s] because '%s' is missing\n", *(sv_cmd_args->argv[sv_cmd_args->nesting] + 0), *(sv_cmd_args->argv[sv_cmd_args->nesting] + 1));
			return;
		}

		//for restore position
		L_CreateCmd("cjmod_mapname");
		L_CreateCmd("cjmod_X");
		L_CreateCmd("cjmod_Y");
		L_CreateCmd("cjmod_Z");
		L_CreateCmd("cjmod_angleX");
		L_CreateCmd("cjmod_angleY");



		dvar_s* sv_punkbuster = Dvar_FindMalleableVar("sv_punkbuster");
		dvar_s* cl_punkbuster = Dvar_FindMalleableVar("cl_punkbuster");

		if (sv_punkbuster) { sv_punkbuster->current.integer = 0; sv_punkbuster->current.enabled = 0; }
		if (cl_punkbuster) { cl_punkbuster->current.integer = 0; cl_punkbuster->current.enabled = 0; }

		// load features for any local map, no mod required
		Com_Printf(CON_CHANNEL_CONSOLEONLY, "Loading 1_kej features..\n");
		std::cout << "Loading 1_kej features..\n";

		cg::CG_InitHooks();
		Cmd_Init();
		if (!r::R_Init()) {
			//MessageBoxA(NULL, "failed to hook renderer", "FATAL ERROR", MB_ICONERROR);
			fs::Log_Write(LOG_FATAL, "failed to hook renderer\n");
		}
		box500.bounds[0] = 32;
		box500.bounds[1] = 32;
		box500.bounds[2] = 32;


		Com_Printf(CON_CHANNEL_CONSOLEONLY, "^2Done!\n");
	}

	return;
}