#include "pch.h"

namespace {
    // Keep an absolute, unwrapped copy of the FPS bands for a W-only run.
    // Repeating the phase every 90 degrees during a run would permit another
    // world-axis sector to be selected after the original band is exhausted.
    struct AirBand {
        double lo, hi;
        bool Contains(double x) const { return hi > lo && x >= lo && x < hi; }
    };
    struct AirBands {
        AirBand fps333, fps250;
        int At(double x) const {
            return fps333.Contains(x) ? 333 : fps250.Contains(x) ? 250 : 0;
        }
    };
    AirBands MakeBands(int speed, int side, double origin) {
        const double edge333 = std::round(speed / 3.0) + 10.0;
        const double edge250 = std::round(speed / 4.0) + 10.0;
        const double len250 = 2.0 * edge250 - 96.0;
        const double start250 = 3.0 * edge250 - 101.0;
        const double left250 = side > 0 ? 90.0 - start250 - len250 : start250;
        return {{origin + 90.0 - edge333, origin + edge333},
                {origin + left250, origin + left250 + len250}};
    }
    // Choose the copy containing the starting angle, or the nearest copy if
    // it lies in a gap. Never choose another copy until manual input resumes.
    double FindBandOrigin(double yaw, int side, int speed) {
        const double base = std::floor(yaw / 90.0) * 90.0;
        double origin = base, best = 1e30;
        for (int i = -1; i <= 1; ++i) {
            const double candidate = base + i * 90.0;
            if (MakeBands(speed, side, candidate).fps333.Contains(yaw)) return candidate;
        }
        for (int i = -1; i <= 1; ++i) {
            const double candidate = base + i * 90.0;
            const auto b = MakeBands(speed, side, candidate);
            if (b.fps250.Contains(yaw)) return candidate;
            for (const AirBand band : {b.fps333, b.fps250}) {
                if (band.hi <= band.lo) continue;
                const double d = yaw < band.lo ? band.lo - yaw : yaw > band.hi ? yaw - band.hi : 0.0;
                if (d < best) { best = d; origin = candidate; }
            }
        }
        return origin;
    }
    double AngleOffset(int forward, int strafe, int side) {
        // A W-only continuation preserves the previous diagonal's displayed
        // sector. Do not introduce a new +/-45-degree phase on release.
        if (forward == 127 && strafe == 0) return 0.0;
        return std::atan2(-static_cast<double>(strafe), static_cast<double>(forward)) * 57.29577951308232
            + (side > 0 ? 45.0 : -45.0);
    }
    int RestrictedZoneFPS(float yaw, int forward, int strafe, int side, int speed) {
        if (!std::isfinite(yaw) || speed <= 0 || side == 0) return 0;
        const double phase = yaw + AngleOffset(forward, strafe, side);
        const double base = std::floor(phase / 90.0) * 90.0;
        bool in250 = false;
        for (int i = -1; i <= 1; ++i) {
            const int fps = MakeBands(speed, side, base + 90.0 * i).At(phase);
            if (fps == 333) return 333;
            in250 = in250 || fps == 250;
        }
        return in250 ? 250 : 0;
    }
    bool AirborneForStrafe() {
        const auto& ps = cgs->predictedPlayerState;
        return ps.pm_type == PM_NORMAL && ps.groundEntityNum == ENTITYNUM_NONE &&
            !(ps.pm_flags & (PMF_MANTLE | PMF_LADDER | PMF_DUCKED | PMF_PRONE));
    }
    // The two bands are traversed separately, rather than waiting for the
    // end of their union. Do not reselect a neighbouring 90-degree sector.
    AirBand PhaseBand(const AirBands& bands, int fps, int side, double origin) {
        AirBand band = fps == 250 ? bands.fps250 : bands.fps333;
        if (fps == 250) {
            if (side < 0) band.lo = glm::max(band.lo, bands.fps333.hi);
            else band.hi = glm::min(band.hi, bands.fps333.lo);
        }
        band.lo = glm::max(band.lo, origin);
        band.hi = glm::min(band.hi, origin + 90.0);
        return band;
    }
    struct AutoSector {
        bool active = false, reversePending = false, transferPending = false;
        int side = 0, fps = 0;
        double origin = 0.0, lastYaw = 0.0;
        DWORD phaseStart = 0;
        void Reset() { active = reversePending = transferPending = false; side = fps = 0; phaseStart = 0; }
        bool Advance() {
            if (transferPending) {
                transferPending = false; fps = 250; phaseStart = 0; return false;
            }
            if (!reversePending) return false;
            reversePending = false; side = -side; fps = 333; phaseStart = 0;
            return true;
        }
        double Unwrap(double yaw) const { return lastYaw + std::remainder(yaw - lastYaw, 360.0); }
        void Begin(double yaw, int direction, int speed) {
            Reset(); active = true; side = direction; lastYaw = yaw;
            origin = FindBandOrigin(yaw, direction, speed);
            fps = 333;
        }
    } autoSector;
    struct BounceGate {
        int lastStep = -1, pendingContact = -1;
        bool bounced = false;
    } bounceGate;
    bool Auto250Allowed() {
        return v::mod_auto_250.isEnabled() && v::mod_autoFPS.isEnabled() &&
            !(v::mod_only3232.isEnabled() && bounceGate.bounced);
    }
}

