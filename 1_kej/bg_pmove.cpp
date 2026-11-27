#include "pch.h"

bool anim_can_use; // true after touching ground to avoid spam mid-air

BOOL cg::PM_SlideMove(pmove_t* pm, pml_t* pml, int gravity)
{
	BOOL clipped = PM_SlideMove_f(pm, pml, gravity);

	jumpanalyzer.velocity_clipped = clipped > 0;

	return clipped;
}
void cg::PM_WalkMove(pmove_t* pm, pml_t* pml)
{
	Mod_DetermineFPS(pm, pml);

	jumpanalyzer.frametime = pml->frametime;
	jumpanalyzer.msec = pml->msec;

	PM_WalkMove_f(pm, pml);
	jumpanalyzer.walking = pml->walking;

	Mod_A_AdjustRPG(pm, pml);



	
	if((pm->cmd.buttons & 8194) != 0 || (pm->oldcmd.buttons & 8194) != 0)
		anim_can_use = true;

	glob_pm = reinterpret_cast<pmove_t*>(pm);
	glob_pml = reinterpret_cast<pml_t*>(pml);



}
void cg::PM_AirMove(pmove_t* pm, pml_t* pml)
{
	Mod_DetermineFPS(pm, pml);

	jumpanalyzer.frametime = pml->frametime;
	jumpanalyzer.msec = pml->msec;

	PM_AirMove_f(pm, pml);
	jumpanalyzer.walking = pml->walking;

	Mod_HitAnalyzer(pm, pml);
	Mod_JumpAnalyzer(pm, pml);
	Mod_A_AdjustRPG(pm, pml);



	

	//animations
	static DWORD ms = Sys_MilliSeconds(), anim_time = 0;
	static bool activeAnim = false, wait_anim = 0;

	if(anim_can_use)
		pm->ps->weapAnim = 0;

	if (!v::mod_smooth_rpg_anim.isEnabled()) {
		if (v::mod_use_jump_anim.isEnabled() && !activeAnim/* && pm->ps->weapAnim == 0*/ /*&& pm->ps->weapAnim != (WEAP_SPRINT_LOOP | ~pm->ps->weapAnim & 0x200)*/ && anim_can_use && pm->ps->jumpTime + 250 < pm->cmd.serverTime) {
			pm->ps->weapAnim = 0;
			pm->ps->weapAnim = v::mod_jump_anim.GetInt() | ~pm->ps->weapAnim & 0x200;
			ms = Sys_MilliSeconds();
			activeAnim = true;
			anim_can_use = false;
		}
		else if (activeAnim && ms + 2 < Sys_MilliSeconds()) {
			pm->ps->weapAnim = 0;
			activeAnim = false;

		}
	}
	else if (v::mod_smooth_rpg_anim.isEnabled()) {
		
		if (anim_can_use || wait_anim) {

			if (!wait_anim && (clients->snap.ps.weapon == BG_FindWeaponIndexForName("rpg_mp") || clients->snap.ps.weapon == BG_FindWeaponIndexForName("rpg_sustain_mp"))) {
				anim_can_use = false;
				//CG_SelectWeaponIndex(BG_FindWeaponIndexForName("deserteaglegold_mp"), cgs->clientNum);
				SendCommand("weapnext");
				anim_time = Sys_MilliSeconds();
				wait_anim = true;
			}
			
			else if(anim_time != 0) {
				if (Sys_MilliSeconds() - anim_time > 2) {
					CG_SelectWeaponIndex(BG_FindWeaponIndexForName(v::mod_rpg_mode.GetString() == "sustain" ? "rpg_sustain_mp" : "rpg_mp"), cgs->clientNum);
					wait_anim = false;
					anim_time = 0;
				}
			}


		}

	}



	glob_pm = reinterpret_cast<pmove_t*>(pm);
	glob_pml = reinterpret_cast<pml_t*>(pml);

}
void cg::PM_UFOMove(pmove_t* pmm, pml_t* pmll)
{
	static pml_t* pml;
	static pmove_t* pm;
	__asm	mov pml, edx;
	__asm	mov pm, eax;

	PM_UFOMove_f(pmm, pmll);

	Mod_DetermineFPS(pm, pml);


	jumpanalyzer.commandTime = pm->ps->commandTime;
	jumpanalyzer.serverTime = pm->cmd.serverTime;
	jumpanalyzer.hugging_bounce = false;

	return;
}
void cg::PM_Weapon(pml_t* pmll, pmove_t* pmm)
{
	static pml_t* pml;
	static pmove_t* pm;
	__asm	mov pml, eax;
	__asm	mov pm, ebp;

	PM_Weapon_f(pml, pm);

	if (pm && pml) {
		Mod_RPGAnalyzer(pm, pml);
	}

	return;
}
void cg::Pmove(pmove_t* pm)
{

	dvar_s* com_maxfps = Dvar_FindMalleableVar("com_maxfps");

	if (!com_maxfps) {
		Com_PrintError(CON_CHANNEL_OBITUARY, "please use com_maxfps as your dvar name\n");
		return;
	}
	const int ref_fps = com_maxfps->current.integer;

	int _msec;

	_msec = 1000 / (ref_fps == 0 ? 1 : ref_fps);

	if (_msec == NULL)
		_msec = 1;

	if (v::mod_pmove_fixed.isEnabled())
		pm->cmd.serverTime = ((pm->cmd.serverTime + (_msec < 2 ? 2 : _msec) - 1) / _msec) * _msec;

	int finalTime = pm->cmd.serverTime;

	if (finalTime < pm->ps->commandTime) {
		return; // should not happen
	}

	if (finalTime > pm->ps->commandTime + 1000) {
		pm->ps->commandTime = finalTime - 1000;
	}
	pm->numtouch = 0;

	while (pm->ps->commandTime != finalTime) {
		int msec;

		msec = finalTime - pm->ps->commandTime;

		if (v::mod_pmove_fixed.isEnabled()) {
			if (msec > _msec) {
				msec = _msec;
			}
		}
		else {
			if (msec > 66)
				msec = 66;
		}

		pm->cmd.serverTime = pm->ps->commandTime + msec;
		PmoveSingle(pm);

		//jumpanalyzer.commandTime = pm->ps->commandTime;
		jumpanalyzer.serverTime = pm->cmd.serverTime;
		memcpy(&pm->oldcmd, &pm->cmd, sizeof(pm->oldcmd));


	}
}


