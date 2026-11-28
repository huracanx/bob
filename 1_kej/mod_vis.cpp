#include "pch.h"

void cg::Mod_DrawEvents()
{
	
	if (automation.pendingSlide) {

		Material* fxMaterial = r::R_RegisterMaterial("decode_characters");
		Material* fxMaterialGlow = r::R_RegisterMaterial("decode_characters_glow");

		r::R_AddCmdDrawTextWithEffects((char*)"Slide", "fonts/objectivefont", r::X(v::mod_velometer.GetArray(1)), r::Y(v::mod_velometer.GetArray(2) - 40) , v::mod_velometer.GetArray(3), v::mod_velometer.GetArray(3), 0.f, vec4_t{ 0,1,0,1 }, 3, v::mod_velometer_glow.evar->vecValue, fxMaterial, fxMaterialGlow, 0, 500, 1000, 2000);


		//r::R_DrawText("Slide", v::mod_velometer.GetArray(1), v::mod_velometer.GetArray(2) - 20 * v::mod_velometer.GetArray(3), v::mod_velometer.GetArray(3), v::mod_velometer.GetArray(3), 0, vec4_t{ 0,1,0,1 }, 0);
	}
}
void cg::Mod_DrawVelocity()
{
	if (!v::mod_velometer.isEnabled())
		return;


	int32_t velocity;

	velocity = (int32_t)glm::length(glm::vec2(cg::clients->cgameVelocity[0], cg::clients->cgameVelocity[1]));

	static int32_t velocity_prev_frame = velocity;

	static DWORD ms = Sys_MilliSeconds();
	const DWORD timePassed = Sys_MilliSeconds() - ms;

	static glm::vec4 col{};

	if (timePassed > (1000.f / cgs->frametime) / 10) {
		ms = Sys_MilliSeconds();
		col.a = 255;
		if (velocity > velocity_prev_frame) {
			col.r = 0;
			col.g = 255;
			col.b = 0;
		}
		else if (velocity < velocity_prev_frame) {
			col.r = 255;
			col.g = 0;
			col.b = 0;
		}
		else {
			col.r = 255;
			col.g = 255;
			col.b = 0;
		}

		velocity_prev_frame = velocity;

	}

	char buffer[20];
	sprintf_s(buffer, "%i", velocity);

	buffer[19] = '\0';
	//r::R_DrawText(buffer, "fonts/normalfont", v::mod_velometer.evar->arrayValue[1], v::mod_velometer.evar->arrayValue[2], v::mod_velometer.evar->arrayValue[3], v::mod_velometer.evar->arrayValue[3], 0, (float*)&col, 1);
	
	Material* fxMaterial = r::R_RegisterMaterial("decode_characters");
	Material* fxMaterialGlow = r::R_RegisterMaterial("decode_characters_glow");

	r::R_AddCmdDrawTextWithEffects(buffer, "fonts/objectivefont", r::X(v::mod_velometer.GetArray(1)), r::Y(v::mod_velometer.GetArray(2)), v::mod_velometer.GetArray(3), v::mod_velometer.GetArray(3), 0.f, (float*)&col, 3, v::mod_velometer_glow.evar->vecValue, fxMaterial, fxMaterialGlow, 0, 500, 1000, 2000);


}
void cg::Mod_DrawFPSLimit()
{
    if (!v::mod_showFPS.isEnabled())
        return;

    const auto* fps = Dvar_FindMalleableVar("com_maxfps");
    char buffer[32];
    if (!fps) return;
    sprintf_s(buffer, "%i", fps->current.integer);
    const float x = v::mod_showFPS_layout.GetArray(0);
    const float y = v::mod_showFPS_layout.GetArray(1);
    const float scale = v::mod_showFPS_layout.GetArray(2);
    auto* material = r::R_RegisterMaterial("decode_characters");
    auto* glow = r::R_RegisterMaterial("decode_characters_glow");
    r::R_AddCmdDrawTextWithEffects(buffer, "fonts/objectivefont", r::X(x), r::Y(y),
        scale, scale, 0.f, v::mod_showFPS_color.evar->vecValue, 3,
        v::mod_showFPS_glow.evar->vecValue, material, glow, 0, 500, 1000, 2000);
}

