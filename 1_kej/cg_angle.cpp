#include "pch.h"

cg::range_t cg::AnglesToRange(float start, float end, float yaw, float fov, int projType)
{
	if (fabsf(end - start) > 2 * (float)M_PI)
	{
		range_t const ret = { 0, float(1920.f), false };
		return ret;
	}

	bool split = end > start;
	start = AngleNormalizePI(start - yaw);
	end = AngleNormalizePI(end - yaw);

	if (end > start)
	{
		split = !split;
		float const tmp = start;
		start = end;
		end = tmp;
	}

	range_t const ret = { ProjectionX(start, fov, projType), ProjectionX(end, fov, projType), split };
	return ret;
}
float cg::ProjectionX(float angle, float fov, int projType)
{
	float SCREEN_WIDTH = 1920.f;
	float const half_fov_x = (float)DEG2RAD(fov) / 2;
	if (angle >= half_fov_x)
	{
		return 0;
	}
	if (angle <= -half_fov_x)
	{
		return SCREEN_WIDTH;
	}
	switch (projType) {
	case 0:
		return SCREEN_WIDTH / 2 * (1 - tanf(angle) / tanf(half_fov_x));
		break;
	case 1: 
		return SCREEN_WIDTH / 2 * (1 - angle / half_fov_x);
		break;
	case 2: 
		return SCREEN_WIDTH / 2 * (1 - tanf(angle / 2) / tanf(half_fov_x / 2));
		break;

	default:
		return 0;
		break;

	}
}
void cg::CG_FillAngleYaw(float start, float end, float yaw, float y, float h, float fov, vec4_t const color, bool useImGui, bool isCylindrical)
{

	range_t const range = AnglesToRange(DEG2RAD(start), DEG2RAD(end), DEG2RAD(yaw), fov);

	const float widthOffs = v::mod_fps_transferz.GetArray(5);
	const float minX = widthOffs;
	const float maxX = 1920.f - widthOffs;

	float x1 = range.x1 < minX ? minX : range.x1;
	float x2 = range.x2 - range.x1;

	
	x1 = range.x1 > maxX ? maxX : x1;
	x2 = range.x2 > maxX ? maxX - x1 : x2;
	

	if (!range.split) {
		//r::R_DrawRect("white", range.x1, y, range.x2 - range.x1, h, color);
		if (!isCylindrical) {
			if (useImGui)
				ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(r::X(x1), r::Y(y)), ImVec2(r::X(ImClamp(range.x2, minX, maxX)), r::Y(y + h)), ImColor(color[0], color[1], color[2], 0.7f));
			else
				r::R_DrawRect("white", range.x1, y, range.x2 - range.x1, h, color);

			return;
		}
		int const heightOffsetX1 = glm::distance((float)range.x1, r::X(960.f));
		int const heightOffsetX2 = glm::distance((float)range.x2, r::X(960.f));
		
		float midYAW = glm::distance(end, start)/2;
		midYAW += start;
		range_t const midpoint = AnglesToRange(DEG2RAD(midYAW), DEG2RAD(midYAW), DEG2RAD(yaw), fov);

		int const heightOffsetMiddle = glm::distance((float)midpoint.x1, r::X(960.f));

		const ImVec2 a(range.x1, y+heightOffsetX1/2);
		const ImVec2 b(midpoint.x1, y+heightOffsetMiddle/2);
		const ImVec2 c(range.x2, y+heightOffsetX2/2);

		ImGui::GetBackgroundDrawList()->AddBezierQuadratic(a, b, c, ImColor(color[0], color[1], color[2], color[3]), h);
		//ImGui::GetBackgroundDrawList()->AddText(ImVec2(range.x1, y), IM_COL32(255, 255, 255, 255), std::string(std::to_string(start)).c_str());
		//ImGui::GetBackgroundDrawList()->AddText(ImVec2(range.x2, y), IM_COL32(255, 255, 255, 255), std::string(std::to_string(end)).c_str());

		//float const val = midpoint.x1;

		//ImGui::GetBackgroundDrawList()->AddText(ImVec2(val, y), IM_COL32(255, 255, 255, 255), std::string(std::to_string(midYAW)).c_str());

	}
	else {
		if (!isCylindrical) {
			if (useImGui) {
				ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(r::X(minX), r::Y(y)), ImVec2(r::X(x1), r::Y(y + h)), ImColor(color[0], color[1], color[2], color[3]));
				ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(r::X(ImClamp(range.x2, minX, maxX)), r::Y(y)), ImVec2(r::X(ImClamp((float)cgs->refdef.width, minX, maxX)), r::Y(y + h)), ImColor(color[0], color[1], color[2], color[3]));
			}
			else {
				r::R_DrawRect("white", 0, y, range.x1, h, color);
				r::R_DrawRect("white", range.x2, y, 1920.f - range.x2, h, color);
			}
		}
	}
}
void cg::setYaw(float ref, float ang)
{
	ref = fmodf(ref, 360);

	ref -= ref * 2 - ang;

	clients->viewangles[1] += ref;

}
void cg::setPitch(float ref, float ang)
{
	ref = fmodf(ref, 360);

	ref -= ref * 2 - ang;

	clients->viewangles[0] += ref;
}
void cg::setRoll(float ref, float ang)
{
	ref = fmodf(ref, 360);

	ref -= ref * 2 - ang;

	clients->viewangles[2] += ref;

}
void cg::CG_SetPlayerAngles(vec3_t source, vec3_t target)
{
	setPitch(source[0], target[0]);
	setYaw(source[1], target[1]);
	setRoll(source[2], target[2]);
}
void cg::CG_ApplySmoothing(vec3_t src, vec3_t dst, const float smoothing, vec3_t out)
{
	vec3_t dist;
	VectorSubtract(dst, src, dist);

	out[0] = (dist[0] / smoothing) + src[0];
	out[1] = (dist[1] / smoothing) + src[1];
	out[2] = (dist[2] / smoothing) + src[2];


}
void cg::CG_ApplySmoothing(float src, float dst, const float smoothing, float& out) {

	const float dist = dst - src;

	out = (dist / smoothing) + src;
}
//more suited for visual stuff
float cg::R_getOptAngle(const bool rightmove, float& diff)
{
	

	//usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);

	char* forwardmove = &input->move;
	char* sidemove = &input->strafe;


	float _speed = glm::length(glm::vec2(clients->cgameVelocity[0], clients->cgameVelocity[1]));

	if (_speed < 1)
		return -400.0;

	float yaw = ps_loc->viewangles[YAW];
	float g_speed = (float)cgs->snap->ps.speed;
	dvar_s* fpsDvar = Dvar_FindMalleableVar("com_maxfps");
	const float FPS = fpsDvar ? (float)fpsDvar->current.integer : 125.0f;

	if (g_speed <= 0.0f || FPS <= 0.0f)
		return -400.0f;

	float accel = FPS / g_speed;

	if (_speed < 190)
		g_speed = 190.f - (190.f - _speed);
	else if (GROUND)
		g_speed = 224.f;

	const double velocitydirection = atan2(clients->cgameVelocity[1], clients->cgameVelocity[0]) * 180.f / PI;
	const double accelerationAng = atan2(-(int)*sidemove, (int)*forwardmove) * 180.f / PI;
	double acosArg = (g_speed - accel) / _speed;
	acosArg = acosArg < -1.0 ? -1.0 : (acosArg > 1.0 ? 1.0 : acosArg);
	diff = (float)(acos(acosArg) * 180.0 / PI);
	//const float minAngle = acos(g_speed / _speed) * 180.f / PI;

	float delta = yaw;


	if (rightmove) {
		delta = (velocitydirection - diff - accelerationAng);
		//_opt = delta - yaw;
	}
	else{
		delta = (velocitydirection + diff - accelerationAng);
		//_opt = delta - yaw;
	}
	diff /= 2;
	yaw = delta;

	return yaw;
}
float cg::getOptAngle(float& _opt, const bool all_techs)
{
    // Preserve the original entry point for analyzer/jump callers.
    return getOptAngle(_opt, all_techs, input->move, input->strafe);
}

