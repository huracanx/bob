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
	{ automation.paraActive = false; resetGround(); lastAirSide = 0; lastAirUpdate = 0; airSwitchTime = 0; return; }

	const usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);
    if (!cmd) return;
    // Live movement input is used for air WA/WD; ground keeps its existing path.
    const char forward = input->move;
    const char sideMove = input->strafe;
    // W-only modes are airborne only. Explicit A/D always wins.
    const bool manual = sideMove != 0 || GetAsyncKeyState('A') < 0 || GetAsyncKeyState('D') < 0;
    const bool airborne = NOT_GROUND && !jumpanalyzer.walking;
    const bool para = airborne && forward > 0 && !manual &&
        (v::mod_auto_para.isEnabled() || v::mod_auto_250.isEnabled());
    if (sideMove) automation.paraSide = sideMove > 0 ? 1 : -1;
    static bool was250 = false;
    static DWORD last250Switch = 0;
    const DWORD switchNow = Sys_MilliSeconds();
    automation.paraActive = para;
    const bool auto250 = para && v::mod_auto_250.isEnabled() && v::mod_autoFPS.isEnabled() &&
        v::mod_fps_override.GetInt() == 0;
    if (!auto250) { was250 = false; last250Switch = 0; }
    else {
        const int zone = Mod_RecommendedFPS(AngleNormalize180(clients->cgameViewangles[YAW]), forward, 0);
        const auto* currentFPS = Dvar_FindMalleableVar("com_maxfps");
        const bool in250 = zone == 250 || (currentFPS && currentFPS->current.integer == 250);
        if (in250 && !was250 && (!last250Switch || switchNow - last250Switch >= 100)) {
            automation.paraSide = -automation.paraSide;
            last250Switch = switchNow;
            ++automation.auto250Switches;
            was250 = true;
        } else if (!in250 && (!last250Switch || switchNow - last250Switch >= 100)) {
            was250 = false;
        }
    }

    // WA/WD is the basic ground mode. It must not depend on the separate
    // "All Techniques" option; that option remains relevant to other patterns.
    if ((!para && sideMove == 0) || (GROUND && forward <= 0)) {
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

	optYaw = getOptAngle(test, v::mod_strafebot_all.isEnabled(), forward, sideMove);
	if (optYaw == -400.f || !std::isfinite(optYaw)) {
        resetGround();
        return;
    }

    if (GROUND) {
        lastAirSide = 0; lastAirUpdate = 0; airSwitchTime = 0;
        const DWORD now = Sys_MilliSeconds();
        const int side = automation.paraSide;
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
    const int side = automation.paraSide;
    const bool continuous = lastAirUpdate && now - lastAirUpdate <= 100;
    if (continuous && lastAirSide && side != lastAirSide)
        airSwitchTime = now;
    if (!continuous) airSwitchTime = 0;
    const float dt = continuous ? glm::clamp((now - lastAirUpdate) * 0.001f, 0.0f, 0.025f) : 0.003f;
    lastAirUpdate = now;
    lastAirSide = side;
    const float error = AngleNormalize180(optYaw - clients->cgameViewangles[YAW]);
    const bool switching = airSwitchTime && now - airSwitchTime < 65;
    const bool diagonal = forward > 0 && (sideMove != 0 || para);
    const int sinceJump = jumpanalyzer.serverTime - jumpanalyzer.airTime;
    const int sinceGround = jumpanalyzer.serverTime - jumpanalyzer.groundTime;
    const float smoothing = glm::max(1.0f, (sinceJump >= 0 && sinceJump < 300 || sinceGround < 200)
        ? v::mod_strafebot_bhop_s.GetFloat() : v::mod_strafebot_smooth.GetFloat());
    // Fast shortest-arc correction during WA/WD reversal; normal smoothing otherwise.
    const float alpha = switching ? 1.0f : (smoothing <= 1.0f ? 1.0f : 1.0f - powf(1.0f - 1.0f / smoothing, dt * 125.0f));
    // A reversal must reach the new acceleration direction immediately.
    // For ordinary diagonal tracking, keep the error within a small tolerance.
    const float smoothed = error * alpha;
    const float correction = diagonal && switching ? error : diagonal
        ? error - glm::clamp(error - smoothed, -0.10f, 0.10f) : smoothed;
    clients->viewangles[YAW] += correction;

}
void cg::Mod_A_AutoFPS()
{
	if (!v::mod_autoFPS.isEnabled() && !box500.boxExists && v::mod_autoFPS_hug500.GetInt() == 0)
		return;

	if (!VID_ACTIVE)
		return;

	dvar_s* com_maxfps = Dvar_FindMalleableVar("com_maxfps");
	if (!com_maxfps)
		return;

	bool const spaceHeld = (GetAsyncKeyState(VK_SPACE) < 0) == true;

	// Refresh the recommendation independently of the FPS helper rendering.
    if (v::mod_autoFPS.isEnabled() && NOT_GROUND && !r::inputCaptured) {
        const usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);
        const char forward = v::mod_strafebot.isEnabled() ? input->move : (cmd ? cmd->forwardmove : 0);
        const char side = v::mod_strafebot.isEnabled() ? input->strafe : (cmd ? cmd->rightmove : 0);
        if (forward != 0 || side != 0)
            jumpanalyzer.recommendedFPS = Mod_RecommendedFPS(
                AngleNormalize180(clients->cgameViewangles[YAW]), forward, side);
    }
    int fps = jumpanalyzer.recommendedFPS ? jumpanalyzer.recommendedFPS : com_maxfps->current.integer;

	if (!v::mod_autoFPS.isEnabled())
		fps = com_maxfps->current.integer;
    else if (NOT_GROUND) {
        const int selected = v::mod_fps_override.GetInt();
        if (selected == 166 || selected == 142 || selected == 111) fps = selected;
    }

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

	// Ground jump height and ground strafe gain.
	if (v::mod_autoFPS.isEnabled() && !box500.use500 && GROUND)
		fps = spaceHeld ? 125 : 333;

	if (automation.currentlySliding && v::mod_autoslide.GetInt() == 2) fps = 15;
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

namespace {
    DWORD slideArmedAt = 0, slideStartedAt = 0;
    int slideSavedFPS = 0, slideActiveMode = 0;
    bool slideKeyDown = false, slidePatched = false, slideSawAir = false, slideContact = false;
}
void cg::Mod_A_CancelSliding()
{
    if (slidePatched) {
        hook h;
        h.write_addr(0x410660, "\x83", 1);
        slidePatched = false;
    }
    if (slideActiveMode == 2 && slideSavedFPS > 0) {
        if (auto* fps = Dvar_FindMalleableVar("com_maxfps")) fps->current.integer = slideSavedFPS;
    }
    automation.pendingSlide = automation.currentlySliding = automation.keybindPressed = false;
    slideArmedAt = slideStartedAt = 0;
    slideSavedFPS = slideActiveMode = 0;
    slideSawAir = slideContact = false;
}
void cg::Mod_A_AutoSliding(pmove_t* pm, pml_t* pml)
{
    const int mode = v::mod_autoslide.GetInt();
    const int key = v::mod_edge_key.GetInt();
    const bool down = key > 0 && key <= 255 && (GetAsyncKeyState(key) < 0);
    const bool pressed = down && !slideKeyDown;
    slideKeyDown = down;
    if (!pm || !pm->ps || !pml || !mode || r::inputCaptured || !VID_ACTIVE || pm->ps->pm_type != PM_NORMAL ||
        (slideActiveMode && slideActiveMode != mode)) {
        Mod_A_CancelSliding(); return;
    }
    const DWORD now = Sys_MilliSeconds();
    if (pressed || automation.keybindPressed) {
        automation.keybindPressed = false;
        if (automation.pendingSlide || automation.currentlySliding) { Mod_A_CancelSliding(); return; }
        automation.pendingSlide = true; slideArmedAt = now;
    }
    if (automation.currentlySliding) {
        if (slideActiveMode == 2) {
            if (auto* fps = Dvar_FindMalleableVar("com_maxfps")) fps->current.integer = 15;
        }
        const DWORD duration = slideActiveMode == 1 ? 100 : 250;
        if (pml->walking || pm->ps->groundEntityNum != ENTITYNUM_NONE || pml->groundPlane)
            slideContact = true;
        // Keep the slide active through the landing step, not just until it begins.
        if ((slideContact && now - slideStartedAt >= duration) || now - slideStartedAt >= 500)
            Mod_A_CancelSliding();
        return;
    }
    if (!automation.pendingSlide) return;
    if (!pml->walking && pm->ps->groundEntityNum == ENTITYNUM_NONE) slideSawAir = true;
    if (now - slideArmedAt > 15000) { Mod_A_CancelSliding(); return; }
    if (!slideSawAir) return;
    // Predict an approaching surface using the player's bounding box and
    // actual fall velocity; do not start on an arbitrary 100-unit proximity.
    const bool contact = pml->walking || pm->ps->groundEntityNum != ENTITYNUM_NONE || pml->groundPlane;
    const float vz = pm->ps->velocity[2];
    bool landing = contact;
    if (!landing && vz < 0.0f) {
        const float horizon = mode == 2 ? 0.067f : glm::clamp(pml->frametime * 2.0f, 0.006f, 0.040f);
        vec3_t end = {pm->ps->origin[0] + pm->ps->velocity[0] * horizon,
                   pm->ps->origin[1] + pm->ps->velocity[1] * horizon,
                   pm->ps->origin[2] + vz * horizon - 2.0f};
        trace_t trace{};
        PM_playerTrace(pm, &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);
        landing = trace.fraction < 1.0f && !trace.startsolid && !trace.allsolid && trace.normal[2] > 0.0f;
    }
    if (!landing) return;
    slideContact = contact;
    if (mode == 1) {
        if (*reinterpret_cast<const BYTE*>(0x410660) != 0x83) { Mod_A_CancelSliding(); return; }
        hook h; h.write_addr(0x410660, "\xC3", 1); slidePatched = true;
    } else if (mode == 2) {
        auto* fps = Dvar_FindMalleableVar("com_maxfps");
        if (!fps) { Mod_A_CancelSliding(); return; }
        slideSavedFPS = fps->current.integer; fps->current.integer = 15;
    } else { Mod_A_CancelSliding(); return; }
    slideActiveMode = mode; slideStartedAt = now;
    automation.pendingSlide = false; automation.currentlySliding = true;
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