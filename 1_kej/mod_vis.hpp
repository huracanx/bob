#pragma once

#ifndef modvis
#define modvis

#include "pch.h"

namespace cg
{
	struct fps_s
	{
		float DistanceToTransferZone;
	};

	void Mod_DrawEvents();
    void Mod_ResetHitVisualizer();
    void Mod_RecordJumpStep(pmove_t* pm, int previousGround, const float* previousOrigin);
    void Mod_RecordBounceContact(pmove_t* pm, int commandTime, const float* origin);
    void Mod_DrawHitVisualizer();

	void Mod_DrawVelocity();
	void Mod_DrawFPSLimit();
	void Mod_DrawCoordinates();
	void Mod_DrawVelocityDirection();
	void Mod_DrawWorldAxes();
	void FPS_CalculateSingleBeatDirection(bool& rightmove, const usercmd_s* cmd);
	void Mod_DrawFPSHelpers();
	void Mod_GetAccelerationAngles(const bool rightmove, vec2_t accel_angles);
	void Mod_DrawAngleHelper();
	void Mod_DrawCurveSpeed();


	inline fps_s mod_fps;
}

#endif