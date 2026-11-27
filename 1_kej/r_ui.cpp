#include "pch.h"

// ---------------------------------------------------------------------------
// Cursor suppression while an ImGui menu is open.
//
// The game re-centres the OS cursor every frame (SetCursorPos to the window
// centre) and clips it to a tiny centre rect (ClipCursor) for mouse-look.
// That makes the ImGui cursor jitter in place instead of moving.
//
// We hook both WinAPI calls and, while a menu is open, drop the re-centre
// and only clip to the game window bounds. Toggled from R_RemoveInput().
// ---------------------------------------------------------------------------
namespace
{
    using SetCursorPos_t = BOOL(WINAPI*)(int, int);
    using ClipCursor_t   = BOOL(WINAPI*)(const RECT*);

    SetCursorPos_t SetCursorPos_orig = nullptr;
    ClipCursor_t   ClipCursor_orig   = nullptr;

    // Set by R_RemoveInput(true) when the menu opens, cleared when it closes.
    bool g_suppressCursorRecenters = false;

    BOOL WINAPI SetCursorPos_hook(int X, int Y)
    {
        if (g_suppressCursorRecenters)
            return TRUE;                 // pretend success, don't move

        if (SetCursorPos_orig)
            return SetCursorPos_orig(X, Y);

        return TRUE;                     // defensive: should not happen
    }

    BOOL WINAPI ClipCursor_hook(const RECT* lpRect)
    {
        if (g_suppressCursorRecenters)
        {
            // keep the cursor inside the game window only; drop the small
            // centre-rect clip the game uses for mouse-look
            HWND hwnd = GetActiveWindow();
            if (!hwnd)
                hwnd = GetForegroundWindow();

            if (hwnd)
            {
                RECT rc{};
                if (GetClientRect(hwnd, &rc))
                {
                    POINT tl{ rc.left,  rc.top    };
                    POINT br{ rc.right, rc.bottom };
                    ClientToScreen(hwnd, &tl);
                    ClientToScreen(hwnd, &br);
                    RECT screenRc{ tl.x, tl.y, br.x, br.y };
                    return ClipCursor_orig ? ClipCursor_orig(&screenRc)
                                           : ClipCursor(&screenRc);
                }
            }

            return ClipCursor_orig ? ClipCursor_orig(nullptr)
                                   : ClipCursor(nullptr);
        }

        return ClipCursor_orig ? ClipCursor_orig(lpRect) : ClipCursor(lpRect);
    }

    void Cursor_InstallHooks()
    {
        static bool installed = false;
        if (installed)
            return;
        installed = true;

        if (DetourTransactionBegin() != NO_ERROR)
            return;

        DetourUpdateThread(GetCurrentThread());
        SetCursorPos_orig = (SetCursorPos_t)SetCursorPos;
        ClipCursor_orig   = (ClipCursor_t)ClipCursor;

        DetourAttach((PVOID*)&SetCursorPos_orig, (PVOID)SetCursorPos_hook);
        DetourAttach((PVOID*)&ClipCursor_orig,   (PVOID)ClipCursor_hook);

        if (DetourTransactionCommit() != NO_ERROR)
        {
            fs::Log_Write(LOG_NONE, "failed to hook SetCursorPos/ClipCursor\n");
            SetCursorPos_orig = nullptr;
            ClipCursor_orig   = nullptr;
        }
    }
} // namespace