void cg::Mod_DrawCoordinates()
{
	if (!v::mod_coordinates.isEnabled())
		return;

	char buffer[128];
	static glm::vec4 col{};

	sprintf_s(buffer, "x:     %.6f\ny:     %.6f\nz:     %.6f\nyaw: %.6f", clients->cgameOrigin[0], clients->cgameOrigin[1], clients->cgameOrigin[2], cgs->refdefViewAngles[YAW]);

	col.r = 0;
	col.g = 255;
	col.b = 0;
	col.w = 255;

	if (clients->cgameOrigin[0] == (int)clients->cgameOrigin[0] || clients->cgameOrigin[1] == (int)clients->cgameOrigin[1])
		col.b = 255;

	r::R_AddCmdDrawTextWithEffects(buffer, "fonts/normalfont", r::X(v::mod_coordinates.GetArray(1)), r::Y(v::mod_coordinates.GetArray(2)), v::mod_coordinates.GetArray(3), v::mod_coordinates.GetArray(3), 0.f, vec4_t{0,1,0,1}, 3, vec4_t{1,1,1,0}, 0, 0, 0, 0, 0, 0);


	//r::R_DrawText(buffer, v::mod_coordinates.evar->arrayValue[1], v::mod_coordinates.evar->arrayValue[2], v::mod_coordinates.evar->arrayValue[3], v::mod_coordinates.evar->arrayValue[3], 0, (float*)&col, 0);

}
//#define BAR_START_Y 490
//#define BAR_HEIGHT 10
void cg::Mod_DrawFPSHelpers()
{
	constexpr static float length_marker250 = 1.f;

	static bool rightmove{};

	const usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);


	if (cmd->rightmove > 0)
		rightmove = true;
	else if (cmd->rightmove < 0)
		rightmove = false;


	FPS_CalculateSingleBeatDirection(rightmove, cmd);

	float yaw = clients->cgameViewangles[YAW] > 0 ? clients->cgameViewangles[YAW] : 180.f - clients->cgameViewangles[YAW] * -1; //mirror the yaw
	float aa = atan2(-(int)cmd->rightmove, (int)cmd->forwardmove) * 57.2957795f;
	const dvar_s* _fov = Dvar_FindMalleableVar("cg_fov");
	const dvar_s* fovscale = Dvar_FindMalleableVar("cg_fovscale");

	const float fov = _fov->current.value * fovscale->current.value * v::mod_fps_transferz.evar->arrayValue[3];
	bool isInverted = clients->cgameViewangles[YAW] < fov || clients->cgameViewangles[YAW] > 180.f - fov;

	

	if (automation.paraActive && cmd->forwardmove > 0 && cmd->rightmove == 0) {
        rightmove = automation.paraSide > 0;
        aa = 0.0f;
    } else
        aa > 0 ? aa -= 45 : aa += 45;

	static const float m250 = 86.f;
	vec2_t marker250 = { m250, 90.f + m250 };
	vec2_t zone125 = { fps_zones.fps125, 90.f + fps_zones.fps125 };
	vec2_t zone200 = { fps_zones.fps200, 90.f + fps_zones.fps200 };
	vec2_t zone250 = { fps_zones.fps250, 90.f + fps_zones.fps250 };
	vec2_t zone333 = { fps_zones.fps333, 90.f + fps_zones.fps333 };

	if (rightmove) {
		marker250[0] = 180.f - marker250[0] - length_marker250;
		marker250[1] = 180.f - marker250[1] - length_marker250;
		zone125[0] = 180.f - zone125[0] - fps_zones.length125;
		zone125[1] = 180.f - zone125[1] - fps_zones.length125;
		zone200[0] = 180.f - zone200[0] - fps_zones.length200;
		zone200[1] = 180.f - zone200[1] - fps_zones.length200;
		zone250[0] = 180.f - zone250[0] - fps_zones.length250;
		zone250[1] = 180.f - zone250[1] - fps_zones.length250;
		//yaw -= 10;
	}

	yaw = AngleNormalize180(yaw += aa);
	if (yaw < 0)
		yaw = 180.f - fabs(yaw);


	
	//r::R_DrawRect("white", 0, BAR_START_Y, 1920, 10, vec4_t{ 1,1,1,100 });

	const float BAR_START_Y = v::mod_fps_transferz.evar->arrayValue[1];
	const float BAR_HEIGHT = v::mod_fps_transferz.evar->arrayValue[2];



	float &DistanceFromZone = mod_fps.DistanceToTransferZone;
	const bool isLong125 = v::mod_autoFPS_long125.isEnabled() && !Mod_A_RestrictedFPS();

	const bool cylindrical = v::mod_FPS_cylindrical.isEnabled();

	if (isInverted) {
		if (v::mod_fps_transferz.evar->arrayValue[0] != NULL) {

			if (!isLong125) {
				CG_FillAngleYaw(-180.f + zone125[0], -180.f + (zone125[0] + fps_zones.length125), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_125col.evar->vecValue, true, cylindrical);
				CG_FillAngleYaw(-180.f + zone125[1], -180.f + (zone125[1] + fps_zones.length125), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_125col.evar->vecValue, true, cylindrical);
			}
			else {
				CG_FillAngleYaw(-180.f + zone333[0], -180.f + (zone333[0] + fps_zones.length333), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_333col.evar->vecValue, true, cylindrical);
				CG_FillAngleYaw(-180.f + zone333[1], -180.f + (zone333[1] + fps_zones.length333), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_333col.evar->vecValue, true, cylindrical);
			}

			CG_FillAngleYaw(-180.f + zone250[0], -180.f + (zone250[0] + fps_zones.length250), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_250col.evar->vecValue, true, cylindrical);
			CG_FillAngleYaw(-180.f + zone250[1], -180.f + (zone250[1] + fps_zones.length250), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_250col.evar->vecValue, true, cylindrical);

			CG_FillAngleYaw(-180.f + zone200[0], -180.f + (zone200[0] + fps_zones.length200), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_200col.evar->vecValue, true, cylindrical);
			CG_FillAngleYaw(-180.f + zone200[1], -180.f + (zone200[1] + fps_zones.length200), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_200col.evar->vecValue, true, cylindrical);

			if (!isLong125) {
				CG_FillAngleYaw(-180.f + zone333[0], -180.f + (zone333[0] + fps_zones.length333), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_333col.evar->vecValue, true, cylindrical);
				CG_FillAngleYaw(-180.f + zone333[1], -180.f + (zone333[1] + fps_zones.length333), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_333col.evar->vecValue, true, cylindrical);
			}
			else {
				CG_FillAngleYaw(-180.f + zone125[0], -180.f + (zone125[0] + fps_zones.length125), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_125col.evar->vecValue, true, cylindrical);
				CG_FillAngleYaw(-180.f + zone125[1], -180.f + (zone125[1] + fps_zones.length125), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_125col.evar->vecValue, true, cylindrical);
			}

			CG_FillAngleYaw(-180.f + marker250[0], -180.f + (marker250[0] + length_marker250), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_markercol.evar->vecValue, true, cylindrical);
			CG_FillAngleYaw(-180.f + marker250[1], -180.f + (marker250[1] + length_marker250), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_markercol.evar->vecValue, true, cylindrical);

		}
		DistanceFromZone = (rightmove == false) ? yaw - (-180.f + marker250[0]) : yaw - (-180.f + (marker250[0] + length_marker250));

	} else {

		DistanceFromZone = (rightmove == false) ? yaw - marker250[0] : yaw - (marker250[0] + length_marker250);

	}
	if (DistanceFromZone < 0.f)
		DistanceFromZone = 90.f + DistanceFromZone;


	if (v::mod_fps_transferz.evar->arrayValue[0] != NULL) {
		if (!isLong125) {
			CG_FillAngleYaw(zone125[0], (zone125[0] + fps_zones.length125), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_125col.evar->vecValue, true, cylindrical);
			CG_FillAngleYaw(zone125[1], (zone125[1] + fps_zones.length125), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_125col.evar->vecValue, true, cylindrical);
		}
		else {
			CG_FillAngleYaw(zone333[0], (zone333[0] + fps_zones.length333), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_333col.evar->vecValue, true, cylindrical);
			CG_FillAngleYaw(zone333[1], (zone333[1] + fps_zones.length333), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_333col.evar->vecValue, true, cylindrical);

		}

		CG_FillAngleYaw(zone250[0], (zone250[0] + fps_zones.length250), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_250col.evar->vecValue, true, cylindrical);
		CG_FillAngleYaw(zone250[1], (zone250[1] + fps_zones.length250), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_250col.evar->vecValue, true, cylindrical);

		CG_FillAngleYaw(zone200[0], (zone200[0] + fps_zones.length200), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_200col.evar->vecValue, true, cylindrical);
		CG_FillAngleYaw(zone200[1], (zone200[1] + fps_zones.length200), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_200col.evar->vecValue, true, cylindrical);

		if (!isLong125) {
			CG_FillAngleYaw(zone333[0], (zone333[0] + fps_zones.length333), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_333col.evar->vecValue, true, cylindrical);
			CG_FillAngleYaw(zone333[1], (zone333[1] + fps_zones.length333), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_333col.evar->vecValue, true, cylindrical);
		}
		else {
			CG_FillAngleYaw(zone125[0], (zone125[0] + fps_zones.length125), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_125col.evar->vecValue, true, cylindrical);
			CG_FillAngleYaw(zone125[1], (zone125[1] + fps_zones.length125), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_125col.evar->vecValue, true, cylindrical);
		}
	
		CG_FillAngleYaw(marker250[0], (marker250[0] + length_marker250), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_markercol.evar->vecValue, true, cylindrical);
		CG_FillAngleYaw(marker250[1], (marker250[1] + length_marker250), yaw, BAR_START_Y, BAR_HEIGHT, fov, v::mod_markercol.evar->vecValue, true, cylindrical);
	}

	DistanceFromZone = fmodf(DistanceFromZone, 90);

	if (!rightmove)
		DistanceFromZone = 90.f - DistanceFromZone;


	if (v::mod_fps_transferz.evar->arrayValue[0] != NULL)
		//CG_FillAngleYaw(yaw-1, yaw+1, yaw, BAR_START_Y, BAR_HEIGHT, fov, vec4_t{ 255,255,255,255 }, true, true);
		ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(r::X(958), r::Y(BAR_START_Y - 20)), ImVec2(r::X(962), r::Y(BAR_HEIGHT + BAR_START_Y + 20)), ImColor(1.f, 1.f, 1.f, 1.f));
		//r::R_DrawRect("white", 958, BAR_START_Y - 10, 4, BAR_HEIGHT + 20, vec4_t{ 1,1,1,255 });


    if (Mod_ExtraFPSZonesActive()) {
        // Render the same optional selector as AutoFPS, in the helper's
        // folded 90-degree phase coordinates. Half-degree sampling is a
        // display approximation; the actual selector is continuous.
        if (v::mod_fps_transferz.evar->arrayValue[0] != 0) {
            const vec4_t color166 = {0.35f, 0.75f, 1.0f, 1.0f};
            const vec4_t color142 = {0.85f, 0.40f, 1.0f, 1.0f};
            int previous = 0;
            float begin = 0.0f;
            for (int i = 0; i <= 180; ++i) {
                const float phase = i * 0.5f;
                const int nextFPS = i == 180 ? 0 : Mod_FPSForPhase(phase + 0.25f, rightmove, fov, true);
                if (nextFPS == previous) continue;
                if (previous == 166 || previous == 142) {
                    for (int copy = -2; copy <= 1; ++copy) {
                        const float offset = copy * 90.0f;
                        CG_FillAngleYaw(begin + offset, phase + offset, yaw, BAR_START_Y,
                            BAR_HEIGHT, fov, previous == 166 ? color166 : color142, true, cylindrical);
                    }
                }
                previous = nextFPS;
                begin = phase;
            }
        }
        jumpanalyzer.recommendedFPS = Mod_FPSForPhase(yaw, rightmove, fov, true);
        return;
    }

	range_t ranges, ranges2;

	if (!isLong125) {
		ranges = AnglesToRange(DEG2RAD(zone333[0]), DEG2RAD(zone333[0] + fps_zones.length333), DEG2RAD(fmodf(yaw, 90)), fov);
		ranges2 = AnglesToRange(DEG2RAD(zone333[1]), DEG2RAD(zone333[1] + fps_zones.length333), DEG2RAD(fmodf(yaw, 90)), fov);
	}
	else {
		ranges = AnglesToRange(DEG2RAD(zone125[0]), DEG2RAD(zone125[0] + fps_zones.length125), DEG2RAD(fmodf(yaw, 90)), fov);
		ranges2 = AnglesToRange(DEG2RAD(zone125[1]), DEG2RAD(zone125[1] + fps_zones.length125), DEG2RAD(fmodf(yaw, 90)), fov);
	}

	if (ranges.x1 <= 960 && ranges.x2 > 960 || ranges2.x1 <= 960 && ranges2.x2 > 960) {
		jumpanalyzer.recommendedFPS = isLong125 == false ? 333 : 125;
		return;
	}

	if (!isLong125) {
		ranges = AnglesToRange(DEG2RAD(zone125[0]), DEG2RAD(zone125[0] + fps_zones.length125), DEG2RAD(fmodf(yaw, 90)), fov);
		ranges2 = AnglesToRange(DEG2RAD(zone125[1]), DEG2RAD(zone125[1] + fps_zones.length125), DEG2RAD(fmodf(yaw, 90)), fov);
	}
	else {
		ranges = AnglesToRange(DEG2RAD(zone333[0]), DEG2RAD(zone333[0] + fps_zones.length333), DEG2RAD(fmodf(yaw, 90)), fov);
		ranges2 = AnglesToRange(DEG2RAD(zone333[1]), DEG2RAD(zone333[1] + fps_zones.length333), DEG2RAD(fmodf(yaw, 90)), fov);
	}

	if (ranges.x1 <= 960 && ranges.x2 > 960 || ranges2.x1 <= 960 && ranges2.x2 > 960) {
		jumpanalyzer.recommendedFPS = isLong125 == false ? 125 : 333;
		return;
	}

	ranges = AnglesToRange(DEG2RAD(zone200[0]), DEG2RAD(zone200[0] + fps_zones.length200), DEG2RAD(fmodf(yaw, 90)), fov);
	ranges2 = AnglesToRange(DEG2RAD(zone200[1]), DEG2RAD(zone200[1] + fps_zones.length200), DEG2RAD(fmodf(yaw, 90)), fov);

	if (ranges.x1 <= 960 && ranges.x2 > 960 || ranges2.x1 <= 960 && ranges2.x2 > 960) {
		jumpanalyzer.recommendedFPS = 200;
		return;
	}

	ranges = AnglesToRange(DEG2RAD(zone250[0]), DEG2RAD(zone250[0] + fps_zones.length250), DEG2RAD(fmodf(yaw, 90)), fov);
	ranges2 = AnglesToRange(DEG2RAD(zone250[1]), DEG2RAD(zone250[1] + fps_zones.length250), DEG2RAD(fmodf(yaw, 90)), fov);

	if (ranges.x1 <= 960 && ranges.x2 > 960 || ranges2.x1 <= 960 && ranges2.x2 > 960) {
		jumpanalyzer.recommendedFPS = 250;
		return;
	}
	

}
void cg::FPS_CalculateSingleBeatDirection(bool& rightmove, const usercmd_s* cmd)
{
	static uint16_t frames = NULL;
	static vec_t oldYaw = NULL;
	static bool _rightmove;
	const vec_t newYaw = cgs->refdefViewAngles[YAW];

	frames += 1;

	if (frames >= 4) {
		frames = 0;

		if (oldYaw < newYaw && cmd->forwardmove != 0 && cmd->rightmove == 0 && rightmove)
			_rightmove = false;
		else if (oldYaw > newYaw && cmd->forwardmove != 0 && cmd->rightmove == 0 && !rightmove)
			_rightmove = true;
		oldYaw = newYaw;
	}
	if(cmd->rightmove == 0)
		rightmove = _rightmove;
}
void cg::Mod_DrawVelocityDirection()
{
	if (!v::mod_veldirection.isEnabled() || NOT_SERVER)
		return;


	vec3_t orgPos;
	VectorCopy(clients->cgameOrigin, orgPos);
	float velAngle = atan2(clients->cgameVelocity[1], clients->cgameVelocity[0]) * 180.f / PI;

	vec3_t angles = { 0, velAngle, 0 }, end;
	AnglesToForward(angles, orgPos, 100, end);
	vec2_t self_xy, end_xy;
	if (r::WorldToScreen(orgPos, self_xy) && r::WorldToScreen(end, end_xy)) {
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(end_xy[0], end_xy[1]), ImVec2(self_xy[0], self_xy[1]), IM_COL32(255, 255, 0, 255), 1.5f);
	}
}
#define DIRECTION_LENGTH 2000
void cg::Mod_DrawWorldAxes()
{
	if (!v::mod_show_worldaxes.isEnabled() || NOT_SERVER)
		return;

	vec3_t forwardStart, start;

	vec3_t orgPos;

	VectorCopy(clients->cgameOrigin, orgPos);

	VectorCopy(orgPos, forwardStart);
	VectorCopy(orgPos, start);

	forwardStart[0] -= DIRECTION_LENGTH;

	const float pitch = clients->cgameViewangles[PITCH];

	start[2] -= 1000;
	forwardStart[2] -= 1000;


	if (pitch < 0) {
		start[0] -= fabs(pitch) * 50;
	}

	vec2_t self_xy, end_xy, nearest_xy;
	if (r::WorldToScreen(forwardStart, self_xy) && r::WorldToScreen(start, end_xy)) {
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(end_xy[0], end_xy[1]), ImVec2(self_xy[0], self_xy[1]), IM_COL32(255, 0, 0, 255), 2.f);
	}
	start[0] = orgPos[0];
	if (pitch < 0) {
		start[0] += fabs(pitch) * 50;
	}
	forwardStart[0] += DIRECTION_LENGTH * 2;
	if (r::WorldToScreen(forwardStart, self_xy) && r::WorldToScreen(start, end_xy)) {
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(end_xy[0], end_xy[1]), ImVec2(self_xy[0], self_xy[1]), IM_COL32(255, 0, 0, 255), 2.f);
	}
	VectorCopy(orgPos, forwardStart);
	forwardStart[2] -= 1000;


	//Y
	forwardStart[1] -= DIRECTION_LENGTH;
	if (pitch < 0) {
		start[1] -= fabs(pitch) * 50;
	}

	if (r::WorldToScreen(forwardStart, self_xy) && r::WorldToScreen(start, end_xy)) {
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(end_xy[0], end_xy[1]), ImVec2(self_xy[0], self_xy[1]), IM_COL32(255, 0, 0, 255), 2.f);
	}
	start[1] = orgPos[1];
	if (pitch < 0) {
		start[1] += fabs(pitch) * 50;
	}
	forwardStart[1] += DIRECTION_LENGTH * 2;
	if (r::WorldToScreen(forwardStart, self_xy) && r::WorldToScreen(start, end_xy)) {
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(end_xy[0], end_xy[1]), ImVec2(self_xy[0], self_xy[1]), IM_COL32(255, 0, 0, 255), 2.f);
	}

	VectorCopy(orgPos, forwardStart);
	forwardStart[2] -= 1000;

	if (!v::mod_worldaxes_opt.isEnabled())
		return;

	const usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);

	static bool rightmove;

	if (cmd->rightmove > 0)
		rightmove = true;
	else if (cmd->rightmove < 0)
		rightmove = false;

	FPS_CalculateSingleBeatDirection(rightmove, cmd);
	float delta;
	float opt = R_getOptAngle(rightmove, delta);
	vec3_t opt3, nearestAxis3;
	AnglesToForward(vec3_t{ 0, opt, 0 }, forwardStart, DIRECTION_LENGTH, opt3);

	axis_s nearestAxis = CG_GetNearestWorldAxisFromYaw(opt);

	AnglesToForward(vec3_t{ 0, nearestAxis.angle, 0 }, forwardStart, DIRECTION_LENGTH, nearestAxis3);


	if (r::WorldToScreen(opt3, self_xy) && r::WorldToScreen(start, end_xy) && r::WorldToScreen(nearestAxis3, nearest_xy) ) {
		ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImVec2(end_xy[0], end_xy[1]), ImVec2(self_xy[0], self_xy[1]), ImVec2(nearest_xy[0], nearest_xy[1]), IM_COL32(255, 255, 0, 25));
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(end_xy[0], end_xy[1]), ImVec2(self_xy[0], self_xy[1]), IM_COL32(255, 255, 0, 255), 2.f);
	}

}
void cg::Mod_GetAccelerationAngles(const bool rightmove, vec2_t out)
{
	static float delta;
	float opt = R_getOptAngle(rightmove, delta);

	if(rightmove)
		out[0] = opt;
	else out[1] = opt;

	//delta = (delta > 0) ? delta : -delta;

	const float maxAngle = 45.f - (delta);



	if (rightmove) {
		out[1] = out[0] - maxAngle;
	}
	else {
		out[0] = out[1] + maxAngle;

	}
	//char buffer[40];
	//sprintf_s(buffer, "delta: %.6f", maxAngle);

	//r::R_DrawText(buffer, 0, 500, 1.5f, 1.5f, 0, vec4_t{ 0,255,0,255 }, 0);
}
void cg::Mod_DrawAngleHelper()
{
	if (NOT_SERVER || !v::mod_anglehelper.isEnabled() || !v::mod_anglehelper.evar->arrayValue[0])
		return;

	const usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);

	float yaw = clients->cgameViewangles[YAW];
	const dvar_s* cg_fov = Dvar_FindMalleableVar("cg_fov");
	const dvar_s* cg_fovscale = Dvar_FindMalleableVar("cg_fovscale");
	vec2_t accel_angles;

	static bool rightmove;

	if (cmd->rightmove > 0)
		rightmove = true;
	else if (cmd->rightmove < 0)
		rightmove = false;

	FPS_CalculateSingleBeatDirection(rightmove, cmd);

	


	if (!cg_fov || !cg_fovscale)
		return;

	Mod_GetAccelerationAngles(rightmove, accel_angles);

	const float fov = (cg_fov->current.value * cg_fovscale->current.value) * v::mod_anglehelper.evar->arrayValue[3];

	CG_FillAngleYaw(accel_angles[0], accel_angles[1], yaw, v::mod_anglehelper.evar->arrayValue[1], v::mod_anglehelper.evar->arrayValue[2], fov, vec4_t{255,0,0,0.7}, false);
	r::R_DrawRect("white", 958, v::mod_anglehelper.evar->arrayValue[1] - 10, 4, v::mod_anglehelper.evar->arrayValue[2] + 20, vec4_t{ 255,255,255,255 });
}
void cg::Mod_DrawCurveSpeed()
{

	if (!v::mod_curve_speed.isEnabled())
		return;

	static float yawPrevious = clients->cgameViewangles[YAW] < 0 ? -clients->cgameViewangles[YAW] : clients->cgameViewangles[YAW];
	const float yawNow = clients->cgameViewangles[YAW] < 0 ? -clients->cgameViewangles[YAW] : clients->cgameViewangles[YAW];
	static int32_t oldCommandTime = clients->snap.ps.commandTime;
	static float difference = yawNow - yawPrevious;
	if (clients->snap.ps.commandTime > oldCommandTime + 50) {
		difference = yawNow - yawPrevious;
		oldCommandTime = clients->snap.ps.commandTime;
		yawPrevious = clients->cgameViewangles[YAW] < 0 ? -clients->cgameViewangles[YAW] : clients->cgameViewangles[YAW];
		
		difference = difference < 0 ? -difference : difference;
	}


	char buffer[24];

	sprintf_s(buffer, "%.2f", difference);

	r::R_DrawText(buffer, 700, 200, 2, 2, 0, vec4_t{ 1,1,1,1 }, 0);
}





