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
	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	style->AntiAliasedLines = true;
	style->AntiAliasedFill = true;

	// Main
	style->WindowPadding.x = 5.0f;
	style->WindowPadding.y = 5.0f;
	style->PopupRounding = 2.0f;
	style->FramePadding.x = 10.0f;
	style->FramePadding.y = 5.0f;
	style->ItemSpacing.x = 10.0f;
	style->ItemSpacing.y = 5.0f;
	style->ItemInnerSpacing.x = 10.0f;
	style->ItemInnerSpacing.y = 4.0f;
	style->TouchExtraPadding.x = 0.0f;
	style->TouchExtraPadding.y = 0.0f;
	style->IndentSpacing = 30.0f;
	style->ScrollbarSize = 14.0f;
	style->GrabMinSize = 10.0f;

	// Borders
	style->WindowBorderSize = 1.0f;
	style->ChildBorderSize = 1.0f;
	style->PopupBorderSize = 1.0f;
	//style->FrameBorderSize = 1.0f;
	style->TabBorderSize = 0.0f;

	// Rounding
	style->WindowRounding = 2.0f;
	style->ChildRounding = 2.0f;
	style->FrameRounding = 2.0f;
	style->ScrollbarRounding = 2.0f;
	style->GrabRounding = 2.0f;
	style->TabRounding = 2.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f * transparencyScale);
	//style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f * transparencyScale);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f * transparencyScale);
	style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f * transparencyScale);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f * transparencyScale);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f * transparencyScale);
	//style->Colors[ImGuiCol_ComboBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f * transparencyScale);
	style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f * transparencyScale);
	style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_Tab] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_TabActive] = ImVec4(0.03f, 0.02f, 0.05f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_TabHovered] = ImVec4(0.27f, 0.3f, 0.3f, 0.80f * transparencyScale);

	//style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	//style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	//style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f * transparencyScale);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f * transparencyScale);
	//style->Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
	//style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
	//style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
	style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f * transparencyScale);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f * transparencyScale);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f * transparencyScale);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f * transparencyScale);
	//style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
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
		font->Scale = 1.5f;
		ImGui::SetCurrentFont(font);
		ResetImFont = false;

		once = false;
	}
	if (r::should_draw_menu) {
		
        R_MenuStyle();
        ImGui::Begin("[ALPHA] 1_kej_v2 - github.com/kejjjjj/", &r::should_draw_menu, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowFontScale(1.2f);
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetWindowSize(ImVec2(viewport->Size.x / 3.f, viewport->Size.y / 3.5f), ImGuiCond_FirstUseEver);
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