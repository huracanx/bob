#pragma once

#ifndef rmenu
#define rmenu
#include "pch.h"

namespace r
{
	inline bool should_draw_menu;
	inline bool inputCaptured = false;
	inline vec3_t capturedInputAngles{};
	void R_ClearMouseDelta();
	inline vec3_t angles_before_menu;

	void IN_ActiveMouse(int active);
	void R_RemoveInput(bool _true, bool save_file = true);
	bool R_OpenMenu(IDirect3DDevice9* pDevice);
	void R_EndRender();
	void R_MenuStyle(float transparencyScale = 1.f);

	void R_Features();

	void R_DrawMenuByName(const char* category, const bool JustClicked);

	void R_ConfigsTab();


	void R_Automation_Features();

	

}

#endif