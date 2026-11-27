#include "pch.h"

void cg::Mod_A_Strafebot()
{
    static DWORD lastGroundUpdate = 0;
    static float groundBlend = 0.0f;
    static int lastGroundSide = 0;
    static int lastAirSide = 0;
    static DWORD lastAirUpdate = 0;
    static DWORD airSwitchTime = 0;
    const auto resetGround = [&]() {
        lastGroundUpdate = 0;
        groundBlend = 0.0f;
        lastGroundSide = 0;
    };
	if (r::inputCaptured || !v::mod_strafebot.isEnabled() || clients->snap.ps.pm_type == PM_UFO || v::mod_strafebot.isEnabled() && (GetAsyncKeyState(v::mod_strafebot_ow.GetInt()) < 0) == true)
	{ resetGround(); lastAirSide = 0; lastAirUpdate = 0; airSwitchTime = 0; return; }

	const usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);

    // WA/WD is the basic ground mode. It must not depend on the separate
    // "All Techniques" option; that option remains relevant to other patterns.
    if (cmd->rightmove == 0 || (GROUND && cmd->forwardmove <= 0)) {
        resetGround();
        if (GROUND) { lastAirSide = 0; lastAirUpdate = 0; airSwitchTime = 0; }
        return;
    }
    if (NOT_GROUND)
        resetGround();

	float optYaw, test;

	if (std::isnan(clients->viewangles[YAW]))
		clients->viewangles[YAW] = 0;
	else if (clients->viewangles[YAW] > 3000 || clients->viewangles[YAW] < -3000)
		clients->viewangles[YAW] = fmodf(clients->viewangles[YAW], 360);

	optYaw = getOptAngle(test, v::mod_strafebot_all.isEnabled(), cmd->forwardmove, cmd->rightmove);
	if (optYaw == -400.f || !std::isfinite(optYaw)) {
        resetGround();
        return;
    }

    if (GROUND) {
        lastAirSide = 0; lastAirUpdate = 0; airSwitchTime = 0;
        const DWORD now = Sys_MilliSeconds();
        const int side = cmd->rightmove > 0 ? 1 : -1;
        if (!lastGroundUpdate || now - lastGroundUpdate > 100) {
            lastGroundUpdate = now;
            lastGroundSide = side;
            groundBlend = 0.0f;
            return; // Preserve camera on engagement and direction changes.
        }
        const bool switched = side != lastGroundSide;
        lastGroundSide = side;
        if (switched) groundBlend = glm::max(groundBlend, 0.75f);
        const float dt = glm::min(0.025f, (now - lastGroundUpdate) * 0.001f);
        lastGroundUpdate = now;
        groundBlend = glm::min(1.0f, groundBlend + dt / 0.15f);
        const float yaw = clients->cgameViewangles[YAW];
        const float error = AngleNormalize180(optYaw - yaw);
        const float maxStep = (60.0f + 180.0f * groundBlend + (switched ? 180.0f : 0.0f)) * dt;
        const float correction = glm::clamp(error * glm::min(1.0f, dt * 18.0f) * groundBlend,
                                            -maxStep, maxStep);
        clients->viewangles[YAW] += correction;
        return;
    }

    const DWORD now = Sys_MilliSeconds();
    const int side = cmd->rightmove > 0 ? 1 : -1;
    const bool continuous = lastAirUpdate && now - lastAirUpdate <= 100;
    if (continuous && lastAirSide && side != lastAirSide)
        airSwitchTime = now;
    if (!continuous) airSwitchTime = 0;
    const float dt = continuous ? glm::clamp((now - lastAirUpdate) * 0.001f, 0.0f, 0.025f) : 0.003f;
    lastAirUpdate = now;
    lastAirSide = side;
    const float error = AngleNormalize180(optYaw - clients->cgameViewangles[YAW]);
    const bool switching = airSwitchTime && now - airSwitchTime < 65;
    const int sinceJump = jumpanalyzer.serverTime - jumpanalyzer.airTime;
    const int sinceGround = jumpanalyzer.serverTime - jumpanalyzer.groundTime;
    const float smoothing = glm::max(1.0f, (sinceJump >= 0 && sinceJump < 300 || sinceGround < 200)
        ? v::mod_strafebot_bhop_s.GetFloat() : v::mod_strafebot_smooth.GetFloat());
    // Fast shortest-arc correction during WA/WD reversal; normal smoothing otherwise.
    const float alpha = switching ? 1.0f : (smoothing <= 1.0f ? 1.0f : 1.0f - powf(1.0f - 1.0f / smoothing, dt * 125.0f));
    const float correction = switching ? glm::clamp(error, -1800.0f * dt, 1800.0f * dt) : error * alpha;
    clients->viewangles[YAW] += correction;

}
void cg::Mod_A_AutoFPS()
{
	if (!v::mod_autoFPS.isEnabled() && !box500.boxExists && v::mod_autoFPS_hug500.GetInt() == 0)
		return;

	if (jumpanalyzer.recommendedFPS == NULL || analyzer.isPreviewing() || !VID_ACTIVE || jbuilder.isGeneratingMovement())
		return;

	dvar_s* com_maxfps = Dvar_FindMalleableVar("com_maxfps");
	if (!com_maxfps)
		return;

	bool const spaceHeld = (GetAsyncKeyState(VK_SPACE) < 0) == true;

	int fps = jumpanalyzer.recommendedFPS;

	if (!v::mod_autoFPS.isEnabled())
		fps = com_maxfps->current.integer;

	// hug500 - bounce hugging needs higher fps
	// (applies regardless of autoFPS state - specific bounce technique)
	if (v::mod_autoFPS_hug500.GetInt() > 0 && jumpanalyzer.hugging_bounce && !jumpanalyzer.walking && !spaceHeld) {
		switch (v::mod_autoFPS_hug500.GetInt()) {
		case 1: fps = 333;  break;
		case 2: fps = 500;  break;
		case 3: fps = 1000; break;
		default: break;
		}
	}

	// box500 - highest priority. Overrides everything else when inside the zone.
	if (box500.use500) {
		fps = 500;
	}

	// autoFPS base logic
	if (v::mod_autoFPS.isEnabled() && !box500.use500) {
		if (GROUND) {
			// On ground:
			//  - holding SPACE with space333 disabled -> 125 (max jump height)
			//  - everything else                        -> 333 (PRIORITY: strafe-gain)
			//
			// Previously this was unconditionally 125 on ground, which made fps
			// stuck at 125 on map load and while running around without jumping,
			// and prevented the player from using 333 strafe-gain fps on ground.
			if (spaceHeld && !v::mod_autoFPS_space333.isEnabled()) {
				fps = 125;
			} else {
				fps = 333;
			}
		} else {
			// In air:
			//  - holding SPACE with space333 enabled -> 333 (bhop)
			//  - otherwise                           -> recommendedFPS (velocity-based)
			if (spaceHeld && v::mod_autoFPS_space333.isEnabled()) {
				fps = 333;
			}
		}
	}

	com_maxfps->current.integer = fps;
}
void cg::Mod_A_AdjustRPG(pmove_t* pm, pml_t* pml)
{

	if (!v::mod_autoanglerpg.isEnabled())
		return;

	static bool rpg_isangling(false);
	static float angleEveryFrame(0);
	int rpg_indx = BG_FindWeaponIndexForName("rpg_mp");



	if(pm->ps->weapon != rpg_indx)
		rpg_indx = BG_FindWeaponIndexForName("rpg_sustain_mp");




	if (pm->ps->weapon == rpg_indx && (pm->cmd.buttons & 1) && pm->ps->weaponstate == WEAPON_READY) {
		rpg_isangling = true;
		float DistanceToDown = glm::distance(pm->ps->viewangles[PITCH], 85.f);
		angleEveryFrame = DistanceToDown / 1000.f * (500.f / (float)Dvar_FindMalleableVar("com_maxfps")->current.integer);
		//Com_Printf(CON_CHANNEL_OBITUARY, "each frame: %.3f\n", angleEveryFrame);
	}

	if (!rpg_isangling)
		return;

	clients->viewangles[PITCH] += angleEveryFrame;

	if (pm->ps->viewangles[PITCH] >= 85 || pm->ps->weaponstate == WEAPON_RELOADING)
		rpg_isangling = false;

}