// Hit Visualizer: local prediction only, with a bounded command-time history.
// Never write to playerState, usercmd, trace mask or collision state here.
namespace {
    using HitPoint = std::array<float, 3>;
    struct HitBoxSample {
        bool valid = false;
        HitPoint origin{}, mins{}, maxs{};
    };
    struct JumpSample {
        std::vector<HitPoint> points;
        HitBoxSample hit;
    };
    JumpSample currentJump, lastJump;
    HitBoxSample pendingHit;
    int pendingHitTime = -1, lastSampleTime = -1;
    bool recordingJump = false;
    HitPoint MakeHitPoint(const float* v) { return {v[0], v[1], v[2]}; }
    bool ValidHitPoint(const HitPoint& v) {
        return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
    }
    bool LocalHitState(const cg::pmove_t* pm) {
        return pm && pm->ps && pm->ps == &cg::cgs->predictedPlayerState &&
            pm->ps->pm_type == cg::PM_NORMAL && !cg::clc.demoplaying;
    }
    void AppendHitPoint(JumpSample& jump, const HitPoint& point) {
        if (!ValidHitPoint(point)) return;
        if (!jump.points.empty() && jump.points.back() == point) return;
        // Preserve the beginning of the jump. Downsample older history if needed.
        if (jump.points.size() >= 4096) {
            size_t write = 1;
            for (size_t read = 2; read < jump.points.size(); read += 2)
                jump.points[write++] = jump.points[read];
            jump.points.resize(write);
        }
        jump.points.push_back(point);
    }
    void DrawHitLine(const HitPoint& a, const HitPoint& b, ImU32 col, float width) {
        vec3_t va{a[0], a[1], a[2]}, vb{b[0], b[1], b[2]};
        vec2_t sa{}, sb{};
        if (r::WorldToScreen(va, sa) && r::WorldToScreen(vb, sb))
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2(sa[0], sa[1]), ImVec2(sb[0], sb[1]), col, width);
    }
}

