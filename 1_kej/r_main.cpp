#include "pch.h"


XAssetHeader __cdecl Material_Register_FastFile(const char* a1)
{
	return ((XAssetHeader(*)(const char* a1))0x489570)(a1);
}


HRESULT __stdcall r::draw_func(IDirect3DDevice9* pDevice)
{
	r::device_needs_reset = false;

	if (mglobs.isUsing) {
		if (!R_ImGui(pDevice)) {
			fs::Log_Write(LOG_FATAL, "Unable to initialize imgui\n");
		}
		if (R_OpenMenu(pDevice)) {

			Mod_DrawVelocityDirection();
            Mod_DrawHitVisualizer();
			if (!clc.demoplaying) {

				Mod_DrawWorldAxes();

				

				

				Mod_DrawFPSHelpers();
				Mod_A_500FPS();
			}
			//CMod_OnHighlightLeafBrushNode();
			R_EndRender();
		}
		
	}

	return pEndScene(pDevice);
}

// Publish the actual local FPS cap using CoD4X's normal UserCVAR path.
// Keep telemetry separate from com_maxfps and never change movement or server FPS.
static void PublishClientFPSLimit()
{
    static int lastQueuedLimit = -1;
    static ULONGLONG lastQueuedAt = 0;
    if (clientUI[0].connectionState != CA_ACTIVE || clc.demoplaying)
        return;

    const auto* limit = Dvar_FindMalleableVar("com_maxfps");
    if (!limit || limit->type != dvar_type::integer)
        return;
    const int value = limit->current.integer;
    if (value < 0)
        return;

    // Check the actual UserCVAR as well, in case a config reset removed it.
    const auto* published = Dvar_FindMalleableVar("client_fps_limit");
    char valueText[24]{};
    sprintf_s(valueText, sizeof(valueText), "%d", value);
    const bool matches = published && (published->flags & dvar_flags::user_info) &&
        ((published->type == dvar_type::string && published->current.string &&
          strcmp(published->current.string, valueText) == 0) ||
         (published->type == dvar_type::integer && published->current.integer == value));
    if (matches)
        return;

    // Coalesce rapid zone changes rather than flood the reliable command queue.
    const ULONGLONG now = GetTickCount64();
    if (lastQueuedLimit >= 0 && now - lastQueuedAt < 1000)
        return;

    char command[64]{};
    sprintf_s(command, sizeof(command), "setu client_fps_limit %d\n", value);
    Cbuf_AddText(command, 0);
    lastQueuedLimit = value;
    lastQueuedAt = now;
}

void __cdecl r::CG_DrawActive()
{
    if (r::inputCaptured) {
        r::R_ClearMouseDelta();
        VectorCopy(r::capturedInputAngles, clients->viewangles);
    }
	if (mglobs.isUsing) {
		Mod_DrawCoordinates();
		/*Mod_DrawFPSHelpers();*/
		
		Mod_DrawCurveSpeed();

		Mod_DrawEvents();
		Mod_BounceCalculator();


		if (!clc.demoplaying) {
			Mod_DrawAngleHelper();
			Mod_A_Strafebot();
			Mod_A_AutoFPS();
            PublishClientFPSLimit();
			Mod_DisallowHalfbeat();
		}
		
        Mod_DrawVelocity();
        Mod_DrawFPSLimit();
		if (vangle_que.yes && !r::inputCaptured) {

			CG_SetPlayerAngles(vangle_que.src, vangle_que.dst);
			vangle_que.yes = false;
		}
		//if(jumpanalyzer.hugging_bounce && !jumpanalyzer.walking)
		//char buff[48];
		//sprintf_s(buff, "airTime: %i\ngroundTime: %i", jumpanalyzer.serverTime - jumpanalyzer.airTime, jumpanalyzer.serverTime - jumpanalyzer.groundTime);
		//r::R_AddCmdDrawTextWithEffects(buff, "fonts/objectivefont", r::X(300), r::Y(300), v::mod_velometer.GetArray(3), v::mod_velometer.GetArray(3), 0.f, vec4_t{255,255,0,255}, 3, v::mod_velometer_glow.evar->vecValue, 0, 0, 0, 500, 1000, 2000);
		//
		//float delta = AngleDelta(cgs->compassNorthYaw, cgs->refdefViewAngles[1]);
		//
		//static float incr = 0;
		//ScreenPlacement* scrPlace = CG_GetScreenPlacement(cgs->clientNum);
		//CG_DrawRotatedPic(1, 1, scrPlace, 100, 100, 100, 100, delta, vec4_t{ 0,255,0,170 }, "white");

		
	
	}



	return CG_DrawActive_f();
}