void cg::PM_ModCode(pml_t* pml, pmove_t* pm)
{
	//this part of the code is called after PM_Weapon()

	jumpanalyzer.commandTime = pm->ps->commandTime;
	jumpanalyzer.serverTime = pm->cmd.serverTime;
	Mod_A_AutoSliding(pm, pml);
	//Mod_A_Strafebot(pm);

	return;
}
void cg::Mod_DetermineFPS(pmove_t* pm, pml_t* pml)
{
	//
	//vec3_t forward, right, wishdir;
	//vec2_t wishvel;
	//vec_t commandScale;

	playerState_s* ps = pm->ps;
	//usercmd_s cmd = pm->cmd;

	//const char forwardmove = pm->cmd.forwardmove;
	//const char rightmove = pm->cmd.rightmove;

	//static const DWORD cmdscale = 0x40EE60;

	//__asm //get command scale
	//{
	//	mov edi, ps;
	//	lea esi, cmd;
	//	call cmdscale;
	//	fstp [commandScale];
	//}


	//VectorCopy(pml->forward, forward);
	//VectorCopy(pml->right, right);

	//forward[2] = 0;
	//right[2] = 0;
	//VectorNormalize(forward);
	//VectorNormalize(right);
	//
	//wishvel[0] = forward[0] * forwardmove + right[0] * rightmove;
	//wishvel[1] = forward[1] * forwardmove + right[1] * rightmove;

	//if (GetAsyncKeyState(VK_END) & 1)
	//	printf("forward[0]: %.6f\n", pml->forward[0]);

	//wishdir[0] = wishvel[0];
	//wishdir[1] = wishvel[1];
	//wishdir[2] = 0.0f;

	//float wishspeed = VectorNormalize(wishdir) * commandScale;

	//const auto Accelerate = [](vec3_t wishDir, pml_t* pml, float wishspeed, float accel, playerState_s* ps) -> void
	//{
	//	float currentSpeed = ps->velocity[1] * wishDir[1] + *wishDir * ps->velocity[0] + ps->velocity[2] * wishDir[2];
	//	float addspeed = wishspeed - currentSpeed;
	//	float _wishspeed, accelspeed;

	//	const dvar_s* stopspeed = Dvar_FindMalleableVar("stopspeed");
	//	airmove.addspeed = addspeed;

	//	if (addspeed > 0.0)
	//	{
	//		//_wishspeed = stopspeed->current.value;



	//		accelspeed = pml->frametime * accel * wishspeed;
	//		if (accelspeed > addspeed)
	//		{
	//			accelspeed = addspeed;
	//		}
	//		airmove.accelspeed = accelspeed;
	//		VectorCopy(wishDir, airmove.wishDir);


	//	}


	//};

	static bool rightmove{};

	const usercmd_s* cmd = cinput->GetUserCmd(cinput->currentCmdNum - 1);



	if (cmd->rightmove > 0)
		rightmove = true;
	else if (cmd->rightmove < 0)
		rightmove = false;


	
	FPS_CalculateSingleBeatDirection(rightmove, cmd);

	fps_zones.fps125 = round((float)ps->speed / 8) + 10;
	fps_zones.fps200 = round((float)ps->speed / 5) + 10;
	fps_zones.fps250 = round((float)ps->speed / 4) + 10;
	fps_zones.fps333 = round((float)ps->speed / 3) + 10;
	
	const bool long125 = v::mod_autoFPS_long125.isEnabled() && !(v::mod_strafebot.isEnabled() && NOT_GROUND &&
        (v::mod_auto_para.isEnabled() || v::mod_auto_250.isEnabled()));
	const float diff = ((float)ps->speed / 190.f);



	if(!rightmove)
		fps_zones.fps333 -= long125 == true ? 17.f * diff : 0;



	fps_zones.length125 = (90.f - fps_zones.fps125) - fps_zones.fps125; //this way around because 125 starts from < 45
	fps_zones.length200 = fps_zones.fps200 - (90.f - fps_zones.fps200) + 3.f; //will fill the empty space with 200fps
	fps_zones.length250 = fps_zones.fps250 - (90.f - fps_zones.fps250);
	fps_zones.length333 = fps_zones.fps333 - (90.f - fps_zones.fps333);

	fps_zones.fps125 -= fps_zones.length125;
	fps_zones.fps250 += fps_zones.length250 - (11.f);
	fps_zones.fps200 =  fps_zones.fps250 + (20.f);
	fps_zones.fps333 -= fps_zones.length333;

	fps_zones.length250 -= 6.f; //hide overlap
	fps_zones.length125 -= long125 == false ? 17.f : 0;

	if	(!rightmove)	
		fps_zones.length333 += (long125 == true ? 17.f : 0);
	else	
		fps_zones.length333 -= (long125 == true ? 17.f : 0);

	return;
	//return Accelerate(wishdir, pml, wishspeed, 1.f, ps);

}
__declspec(naked) void cg::PmoveSingle_stub()
{
	const static DWORD _jmp = 0x414BBB;
	__asm
	{

		push ebx;
		lea ecx, [esp + 0x28]; 
		push ecx;
		call PM_ModCode;
		add esp, 0x8;
		mov eax, ebx;
		mov esi, 0x412390;
		call esi;
		jmp _jmp;

	}
}
//MISLEADING FUNCTION NAME!!!!!
void cg::PM_OverBounce(pmove_t* pm, pml_t* pml)
{

	//Mod_A_Strafebot(pm);

	vec3_t move;

	move[0] = pm->ps->origin[0] - pml->previous_origin[0];
	move[1] = pm->ps->origin[1] - pml->previous_origin[1];
	move[2] = pm->ps->origin[2] - pml->previous_origin[2];

	float dot = move[2] * move[2] + move[1] * move[1] + move[0] * move[0];
	float dot_div_frametime = dot / (pml->frametime * pml->frametime);
	float dot_speed = pm->ps->velocity[2] * pm->ps->velocity[2] + pm->ps->velocity[1] * pm->ps->velocity[1] + pm->ps->velocity[0] * pm->ps->velocity[0];

	if (dot_speed * 0.25 > dot_div_frametime)
	{
		//Com_Printf(CON_CHANNEL_OBITUARY, "possible overbounce!\n");
		float inGameFramesPerSecond = 1.0f / pml->frametime;
		pm->ps->velocity[0] = inGameFramesPerSecond * move[0];
		pm->ps->velocity[1] = inGameFramesPerSecond * move[1];
		pm->ps->velocity[2] = inGameFramesPerSecond * move[2];
	}

	float clampedFrametime = glm::clamp(pml->frametime, 0.f, 1.f);

	float diffX = pm->ps->velocity[0] - pm->ps->oldVelocity[0];
	float diffY = pm->ps->velocity[1] - pm->ps->oldVelocity[1];

	float frameX = clampedFrametime * diffX;
	float frameY = clampedFrametime * diffY;

	pm->ps->oldVelocity[0] = pm->ps->oldVelocity[0] + frameX;
	pm->ps->oldVelocity[1] = pm->ps->oldVelocity[1] + frameY;

	static float previousZ = pm->ps->jumpOriginZ;

	if(pm->ps->jumpOriginZ != NULL)
		previousZ = pm->ps->jumpOriginZ;

	if (v::mod_unlim_bounces.isEnabled()) {
		pm->ps->pm_flags = pm->ps->pm_flags & 0xFFFFFE7F | PMF_JUMPING; //reset bouncing flags
		pm->ps->jumpOriginZ = previousZ;
	}
	//((void(__cdecl*)(float* vec))0x578FE0)(pm->ps->velocity); //Sys_SnapVector

	jumpanalyzer.pm_flags = pm->ps->pm_flags;
	jumpanalyzer.jumpOriginZ = pm->ps->jumpOriginZ;
	jumpanalyzer.jumpTime = pm->ps->jumpTime;
	
	static bool was_airborne(false);

	if (was_airborne && pml->walking) {
		jumpanalyzer.groundTime = jumpanalyzer.serverTime;
		was_airborne = false;
	}

	if (pml->walking) {
		if (was_airborne) {
			jumpanalyzer.groundTime = jumpanalyzer.serverTime;
			was_airborne = false;
		}
		jumpanalyzer.airTime = jumpanalyzer.serverTime;
	}
	else {
		jumpanalyzer.groundTime = jumpanalyzer.serverTime;
		was_airborne = true;
	}
	return;

}
__declspec(naked) void cg::PM_OverBounce_stub()
{
	const static DWORD _jmp = 0x414BC8;

	__asm
	{
		fld     dword ptr[ebp + 1Ch];
		fsub		[esp + 0x90];
		lea     esi, [ebp + 28h];
		
		lea ecx, [esp + 0x28];
		push ecx;
		push ebx;
		call PM_OverBounce;
		add esp, 0x8;
		jmp _jmp;
		//retn;
	}
}
static bool __cdecl TerrainBouncesEnabled()
{
    return v::mod_terrain_bounces.isEnabled();
}
// This hook replaces a register-based return sequence, not a C++ function call.
__declspec(naked) void cg::CM_IsEdgeWalkable()
{
    __asm {
        pushfd
        pushad
        call TerrainBouncesEnabled
        test al, al
        jz keepOriginal
        mov byte ptr [esp + 28], 0
        jmp done
    keepOriginal:
        mov al, byte ptr [esp + 20]
        mov byte ptr [esp + 28], al
    done:
        popad
        popfd
        ret
    }
}
void cg::BG_AddPredictableEventToPlayerstate(playerState_s* ps, unsigned __int8 newEvent, unsigned int eventParm)
{
	ps->events[ps->eventSequence & 3] = newEvent;
	ps->eventParms[ps->eventSequence & 3] = eventParm;
	ps->eventSequence = (ps->eventSequence + 1);
}