bool cg::Mod_A_Only3232Active() {
    const auto& ps = cgs->predictedPlayerState;
    return v::mod_only3232.isEnabled() && v::mod_autoFPS.isEnabled() && !bounceGate.bounced &&
        ps.pm_type == PM_NORMAL && ps.groundEntityNum == ENTITYNUM_NONE;
}
bool cg::Mod_A_RestrictedFPS() {
    return Mod_A_Only3232Active() ||
        (AirborneForStrafe() && v::mod_strafebot.isEnabled() && Auto250Allowed());
}
void cg::Mod_A_Reset3232() { bounceGate = {}; }
void cg::Mod_A_Record3232Contact(int commandTime) {
    if (commandTime > bounceGate.lastStep) bounceGate.pendingContact = commandTime;
}
void cg::Mod_A_Observe3232Step(pmove_t* pm, int previousGround) {
    if (!pm || !pm->ps || pm->ps != &cgs->predictedPlayerState || clc.demoplaying) return;
    const int time = pm->ps->commandTime;
    if (pm->ps->pm_type != PM_NORMAL) { Mod_A_Reset3232(); return; }
    if (time <= bounceGate.lastStep) return;
    bounceGate.lastStep = time;
    if (pm->ps->groundEntityNum != ENTITYNUM_NONE || previousGround != ENTITYNUM_NONE)
        bounceGate.bounced = false;
    if (pm->ps->groundEntityNum == ENTITYNUM_NONE && bounceGate.pendingContact == time)
        bounceGate.bounced = true;
    if (bounceGate.pendingContact <= time) bounceGate.pendingContact = -1;
}

void cg::Mod_A_ResetStrafeDirection()
{
    automation.paraSide = 0;
    automation.paraFPS = 0;
    automation.paraActive = false;
    autoSector.Reset();
}

int cg::Mod_A_AutoStrafeSide(bool airborne)
{
    if (r::inputCaptured || clc.demoplaying || !v::mod_strafebot.isEnabled() ||
        (GetAsyncKeyState(v::mod_strafebot_ow.GetInt()) < 0)) {
        Mod_A_ResetStrafeDirection();
        return 0;
    }
    const bool w = GetAsyncKeyState('W') < 0;
    const bool a = GetAsyncKeyState('A') < 0;
    const bool d = GetAsyncKeyState('D') < 0;
    const bool back = GetAsyncKeyState('S') < 0;
    if (a != d && !back && (w || (airborne && v::mod_halfbeat.isEnabled()))) {
        autoSector.Reset();
        automation.paraSide = d ? 1 : -1;
    }
    if (!w || back) {
        autoSector.Reset();
        if (!(airborne && v::mod_halfbeat.isEnabled() && a != d && !back)) automation.paraSide = 0;
        return 0;
    }
    if (!airborne || !Auto250Allowed()) {
        autoSector.Reset(); return 0;
    }
    if (a || d || !bhopHookInstalled) return 0;
    return automation.paraSide;
}

