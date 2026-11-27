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
        BeginPanel("##strafe_card", "Strafing", 294);
        Toggle("Strafebot", v::mod_strafebot);
        Toggle("Auto Para (W)", v::mod_auto_para);
        Toggle("Auto 250 switch", v::mod_auto_250);
        Toggle("All techniques", v::mod_strafebot_all);
        Toggle("Engine acceleration", v::mod_strafebot_accel);
        ImGui::SetNextItemWidth(105); ImGui::DragFloat("Smoothing", &v::mod_strafebot_smooth.evar->floatValue, .5f,1,45,"%.2f");
        ImGui::SetNextItemWidth(105); ImGui::DragFloat("Bhop smoothing", &v::mod_strafebot_bhop_s.evar->floatValue, .5f,1,100,"%.2f");
        Bind("Bind overwrite", v::mod_strafebot_ow, editOverwrite);
        EndPanel();
        BeginPanel("##slide_card", "Edge Slide", 166);
        const char* modes[]={"Off","Engine Slide","FPS Slide"};
        int mode=glm::clamp(v::mod_autoslide.GetInt(),0,2);
        ImGui::SetNextItemWidth(150); if (ImGui::Combo("Mode", &mode,modes,3)) v::mod_autoslide.SetValue(static_cast<float>(mode));
        Bind("Bind Edge Slide", v::mod_edge_key, editSlide);
        ImGui::TextDisabled("%s", automation.currentlySliding ? "Sliding" : automation.pendingSlide ? "Armed" : "Idle");
        ImGui::TextWrapped("Press once to arm; press again to cancel.");
        EndPanel();
        ImGui::TableNextColumn();
        BeginPanel("##fps_card", "AutoFPS", 210);
        Toggle("AutoFPS", v::mod_autoFPS);
        Toggle("Long 125", v::mod_autoFPS_long125);
        const int limits[]={0,166,142,111}; const char* names[]={"Automatic zones","166","142","111"};
        int selected=0; for(int i=1;i<4;++i) if(v::mod_fps_override.GetInt()==limits[i]) selected=i;
        ImGui::SetNextItemWidth(148); if(ImGui::Combo("Air FPS",&selected,names,4)) v::mod_fps_override.SetValue(static_cast<float>(limits[selected]));
        const char* hug[]={"Disabled","333","500","1000"};
        ImGui::SetNextItemWidth(148); if(ImGui::Combo("Bounce FPS",&v::mod_autoFPS_hug500.evar->intValue,hug,4)) v::mod_autoFPS_hug500.SetValue(static_cast<float>(v::mod_autoFPS_hug500.GetInt()));
        EndPanel();
        BeginPanel("##500_card", "500 FPS area", 174);
        Toggle("Enable spawning",v::mod_auto500_enabled);
        if (!v::mod_auto500_enabled.isEnabled()) { box500.boxExists=false; box500.use500=false; }
        ImGui::BeginDisabled(!box500.boxExists);
        ImGui::SetNextItemWidth(125); ImGui::DragFloat("X bounds", &box500.bounds[0],4,0,10000,"%.2f");
        ImGui::SetNextItemWidth(125); ImGui::DragFloat("Y bounds", &box500.bounds[1],4,0,10000,"%.2f");
        ImGui::SetNextItemWidth(125); ImGui::DragFloat("Z bounds", &box500.bounds[2],4,0,10000,"%.2f");
        if(ImGui::Button("Delete area")) { memset(&box500,0,sizeof(area500_s)); box500.bounds[0]=box500.bounds[1]=box500.bounds[2]=32; }
        ImGui::EndDisabled(); EndPanel();
        BeginPanel("##rpg_card", "RPG", 76); Toggle("RPG lookdown",v::mod_autoanglerpg); EndPanel();
        ImGui::EndTable();
    }
}