void cg::PM_ProjectVelocity(float* _velOut, float* _velIn, float* _normal)
{

	float _lengthSq2D; // st7
	float newVelLength; // st6
	float velLength; // st7
	float _lengthScale; // st7
	float lengthSq2D; // [esp+8h] [ebp-14h]
	float newZ; // [esp+8h] [ebp-14h]
	float _newZ; // [esp+8h] [ebp-14h]
	float VelocityLength; // [esp+Ch] [ebp-10h]
	float _newVelLength; // [esp+Ch] [ebp-10h]
	float division; // [esp+Ch] [ebp-10h]
	float lengthScale; // [esp+Ch] [ebp-10h]
	float velX; // [esp+10h] [ebp-Ch]
	float velY; // [esp+14h] [ebp-8h]

	if (!v::mod_ez_bounces.isEnabled())
		return PM_ProjectVelocity_f(_velOut, _velIn, _normal);

	float *normal = 0, *velOut, *velIn;

	__asm mov normal, edi;
	__asm mov velIn, esi;
	__asm mov velOut, esi;

	lengthSq2D = velIn[1] * velIn[1] + *velIn * *velIn;
	_lengthSq2D = lengthSq2D;

	if (lengthSq2D == 0.0 || normal[2] < 0.001f)
	{
		*velOut = *velIn;
		velOut[1] = velIn[1];
		velOut[2] = velIn[2];
		return;
	}

	newZ = normal[1] * velIn[1] + *velIn * *normal;
	_newZ = -newZ / normal[2];             
	velX = *velIn;
	velY = velIn[1];
	VelocityLength = velIn[2] * velIn[2] + _lengthSq2D;
	newVelLength = _lengthSq2D + _newZ * _newZ;
	velLength = VelocityLength;
	_newVelLength = newVelLength;
	division = velLength / _newVelLength;   
	lengthScale = sqrtf(division);
	_lengthScale = lengthScale;

	if (lengthScale < 10000 || _newZ < 0.0 || velIn[2] > 0.0)
	{
			velOut[2] = _lengthScale * _newZ;

		if (lengthScale < 1.0) {
			*velOut = velX * _lengthScale;
			velOut[1] = velY * _lengthScale;
			velOut[2] = _lengthScale * _newZ;

		}

	}

	return;

}
float cg::PM_CanBeBounced(float* normal, float* velIn)
{
	float _lengthSq2D; // st7
	float newVelLength; // st6
	float velLength; // st7
	float _lengthScale; // st7
	float lengthSq2D; // [esp+8h] [ebp-14h]
	float newZ; // [esp+8h] [ebp-14h]
	float _newZ; // [esp+8h] [ebp-14h]
	float VelocityLength; // [esp+Ch] [ebp-10h]
	float _newVelLength; // [esp+Ch] [ebp-10h]
	float division; // [esp+Ch] [ebp-10h]
	float lengthScale; // [esp+Ch] [ebp-10h]
	float velX; // [esp+10h] [ebp-Ch]
	float velY; // [esp+14h] [ebp-8h]

	lengthSq2D = velIn[1] * velIn[1] + *velIn * *velIn;
	_lengthSq2D = lengthSq2D;

	if (lengthSq2D == 0.0 || normal[2] < 0.001f)
	{
		return 0;
	}

	newZ = normal[1] * velIn[1] + *velIn * *normal;
	_newZ = -newZ / normal[2];
	velX = *velIn;
	velY = velIn[1];
	VelocityLength = velIn[2] * velIn[2] + _lengthSq2D;
	newVelLength = _lengthSq2D + _newZ * _newZ;
	velLength = VelocityLength;
	_newVelLength = newVelLength;
	division = velLength / _newVelLength;
	lengthScale = sqrtf(division);
	_lengthScale = lengthScale;

	if (lengthScale < 1 || _newZ < 0.0 || velIn[2] > 0.0)
	{
		return lengthScale * 100;

	}

	return 0;
}
void scaler(float* val)
{
	//constexpr int ah = sizeof("aaa");
	*val = v::mod_bounce_height.evar->floatValue;
}
__declspec(naked) void cg::PM_BounceHeight()
{
	static const DWORD _jmp_og = 0x41586A;
	static float _scale(1.f);
	static float huh(0);
	__asm {

		lea ecx, _scale;
		push ecx;
		call scaler;
		add esp, 0x4;
		fld dword ptr[esp + 78h];
		fmul _scale;
		fstp dword ptr[ebp + 08h];
		jmp _jmp_og;

	}
	//return;
}
__declspec(naked) void cg::PM_BounceSteepness()
{
	static const DWORD _jmp = 0x41593D;
	static const float min_angle(0.05f);
	__asm
	{

		mov ecx, offset[v::mod_bounce_angle];
		mov eax, dword ptr[ecx];
		//cmp dword ptr[eax + 8h], 1;
		mov ecx, dword ptr[eax + 8h];
		cmp ecx, 1;
		jz _isEnabled;

		fld ds:[6D8D84h];
		fcomp dword ptr[esp + 00000090h];
		jmp _jmp;
		//fnstsw ax;
		//test ah, 41h;
		//jne short _jnz;
		//fstp st(0);

	_isEnabled:
		fld ds : [min_angle] ;
		fcomp dword ptr[esp + 00000090h];

		jmp _jmp;
	}
}