void cg::Mod_A_Strafebot()
{
    static DWORD lastGroundUpdate = 0;
    static float groundBlend = 0.0f;
    static int lastGroundSide = 0;
    static DWORD lastAirUpdate = 0;
    const auto resetGround = [&]() {
        lastGroundUpdate = 0;
        groundBlend = 0.0f;
        lastGroundSide = 0;
    };
	if (r::inputCaptured || !v::mod_strafebot.isEnabled() || clients->snap.ps.pm_type == PM_UFO || v::mod_strafebot.isEnabled() && (GetAsyncKeyState(v::mod_strafebot_ow.GetInt()) < 0) == true)
	{ automation.paraActive = false; automation.paraFPS = 0; automation.paraSide = 0; resetGround(); autoSector.Reset(); lastAirUpdate = 0; return; }

	const usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);
    if (!cmd) return;
    // Use live keys for the mode gate; the old command may be one frame behind.
    const bool leftHeld = GetAsyncKeyState('A') < 0;
    const bool rightHeld = GetAsyncKeyState('D') < 0;
    const bool airborne = AirborneForStrafe() && NOT_GROUND;
    const bool wHeld = GetAsyncKeyState('W') < 0;
    const bool sHeld = GetAsyncKeyState('S') < 0;
    // Read the actual keys, not the previous movement byte, when deciding
    // whether a key combination is allowed. A/D alone is Halfbeat only.
    const char forward = wHeld != sHeld ? (wHeld ? 127 : -127) : 0;
    const char sideMove = leftHeld != rightHeld ? (rightHeld ? 127 : -127) : 0;
    const bool diagonalInput = forward == 127 && sideMove != 0;
    const bool halfbeat = airborne && v::mod_halfbeat.isEnabled() && !wHeld && !sHeld && sideMove != 0;
    // Commit a requested reversal before calculating the new side's angle.
    // CL_FinishMove observes this same side on the outgoing command.
    if (airborne && wHeld && !sHeld && !leftHeld && !rightHeld && autoSector.active && Auto250Allowed()) {
        if (autoSector.Advance()) {
            automation.paraSide = autoSector.side;
            ++automation.auto250Switches;
        }
    }
    const int autoSide = Mod_A_AutoStrafeSide(airborne);
    const bool para = autoSide != 0 && airborne && forward == 127 && !leftHeld && !rightHeld;
    const char effectiveSide = para ? static_cast<char>(autoSide * 127) : sideMove;
    automation.paraActive = para;
    if (!para) { autoSector.Reset(); automation.paraFPS = 0; }
    if ((!airborne && !GROUND) || (!diagonalInput && !halfbeat && !para)) {
        resetGround();
        lastAirUpdate = 0;
        return;
    }
    // In a restricted air mode the angle solver must use the same limit as
    // the zone selector (never a stale 125/200 or an explicit override).
    int angleFPS = 0;
    if (Mod_A_RestrictedFPS() && v::mod_autoFPS.isEnabled() && !automation.currentlySliding) {
        const float yaw = clients->cgameViewangles[YAW];
        auto* limit = Dvar_FindMalleableVar("com_maxfps");
        const int fallbackFPS = limit && limit->current.integer == 250 ? 250 : 333;
        int fps = RestrictedZoneFPS(yaw, forward, effectiveSide, automation.paraSide, clients->snap.ps.speed);
        if (para && std::isfinite(yaw) && clients->snap.ps.speed > 0) {
            if (!autoSector.active || autoSector.side != automation.paraSide)
                autoSector.Begin(yaw, automation.paraSide, clients->snap.ps.speed);
            fps = autoSector.fps;
            automation.paraFPS = fps;
        }
        if (!fps) fps = fallbackFPS;
        jumpanalyzer.recommendedFPS = fps;
        // The solver can use the intended cap without writing the Dvar.
        // AutoFPS submits the final cap through the engine command buffer.
        angleFPS = fps;
    }
    if (NOT_GROUND)
        resetGround();

	float optYaw, test;

	if (std::isnan(clients->viewangles[YAW]))
		clients->viewangles[YAW] = 0;
	else if (clients->viewangles[YAW] > 3000 || clients->viewangles[YAW] < -3000)
		clients->viewangles[YAW] = fmodf(clients->viewangles[YAW], 360);

	optYaw = getOptAngle(test, halfbeat, forward, effectiveSide, angleFPS);
	if (optYaw == -400.f || !std::isfinite(optYaw)) {
        resetGround();
        return;
    }

    if (GROUND) {
        lastAirUpdate = 0;
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
        const bool settled = groundBlend >= 1.0f;
        const float dt = glm::min(0.025f, (now - lastGroundUpdate) * 0.001f);
        lastGroundUpdate = now;
        groundBlend = glm::min(1.0f, groundBlend + dt / 0.35f);
        const float yaw = clients->cgameViewangles[YAW];
        const float error = AngleNormalize180(optYaw - yaw);
        // No jump to the full angle when the start ramp expires. Only an
        // intentional side change after engagement gets immediate correction.
        const float maxStep = (30.0f + 210.0f * groundBlend) * dt;
        const float correction = switched && settled ? error :
            glm::clamp(error, -maxStep, maxStep);
        clients->viewangles[YAW] += correction;
        return;
    }

    const DWORD now = Sys_MilliSeconds();
    const int side = automation.paraSide;
    const bool continuous = lastAirUpdate && now - lastAirUpdate <= 100;
    const float dt = continuous ? glm::clamp((now - lastAirUpdate) * 0.001f, 0.0f, 0.025f) : 0.003f;
    lastAirUpdate = now;
    const float yaw = clients->cgameViewangles[YAW];
    const float error = AngleNormalize180(optYaw - yaw);
    if (para) {
        if (!std::isfinite(yaw) || clients->snap.ps.speed <= 0) return;
        if (!autoSector.active || autoSector.side != side)
            autoSector.Begin(yaw, side, clients->snap.ps.speed);
        const double unwrapped = autoSector.Unwrap(yaw);
        const auto bands = MakeBands(clients->snap.ps.speed, side, autoSector.origin);
        const AirBand allowed = PhaseBand(bands, autoSector.fps, side, autoSector.origin);
        constexpr double margin = 0.08;
        if (allowed.hi - allowed.lo <= 2 * margin) {
            // A custom g_speed may not have a valid 250 band. Do not spin
            // into an adjacent sector or request an impossible angle.
            autoSector.Reset(); automation.paraActive = false; automation.paraFPS = 0;
            return;
        }
        const double direction = -static_cast<double>(side);
        const double edge = direction > 0 ? allowed.hi - margin : allowed.lo + margin;
        const auto* cap = Dvar_FindMalleableVar("com_maxfps");
        const bool inside = allowed.Contains(unwrapped);
        const bool capApplied = cap && cap->current.integer == autoSector.fps;
        if (!capApplied) autoSector.phaseStart = 0;
        if (inside && capApplied && !autoSector.phaseStart) autoSector.phaseStart = now;
        const DWORD elapsed = autoSector.phaseStart ? now - autoSector.phaseStart : 0;
        // Shorten each leg: 333 for at least 65 ms, then 250 for 40 ms.
        // If optimal yaw stalls, guide toward the 333 boundary after 95 ms.
        // Actual cap must have been applied before any phase can complete.
        double target = glm::clamp(unwrapped + error, allowed.lo + margin, allowed.hi - margin);
        if (autoSector.fps == 333 && elapsed >= 95) target = edge;
        if (autoSector.fps == 250) {
            const double nearEdge = direction > 0 ? allowed.lo + margin : allowed.hi - margin;
            target = glm::clamp(target, glm::min(nearEdge, edge), glm::max(nearEdge, edge));
        }
        const float delta = static_cast<float>(target - unwrapped);
        const float bounded = glm::clamp(delta, -720.0f * dt, 720.0f * dt);
        clients->viewangles[YAW] += bounded;
        autoSector.lastYaw = unwrapped + bounded;
        automation.paraFPS = autoSector.fps;
        if (autoSector.fps == 333 && elapsed >= 65 &&
            std::fabs(autoSector.lastYaw - edge) <= 0.16) {
            const AirBand next = PhaseBand(bands, 250, side, autoSector.origin);
            if (next.hi - next.lo > 2 * margin) autoSector.transferPending = true;
        } else if (autoSector.fps == 250 && elapsed >= 40 && allowed.Contains(autoSector.lastYaw)) {
            autoSector.reversePending = true;
        }
        return;
    }
    // No configurable or time-dependent smoothing on manual air strafes.
    // Apply the shortest correction in one update, including Halfbeat.
    clients->viewangles[YAW] += error;

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

	// box500 overrides ordinary AutoFPS; the restricted air modes take precedence.
	if (box500.use500) {
		fps = 500;
	}

	// Ground jump height and ground strafe gain.
	if (v::mod_autoFPS.isEnabled() && !box500.use500 && GROUND)
		fps = v::mod_only3232.isEnabled() ? 333 : (spaceHeld ? 125 : 333);

    if (v::mod_autoFPS.isEnabled() && Mod_A_RestrictedFPS() && (!r::inputCaptured || Mod_A_Only3232Active())) {
        const bool left = GetAsyncKeyState('A') < 0;
        const bool right = GetAsyncKeyState('D') < 0;
        const int strafe = left != right ? (right ? 127 : -127) :
            (automation.paraActive ? automation.paraSide * 127 : 0);
        const int forward = GetAsyncKeyState('W') < 0 ? 127 : 0;
        const int side = strafe ? (strafe > 0 ? 1 : -1) : automation.paraSide;
        const int zone = RestrictedZoneFPS(clients->cgameViewangles[YAW], forward, strafe, side, clients->snap.ps.speed);
        fps = automation.paraActive && automation.paraFPS ? automation.paraFPS :
            (zone ? zone : (com_maxfps->current.integer == 250 ? 250 : 333));
        jumpanalyzer.recommendedFPS = fps;
    }
	if (v::mod_autoFPS.isEnabled() && v::mod_only3232.isEnabled() && GROUND) fps = 333;
    if (automation.currentlySliding && v::mod_autoslide.GetInt() == 2) fps = 15;
    // Use the same engine command as a manual console FPS change. Do not
    // write current/latched values or fabricate usercmd timings or HUD data.
    if (com_maxfps->type != dvar_type::integer || fps < 0 ||
        fps == com_maxfps->current.integer)
        return;
    // FPS slide has its own existing lifecycle; avoid a queued command
    // surviving its short prediction step and changing the restored cap.
    if (automation.currentlySliding && v::mod_autoslide.GetInt() == 2)
        return;
    static int lastRequested = -1;
    static DWORD lastRequestAt = 0;
    const DWORD now = Sys_MilliSeconds();
    if (lastRequested == fps && now - lastRequestAt < 50)
        return;
    char command[48]{};
    sprintf_s(command, sizeof(command), "com_maxfps %d\n", fps);
    Cbuf_AddText(command, 0);
    lastRequested = fps;
    lastRequestAt = now;
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