float cg::getOptAngle(float& _opt, const bool all_techs, const char forwardmoveIn, const char rightmoveIn)
{
    // Use the movement values from the same usercmd that the strafe bot reads.
    char forwardmove = forwardmoveIn;
    char rightmove = rightmoveIn;

    const float _speed = glm::length(glm::vec2(clients->cgameVelocity[1], clients->cgameVelocity[0]));

    if (_speed < 1.0f || (_speed < clients->snap.ps.speed && NOT_GROUND) ||
        (!all_techs && !GROUND && forwardmove != 127 && rightmove != 0) ||
        (GROUND && (forwardmove <= 0 || rightmove == 0)) ||
        (analyzer.isSegmenting() && !analyzer.segmenterData.isReady))
        return -400.0f;

    // Keep the existing all-techs behavior for alternate movement combinations.
    if (!all_techs && !GROUND)
        forwardmove = 127;

	float yaw = clients->cgameViewangles[YAW];

    if (GROUND) {
        // Flat, ordinary ground: friction acts before acceleration.
        // Use the actual physics step, not the render FPS divided by speed.
        const float wishSpeed = static_cast<float>(clients->snap.ps.speed);
        if (!std::isfinite(_speed) || wishSpeed <= 0.0f)
            return -400.0f;

        float dt = jumpanalyzer.frametime;
        if (!std::isfinite(dt) || dt <= 0.0f || dt > 0.05f) {
            dvar_s* fps = Dvar_FindMalleableVar("com_maxfps");
            dt = 1.0f / (fps && fps->current.integer > 0 ? fps->current.integer : 333.0f);
        }
        dt = glm::clamp(dt, 0.001f, 0.05f);
        const float friction = 5.5f;
        const float stopSpeed = 100.0f;
        const float groundAccel = 9.0f;
        const float control = _speed > stopSpeed ? _speed : stopSpeed;
        const float afterFriction = glm::max(0.0f, _speed - control * friction * dt);
        const float accelStep = groundAccel * wishSpeed * dt;
        // A small projection margin reduces lost acceleration from tracking lag.
        const float projection = glm::max(0.0f, wishSpeed - accelStep * 1.10f);
        const float ratio = afterFriction > 0.0f
            ? glm::clamp(projection / afterFriction, 0.0f, 1.0f) : 1.0f;
        const float angle = acosf(ratio) * 180.0f / PI;
        const float velocityYaw = atan2f(clients->cgameVelocity[1], clients->cgameVelocity[0]) * 180.0f / PI;
        const float inputYaw = atan2f(-static_cast<float>(rightmove), static_cast<float>(forwardmove)) * 180.0f / PI;
        // At running start, retain the player's chosen heading and progressively
        // introduce the optimal correction near normal running speed.
        const float blend = glm::clamp((_speed / wishSpeed - 0.70f) / 0.30f, 0.0f, 1.0f);
        const float target = velocityYaw + (rightmove > 0 ? -angle : angle) - inputYaw;
        _opt = AngleNormalize180(target - yaw) * blend;
        return yaw + _opt;
    }

    // Air acceleration projection uses one physics step and player wish speed.
    // Do not mix the displayed FPS zone with the actual frame duration.
    const float g_speed = static_cast<float>(clients->snap.ps.speed);
    dvar_s* fpsDvar = Dvar_FindMalleableVar("com_maxfps");
    const float FPS = fpsDvar && fpsDvar->current.integer > 0
        ? static_cast<float>(fpsDvar->current.integer) : 125.0f;
    if (g_speed <= 0.0f || !std::isfinite(_speed)) return -400.0f;
    const float dt = glm::clamp(1.0f / FPS, 0.001f, 0.05f);
    const float accel = g_speed * dt;

	const double velocitydirection = atan2(clients->cgameVelocity[1], clients->cgameVelocity[0]) * 180.f / PI;
	const double accelerationAng = atan2(-rightmove, forwardmove) * 180.f / PI;
	double acosArg = (g_speed - accel) / _speed;
	acosArg = acosArg < -1.0 ? -1.0 : (acosArg > 1.0 ? 1.0 : acosArg);
	double diff = acos(acosArg) * 180.0 / PI;
	//const float minAngle = acos(g_speed / _speed) * 180.f / PI;

	//if (jumpanalyzer.recommendedFPS == 125)
	//	diff += accel; 

	float delta = yaw;


	if (rightmove > 0 || (rightmove == 0 && all_techs && (WE || SE))) {
		delta = (velocitydirection - diff - accelerationAng);
		_opt = delta - yaw;
	}
	else if (rightmove < 0 || (rightmove == 0 && all_techs && (WQ || SQ))) {
		delta = (velocitydirection + diff - accelerationAng);
		_opt = delta - yaw;
	}
	yaw = delta;

	

	return yaw;
}
float cg::getOptForAnalyzer(jump_data* data, bool onGround)
{
	const float _speed = glm::length(glm::vec2(data->velocity[0], data->velocity[1]));

	if (_speed < 1)
		return -400.0;

	float yaw = data->angles[YAW];
	float g_speed = (float)cgs->snap->ps.speed;
	const float FPS = (float)data->FPS;

	float accel = FPS / g_speed;

	if (_speed < 190)
		g_speed = 190.f - (190.f - _speed);
	else if (onGround && _speed < 370)
		g_speed = 224.f;

	const float velocitydirection = atan2(data->velocity[1], data->velocity[0]) * 180.f / PI;
	const float accelerationAng = atan2(-(int)data->rightmove, (int)data->forwardmove) * 180.f / PI;
	float acosArg = (g_speed - accel) / _speed;
	acosArg = acosArg < -1.0f ? -1.0f : (acosArg > 1.0f ? 1.0f : acosArg);
	float diff = (float)(acos(acosArg) * 180.0 / PI);
	//const float minAngle = acos(g_speed / _speed) * 180.f / PI;

	//if (jumpanalyzer.recommendedFPS == 125)
	//	diff += accel;

	float delta = yaw;


	if ((int)data->rightmove > 0) {
		delta = (velocitydirection - diff - accelerationAng);
	}
	else if ((int)data->rightmove < 0) {
		delta = (velocitydirection + diff - accelerationAng);
	}
	yaw = delta;

	return yaw;
}
float cg::DistanceToOpt(float delta, const float& yaw)
{
	float opt = fabs(yaw - delta);

	opt = opt > 90 == true ? 360.f - opt : opt;
	return opt;

}