void cg::Mod_ResetHitVisualizer()
{
    currentJump = {};
    lastJump = {};
    pendingHit = {};
    pendingHitTime = lastSampleTime = -1;
    recordingJump = false;
}

void cg::Mod_RecordBounceContact(pmove_t* pm, int commandTime, const float* origin)
{
    if (!v::mod_hit_visualizer.evar || !v::mod_hit_visualizer.isEnabled() || !LocalHitState(pm) || commandTime <= lastSampleTime) return;
    if (pendingHit.valid && pendingHitTime == commandTime) return; // first contact in the step
    pendingHit.origin = MakeHitPoint(origin);
    pendingHit.mins = MakeHitPoint(pm->mins);
    pendingHit.maxs = MakeHitPoint(pm->maxs);
    pendingHit.valid = ValidHitPoint(pendingHit.origin) && ValidHitPoint(pendingHit.mins) && ValidHitPoint(pendingHit.maxs);
    pendingHitTime = commandTime;
}

void cg::Mod_RecordJumpStep(pmove_t* pm, int previousGround, const float* previousOrigin)
{
    if (!v::mod_hit_visualizer.evar || !v::mod_hit_visualizer.isEnabled() || !LocalHitState(pm)) return;
    const int time = pm->ps->commandTime;
    if (time <= lastSampleTime) return; // discard prediction replays of old commands
    lastSampleTime = time;
    const bool air = pm->ps->groundEntityNum == ENTITYNUM_NONE;
    if (!recordingJump && previousGround != ENTITYNUM_NONE && air) {
        currentJump = {};
        recordingJump = true;
        AppendHitPoint(currentJump, MakeHitPoint(previousOrigin));
    }
    if (recordingJump) {
        if (pendingHit.valid && pendingHitTime == time && !currentJump.hit.valid) {
            currentJump.hit = pendingHit;
            AppendHitPoint(currentJump, pendingHit.origin);
        }
        AppendHitPoint(currentJump, MakeHitPoint(pm->ps->origin));
        if (!air) {
            lastJump = std::move(currentJump);
            currentJump = {};
            recordingJump = false;
        }
    }
    if (pendingHitTime <= time) { pendingHit = {}; pendingHitTime = -1; }
}

void cg::Mod_DrawHitVisualizer()
{
    if (!v::mod_hit_visualizer.evar || !v::mod_hit_visualizer.isEnabled() || NOT_SERVER || clc.demoplaying || !ImGui::GetCurrentContext()) return;
    // Show a completed jump until the next is finished; show the live trail
    // only when there is no completed jump yet.
    const auto& jump = lastJump.points.empty() ? currentJump : lastJump;
    for (size_t i = 1; i < jump.points.size(); ++i)
        DrawHitLine(jump.points[i - 1], jump.points[i], IM_COL32(135, 221, 240, 225), 2.0f);
    if (!jump.hit.valid) return;
    HitPoint corners[8]{};
    for (int i = 0; i < 8; ++i)
        for (int axis = 0; axis < 3; ++axis)
            corners[i][axis] = jump.hit.origin[axis] + ((i & (1 << axis)) ? jump.hit.maxs[axis] : jump.hit.mins[axis]);
    for (int i = 0; i < 8; ++i)
        for (int axis = 0; axis < 3; ++axis)
            if (!(i & (1 << axis))) DrawHitLine(corners[i], corners[i | (1 << axis)], IM_COL32(245, 151, 183, 255), 2.0f);
}
