#include "pch.h"

namespace {
    void Toggle(const char* label, evar_o& setting) {
        if (ImGui::Checkbox(label, &setting.evar->enabled)) setting.SetValue(setting.isEnabled());
    }
    void Bind(const char* label, evar_o& setting, bool& editing) {
        if (ImGui::Button(editing ? "Press key..." : label)) editing = true;
        if (editing && !ImGui::IsMouseDown(0)) {
            const BYTE key = KeyPressed();
            if (key && key != VK_LBUTTON && key != VK_RBUTTON) { setting.SetValue(static_cast<float>(key)); editing = false; }
        }
        ImGui::SameLine(); ImGui::TextDisabled("VK %d", setting.GetInt());
        ImGui::SameLine(); if (ImGui::SmallButton("Clear")) { setting.SetValue(0.0f); editing = false; }
    }
    void BeginPanel(const char* id, const char* title, float h) {
        ImGui::BeginChild(id, ImVec2(0,h), true);
        ImGui::TextDisabled("%s", title); ImGui::Spacing();
    }
    void EndPanel() { ImGui::EndChild(); }
}
void r::R_Automation_Features()
{
    static bool editOverwrite=false, editSlide=false;
    if (ImGui::BeginTable("##automation_columns", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        BeginPanel("##strafe_card", "Strafing", 390);
        Toggle("Strafebot", v::mod_strafebot);
        Toggle("Auto Para", v::mod_auto_para);
        Toggle("Auto 250 Switch", v::mod_auto_250);
        Toggle("Halfbeat (A/D)", v::mod_halfbeat);
        ImGui::TextDisabled("%s | Auto turns: %u", automation.paraActive ? (automation.paraSide > 0 ? "Auto right" : "Auto left") : "Manual / idle", automation.auto250Switches);
        ImGui::TextDisabled("Default: WA / WD only");
        ImGui::TextDisabled("Auto command: %s", bhopHookInstalled ? "ready" : "unavailable");
        Bind("Bind overwrite", v::mod_strafebot_ow, editOverwrite);
        EndPanel();
        BeginPanel("##slide_card", "Edge Slide", 190);
        const char* modes[]={"Off","Engine Slide","FPS Slide"};
        int mode=glm::clamp(v::mod_autoslide.GetInt(),0,2);
        ImGui::SetNextItemWidth(150); if (ImGui::Combo("Mode", &mode,modes,3)) v::mod_autoslide.SetValue(static_cast<float>(mode));
        Bind("Bind Edge Slide", v::mod_edge_key, editSlide);
        ImGui::TextDisabled("%s", automation.currentlySliding ? "Sliding" : automation.pendingSlide ? "Armed" : "Idle");
        ImGui::TextWrapped("Arm before landing. Press again to cancel.");
        EndPanel();
        ImGui::TableNextColumn();
        BeginPanel("##fps_card", "AutoFPS", 245);
        Toggle("AutoFPS", v::mod_autoFPS);
        const bool restricted = v::mod_auto_para.isEnabled() || v::mod_auto_250.isEnabled();
        ImGui::BeginDisabled(restricted);
        Toggle("Long 125", v::mod_autoFPS_long125);
        const int limits[]={0,166,142,111}; const char* names[]={"Automatic zones","166","142","111"};
        int selected=0; for(int i=1;i<4;++i) if(v::mod_fps_override.GetInt()==limits[i]) selected=i;
        ImGui::SetNextItemWidth(148); if(ImGui::Combo("Air FPS",&selected,names,4)) v::mod_fps_override.SetValue(static_cast<float>(limits[selected]));
        const char* hug[]={"Disabled","333","500","1000"};
        ImGui::SetNextItemWidth(148); if(ImGui::Combo("Bounce FPS",&v::mod_autoFPS_hug500.evar->intValue,hug,4)) v::mod_autoFPS_hug500.SetValue(static_cast<float>(v::mod_autoFPS_hug500.GetInt()));
        ImGui::EndDisabled();
        if (restricted) ImGui::TextWrapped("Auto legs: 250 / 333 only. Auto Para returns control after two legs.");
        EndPanel();
        BeginPanel("##500_card", "500 FPS area", 200);
        Toggle("Enable spawning",v::mod_auto500_enabled);
        if (!v::mod_auto500_enabled.isEnabled()) { box500.boxExists=false; box500.use500=false; }
        ImGui::BeginDisabled(!box500.boxExists);
        ImGui::SetNextItemWidth(125); ImGui::DragFloat("X bounds", &box500.bounds[0],4,0,10000,"%.2f");
        ImGui::SetNextItemWidth(125); ImGui::DragFloat("Y bounds", &box500.bounds[1],4,0,10000,"%.2f");
        ImGui::SetNextItemWidth(125); ImGui::DragFloat("Z bounds", &box500.bounds[2],4,0,10000,"%.2f");
        if(ImGui::Button("Delete area")) { memset(&box500,0,sizeof(area500_s)); box500.bounds[0]=box500.bounds[1]=box500.bounds[2]=32; }
        ImGui::EndDisabled(); EndPanel();
        BeginPanel("##rpg_card", "RPG", 88); Toggle("RPG lookdown",v::mod_autoanglerpg); EndPanel();
        ImGui::EndTable();
    }
}