axis_s cg::CG_GetNearestWorldAxisFromYaw(float yawangle)
{
	if (yawangle > 180 || yawangle < -180)
		yawangle = AngleNormalize180(yawangle);

	axis_s ax;


	if (yawangle >= 135 || yawangle <= -135) {
		ax.axis = axis_e::xNegative;
		ax.angle = 180;
		return ax;
	}
	else if (yawangle <= -45 && yawangle >= -135) {
		ax.axis = axis_e::yNegative;
		ax.angle = 270;
		return ax;
	}
	else if (yawangle >= 45 && yawangle <= 135) {
		ax.axis = axis_e::yPositive;
		ax.angle = 90;
		return ax;
	}
	ax.axis = axis_e::xPositive;
	ax.angle = 0;
	return ax;  


}
axis_s cg::CG_GetNearestWorldAxisFromYaw()
{
	axis_s ax;

	const float yawangle = clients->cgameViewangles[YAW];

	if (yawangle >= 135 || yawangle <= -135) {
		ax.axis = axis_e::xNegative;
		ax.angle = 180;
		return ax;
	}
	else if (yawangle <= -45 && yawangle >= -135) {
		ax.axis = axis_e::yNegative;
		ax.angle = 270;
		return ax;
	}
	else if (yawangle >= 45 && yawangle <= 135) {
		ax.axis = axis_e::yPositive;
		ax.angle = 90;
		return ax;
	}
	ax.axis = axis_e::xPositive;
	ax.angle = 0;
	return ax; 

}