void cg::Mod_A_AutoSliding(pmove_t* pmove, pml_t* pml)
{

	if (!v::mod_autoslide.isEnabled()) {
		//Com_PrintError(CON_CHANNEL_OBITUARY, "Mod_A_AutoSliding(): shouldn't have been called at this time! (keybind used but the feature itself thinks its unbound?)\n");
		//Com_PrintError(CON_CHANNEL_CONSOLEONLY, "Mod_A_AutoSliding(): shouldn't have been called at this time! (keybind used but the feature itself thinks its unbound?)\n");
		return;
	}

	// force sliding through engine
	const auto methodA = [](pmove_t* pmove, pml_t* pml, DWORD& old_ms, const bool part1) -> void
	{
		hook* a = nullptr;
		if (automation.pendingSlide && (pml->almostGroundPlane || pml->groundPlane)) {
			automation.currentlySliding = true;
			a->write_addr(0x410660, "\xC3", 1); 
			old_ms = Sys_MilliSeconds();
		}
		if (part1)
			return;

		const DWORD ms = Sys_MilliSeconds();


		if ((old_ms + (move->jump == true && v::mod_bhop_nodelay.isEnabled() ? 1 : 100)) < ms) { //allow 100ms to slide and if jump is not held
			old_ms = ms;
			automation.currentlySliding = false;
			a->write_addr(0x410660, "\x83", 1);
			automation.pendingSlide = false;
		}

	};	
	//switch to 15fps
	const auto methodB = [](pmove_t* pm, pml_t* pml, DWORD& old_ms, const bool part1) -> void 
	{
		static int fps_b4_switch = 125;
		if (automation.pendingSlide && !automation.currentlySliding) {
			fps_b4_switch = Dvar_FindMalleableVar("com_maxfps")->current.integer;
			trace_t trace;
			playerState_s* ps = pm->ps;
			vec3_t end(ps->origin[0], ps->origin[1], ps->origin[2]);
			end[2] -= 100;
			
			PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end, ps->clientNum, pm->tracemask);

			if (trace.fraction != 1.f && ps->velocity[2] > 0 && NOT_GROUND) {
				//Com_Printf(CON_CHANNEL_OBITUARY, "hey from 15fps\n");
				automation.currentlySliding = true;
			}

			old_ms = Sys_MilliSeconds();

		}
		else if (automation.currentlySliding) {
			if (v::mod_autoFPS.isEnabled())
				jumpanalyzer.recommendedFPS = 15;
			else
				Dvar_FindMalleableVar("com_maxfps")->current.integer = 15;

			if (old_ms + 250 < Sys_MilliSeconds()) {
				automation.pendingSlide = false;
				automation.currentlySliding = false;

				Dvar_FindMalleableVar("com_maxfps")->current.integer = fps_b4_switch;


			}

		}
	};

	hook* a = 0;
	static DWORD old_ms(0);

	if (automation.keybindPressed & 1 && !automation.currentlySliding) { 
		automation.pendingSlide = true;
		automation.keybindPressed = false;
	}

	bool use_methodA = v::mod_autoslide.GetInt() == 1;

	if (use_methodA)
		methodA(pmove, pml, old_ms, true);
	else
		methodB(pmove, pml, old_ms, true);


	if (!automation.currentlySliding)
		return;
	

	if (use_methodA)
		methodA(pmove, pml, old_ms, false);



}
void cg::Mod_A_OnCreate500FPS()
{
	if (!v::mod_auto500_enabled.isEnabled()) {
		Com_PrintError(CON_CHANNEL_OBITUARY, "500fps area spawning is disabled!\n");
		return;
	}

	trace_t trace;
	vec3_t end;

	AnglesToForward(clients->cgameViewangles, rg->viewOrg, 99999, end);

	CG_TracePoint(vec3_t{ 1,1,1 }, &trace, rg->viewOrg, vec3_t{ -1,-1,-1 }, end, cgs->clientNum, MASK_PLAYERSOLID, 1, 1);

	vec3_t endpos{};

	box500.origin[0] = rg->viewOrg[0] + trace.fraction * (end[0] - rg->viewOrg[0]);
	box500.origin[1] = rg->viewOrg[1] + trace.fraction * (end[1] - rg->viewOrg[1]);
	box500.origin[2] = rg->viewOrg[2] + trace.fraction * (end[2] - rg->viewOrg[2]);

	Com_Printf(CON_CHANNEL_OBITUARY, "^2Spawned at (%.3f, %.3f, %.3f)\n", box500.origin[0], box500.origin[1], box500.origin[2]);

	box500.boxExists = true;

}
void cg::Mod_A_500FPS()
{
	if (!box500.boxExists)
		return;

	static bool isInside(false);

	vec3_t mins, maxs, null{0,0,0};

	VectorCopy(null, mins);
	VectorCopy(null, maxs);

	VectorSubtract(mins, box500.bounds, mins);
	VectorSubtract(maxs, box500.bounds, maxs);

	if (!isInside) { // don't draw the box when we are inside of it
		r::box_s box(box500.origin, mins, maxs);
		box.R_DrawConstructedBox(vec4_t{ 255,0,0,55 });
		box.R_DrawConstructedBoxEdges(vec4_t{ 255,0,0,255 });
	}
	const auto PlayerWithinBounds = [](vec3_t mins, vec3_t maxs) -> bool {


		if (	clients->cgameOrigin[0] > mins[0] && clients->cgameOrigin[0] < maxs[0]
			&&	clients->cgameOrigin[1] > mins[1] && clients->cgameOrigin[1] < maxs[1]
			&&	clients->cgameOrigin[2] > mins[2] && clients->cgameOrigin[2] < maxs[2]) 

			return true;

			

		
		return false;

	};

	VectorCopy(null, mins);
	VectorCopy(null, maxs);

	VectorSubtract(mins, box500.bounds, mins);
	VectorAdd(maxs, box500.bounds, maxs);

	VectorAdd(maxs, box500.origin, maxs);
	VectorAdd(mins, box500.origin, mins);

	isInside = PlayerWithinBounds(mins, maxs);
	box500.use500 = isInside && !jumpanalyzer.hasBounced;


}