void r::R_MenuStyle(float transparencyScale)
{
    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.Alpha = transparencyScale;
    st.WindowPadding = ImVec2(10,10); st.FramePadding = ImVec2(8,5);
    st.ItemSpacing = ImVec2(8,7); st.WindowRounding=10; st.ChildRounding=6;
    st.FrameRounding=4; st.GrabRounding=4; st.ScrollbarRounding=4; st.ScrollbarSize=5;
    st.WindowBorderSize=1; st.ChildBorderSize=1; st.FrameBorderSize=1;
    auto* c=st.Colors;
    c[ImGuiCol_WindowBg]=ImVec4(.11f,.11f,.11f,1);
    c[ImGuiCol_ChildBg]=ImVec4(.125f,.125f,.125f,1);
    c[ImGuiCol_Border]=ImVec4(.21f,.21f,.21f,1);
    c[ImGuiCol_FrameBg]=ImVec4(.115f,.115f,.115f,1);
    c[ImGuiCol_FrameBgHovered]=c[ImGuiCol_HeaderHovered]=ImVec4(.19f,.17f,.18f,1);
    c[ImGuiCol_FrameBgActive]=c[ImGuiCol_HeaderActive]=ImVec4(.27f,.22f,.24f,1);
    c[ImGuiCol_Header]=c[ImGuiCol_Button]=ImVec4(.16f,.16f,.16f,1);
    c[ImGuiCol_ButtonHovered]=c[ImGuiCol_HeaderHovered]; c[ImGuiCol_ButtonActive]=c[ImGuiCol_HeaderActive];
    c[ImGuiCol_CheckMark]=c[ImGuiCol_SliderGrab]=c[ImGuiCol_SliderGrabActive]=ImVec4(.69f,.55f,.59f,1);
    c[ImGuiCol_Text]=ImVec4(.95f,.95f,.95f,1); c[ImGuiCol_TextDisabled]=ImVec4(.48f,.48f,.48f,1);

}
void r::IN_ActiveMouse(int active)
{
	const bool on = active != 0;

	if (cod4x_entry) {
		// let cod4x update its own input state too
		const DWORD fnc = (cod4x_entry + 0x79918);
		_asm {
			push active;
			call fnc;
			add esp, 0x4;
		}
	}

	// Always toggle both flags in iw3mp's own mouse state (works with and
	// without cod4x). The per-frame cursor re-centering in the game's
	// IN_MouseMove() is gated on mouseActive, so clearing only
	// mouseInitialized left the game snapping the cursor back to the window
	// centre every frame -> the cursor could not be moved and "jittered in
	// place" while the menu was open.
	s_wmv->mouseActive = on;
	s_wmv->mouseInitialized = on;
}
void r::R_ClearMouseDelta()
{
    clients->mouseDx[0] = clients->mouseDx[1] = 0;
    clients->mouseDy[0] = clients->mouseDy[1] = 0;
}
void r::R_RemoveInput(bool _true, bool save_file)
{

	if (!ImGui::GetCurrentContext()) {
		std::cout << "R_RemoveInput(" << _true << "): called without imgui context!\n";
		Com_PrintError(CON_CHANNEL_CONSOLEONLY, "R_RemoveInput(): called without imgui context!\n");
		return;
	}

	static const DWORD MouseInput = (cod4x_entry != NULL) ? (cg::cod4x_entry + 0x4480E01) : (DWORD)&s_wmv->mouseInitialized;
	static const DWORD KeyInput = 0x4631B0; //CL_KeyMove
	ImGuiIO& io = ImGui::GetIO();

	// write_addr()/nop() never touch instance state, but calling them through
	// a null pointer is undefined behaviour and can be miscompiled
	static hook h;
	hook* a = &h;


	if (_true) {
        if (!inputCaptured) VectorCopy(clients->viewangles, capturedInputAngles);
        inputCaptured = true;
        R_ClearMouseDelta();

		// Cursor: stop the game from re-centering the OS cursor / clipping it
		// to the tiny centre rect while the menu is open. Without this the
		// ImGui cursor jitters in place and cannot be moved, no matter what
		// we do to s_wmv->mouseActive / mouseInitialized.
		Cursor_InstallHooks();
		g_suppressCursorRecenters = true;
		::ClipCursor(nullptr); // release any active clip immediately

		io.MouseDrawCursor = true;
		io.WantCaptureMouse = true;
		
		IN_ActiveMouse(0);
		//a->write_addr(MouseInput, "\x00", 1);
		a->write_addr(KeyInput, "\xC3", 1);
		a->write_addr(0x467EB0, "\xC3", 1); //CL_KeyEvent
		return;
	}

	R_ClearMouseDelta();
    inputCaptured = false;
    g_suppressCursorRecenters = false;

	io.MouseDrawCursor = false;
	io.WantCaptureMouse = false; 
	IN_ActiveMouse(1);

//	a->write_addr(MouseInput, "\x01", 1);
	a->write_addr(KeyInput, "\x51", 1);
	a->write_addr(0x467EB0, "\x8B", 1); //CL_KeyEvent
	if(save_file)
		Evar_SaveToFile(v::cfg::cfgDirectory);

}
bool r::R_OpenMenu(IDirect3DDevice9* pDevice)
{

	if (!ImGui::GetCurrentContext()) 
		return false;

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	static bool once = true;
	if (once || ResetImFont) {
		fs::Log_Write(LOG_NONE, "setting imFont\n");
		ImFont* font = ImGui::GetFont();
		font->Scale = 1.0f;
		ImGui::SetCurrentFont(font);
		ResetImFont = false;

		once = false;
	}
	if (r::should_draw_menu) {
		
        R_MenuStyle();
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 fixedSize((viewport->Size.x * 0.92f < 838.0f ? viewport->Size.x * 0.92f : 838.0f),
                               (viewport->Size.y * 0.90f < 535.0f ? viewport->Size.y * 0.90f : 535.0f));
        ImGui::SetNextWindowSize(fixedSize, ImGuiCond_Always);
        ImGui::Begin("[ALPHA] 1_kej_v2 - github.com/kejjjjj/", &r::should_draw_menu,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
        ImGui::SetWindowFontScale(1.0f);
        if (r::should_draw_menu) R_Features();
        ImGui::End();
        if (!r::should_draw_menu || ImGui::GetIO().KeysDownDuration[VK_ESCAPE] >= 0.0f) {
            r::should_draw_menu = false;
            R_RemoveInput(false);
        }

	}

	return true;
}
void r::R_EndRender()
{
	ImGui::EndFrame();
	ImGui::Render();
	ImDrawData* data = ImGui::GetDrawData();
	ImGui_ImplDX9_RenderDrawData(data);
}