#pragma once
#include "../core/includes.h"
#include "../core/config.h"
#include "aimbot.h"

struct CheatStatus {
    bool initialized;
    bool presentHooked;
    bool createMoveHooked;
    uintptr_t createMoveAddr;
    uintptr_t clientBase;
    uintptr_t entityList;
    uintptr_t viewAngles;
    uintptr_t viewMatrix;
    char clientDll[32];
    char cmRva[32];
};
extern CheatStatus g_Status;

inline void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    
    // Palette
    ImVec4 bg(0.06f, 0.06f, 0.08f, 1.f);
    ImVec4 panel(0.10f, 0.10f, 0.13f, 1.f);
    ImVec4 frame(0.15f, 0.15f, 0.18f, 1.f);
    ImVec4 accent(1.f, 0.55f, 0.f, 1.f);
    ImVec4 accentHover(1.f, 0.65f, 0.15f, 1.f);
    ImVec4 text(0.90f, 0.90f, 0.92f, 1.f);
    ImVec4 textDim(0.45f, 0.45f, 0.50f, 1.f);

    // Rounding
    s.WindowRounding     = 10.f;
    s.ChildRounding      = 8.f;
    s.FrameRounding      = 6.f;
    s.PopupRounding      = 8.f;
    s.ScrollbarRounding  = 6.f;
    s.GrabRounding       = 6.f;
    s.TabRounding        = 6.f;

    // Spacing
    s.WindowPadding    = ImVec2(16, 16);
    s.FramePadding     = ImVec2(8, 5);
    s.ItemSpacing      = ImVec2(12, 8);
    s.ItemInnerSpacing = ImVec2(8, 5);
    s.ScrollbarSize    = 6.f;
    s.GrabMinSize      = 12.f;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    auto& cl = s.Colors;
    cl[ImGuiCol_WindowBg]          = bg;
    cl[ImGuiCol_ChildBg]           = panel;
    cl[ImGuiCol_PopupBg]           = panel;
    cl[ImGuiCol_FrameBg]           = frame;
    cl[ImGuiCol_FrameBgHovered]    = ImVec4(0.20f, 0.20f, 0.25f, 1.f);
    cl[ImGuiCol_FrameBgActive]     = accent;
    cl[ImGuiCol_TitleBg]           = panel;
    cl[ImGuiCol_TitleBgActive]     = ImVec4(0.12f, 0.12f, 0.15f, 1.f);
    cl[ImGuiCol_TitleBgCollapsed]  = panel;
    cl[ImGuiCol_MenuBarBg]         = panel;
    cl[ImGuiCol_ScrollbarBg]       = panel;
    cl[ImGuiCol_ScrollbarGrab]     = ImVec4(0.30f, 0.30f, 0.35f, 1.f);
    cl[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.45f, 1.f);
    cl[ImGuiCol_ScrollbarGrabActive]  = accent;
    cl[ImGuiCol_CheckMark]         = accent;
    cl[ImGuiCol_SliderGrab]        = accent;
    cl[ImGuiCol_SliderGrabActive]  = accentHover;
    cl[ImGuiCol_Button]            = frame;
    cl[ImGuiCol_ButtonHovered]     = accentHover;
    cl[ImGuiCol_ButtonActive]      = accent;
    cl[ImGuiCol_Header]            = accent;
    cl[ImGuiCol_HeaderHovered]     = accentHover;
    cl[ImGuiCol_HeaderActive]      = accent;
    cl[ImGuiCol_Separator]         = ImVec4(0.20f, 0.20f, 0.25f, 1.f);
    cl[ImGuiCol_Tab]               = frame;
    cl[ImGuiCol_TabHovered]        = accentHover;
    cl[ImGuiCol_TabActive]         = accent;
    cl[ImGuiCol_TabUnfocused]      = frame;
    cl[ImGuiCol_TabUnfocusedActive]= panel;
    cl[ImGuiCol_Text]              = text;
    cl[ImGuiCol_TextDisabled]      = textDim;
    cl[ImGuiCol_Border]            = ImVec4(0.15f, 0.15f, 0.18f, 1.f);
    cl[ImGuiCol_BorderShadow]      = ImVec4(0, 0, 0, 0);
}

// Custom Switch Widget
inline bool RenderSwitch(const char* label, bool* v) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    
    ImVec2 widgetSize = ImVec2(40, 18);
    float radius = widgetSize.y * 0.5f;
    
    // Interaction via InvisibleButton
    bool clicked = ImGui::InvisibleButton(label, widgetSize);
    if (clicked) *v = !(*v);
    
    // Drawing
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pMin = ImGui::GetItemRectMin();
    ImVec2 pMax = ImGui::GetItemRectMax();
    
    ImU32 colBg = *v ? ImGui::GetColorU32(ImVec4(1.f, 0.55f, 0.f, 1.f)) : ImGui::GetColorU32(ImVec4(0.20f, 0.20f, 0.25f, 1.f));
    ImU32 colCircle = *v ? IM_COL32(255, 255, 255, 255) : ImGui::GetColorU32(ImVec4(0.60f, 0.60f, 0.65f, 1.f));
    
    // Background pill
    draw->AddRectFilled(pMin, pMax, colBg, radius);
    
    // Circle knob
    float knobX = *v ? pMax.x - radius - 2 : pMin.x + radius + 2;
    draw->AddCircleFilled(ImVec2(knobX, pMin.y + radius), radius - 2, colCircle);
    
    // Label
    if (label) {
        ImGui::SameLine();
        ImGui::Text(" %s", label);
    }
    
    return clicked;
}

inline void SectionHeader(const char* label) {
    ImGui::TextColored(ImVec4(1.f, 0.55f, 0.f, 1.f), "%s", label);
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 2));
}

inline void RenderKeyBinder(const char* label, int& key, bool& keyWait) {
    if (keyWait) {
        ImGui::Text("Press a key...");
        for (int k = 1; k < 256; k++) {
            if ((GetAsyncKeyState(k) & 1) && k != VK_INSERT && k != VK_END) {
                key = k;
                keyWait = false;
                break;
            }
        }
    } else {
        if (ImGui::Button(label, ImVec2(90, 0))) keyWait = true;
        ImGui::SameLine();
        ImGui::Text(" %s", AimbotKeyName(key));
    }
}

inline void RenderMenu(bool& menuOpen) {
    ImGui::SetNextWindowSize(ImVec2(760, 620), ImGuiCond_Always);
    ImGui::Begin("CockEngine", &menuOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    if (ImGui::BeginTabBar("Tabs")) {
        float winW = ImGui::GetContentRegionAvail().x;
        float colW = (winW - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        float childH = 505;

        // === Aimbot ===
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::BeginChild("##aimbot_left", ImVec2(colW, childH), true);
            SectionHeader("Aimbot");

            RenderSwitch("Enable", &settings::aimbotEnabled);
            RenderSwitch("Team check", &settings::aimbotTeamCheck);

            ImGui::Text("Hitbox");
            ImGui::SetNextItemWidth(140);
            ImGui::Combo("##hb", &settings::aimbotHitbox, "Head\0Neck\0Chest\0");

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextDisabled("Key binding");
            ImGui::Separator();
            RenderKeyBinder("Bind key", settings::aimbotKey, settings::aimbotKeyWait);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextDisabled("FOV circle");
            ImGui::Separator();
            RenderSwitch("Show circle", &settings::aimbotShowFov);
            if (settings::aimbotShowFov) {
                ImGui::Text("Radius");
                ImGui::SetNextItemWidth(140);
                ImGui::SliderInt("##fov", &settings::aimbotFov, 1, 30, "%d");
            }

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextDisabled("Smoothing");
            ImGui::Separator();
            RenderSwitch("Smooth", &settings::aimbotSmoothEnabled);
            if (settings::aimbotSmoothEnabled) {
                ImGui::Text("Value");
                ImGui::SetNextItemWidth(140);
                ImGui::SliderInt("##smooth", &settings::aimbotSmooth, 1, 30, "%d");
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##triggerbot", ImVec2(colW, childH), true);
            SectionHeader("Triggerbot");

            RenderSwitch("Enable", &settings::triggerbotEnabled);
            RenderSwitch("Team check", &settings::triggerbotTeamCheck);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextDisabled("Key binding");
            ImGui::Separator();
            RenderKeyBinder("Bind key", settings::triggerbotKey, settings::triggerbotKeyWait);
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        // === ESP ===
        if (ImGui::BeginTabItem("ESP")) {
            ImGui::BeginChild("##esp_left", ImVec2(colW, childH), true);
            SectionHeader("General");

            RenderSwitch("Enable ESP", &settings::espEnabled);
            RenderSwitch("Team check", &settings::espTeamCheck);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextDisabled("Box");
            ImGui::Separator();
            RenderSwitch("Box", &settings::box);
            RenderSwitch("Corner", &settings::boxCorner);
            RenderSwitch("Filled", &settings::boxFilled);
            if (settings::box || settings::boxCorner) {
                ImGui::Text("Thickness");
                ImGui::SetNextItemWidth(140);
                ImGui::SliderInt("##bthick", &settings::boxThickness, 1, 5, "%d");
            }
            if (settings::box || settings::boxCorner) {
                ImGui::Text("Color");
                ImGui::ColorEdit4("##bcolor", settings::boxColor, ImGuiColorEditFlags_NoInputs);
            }
            if (settings::boxFilled) {
                ImGui::Text("Fill color");
                ImGui::ColorEdit4("##bfill", settings::boxFilledColor, ImGuiColorEditFlags_NoInputs);
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##esp_right", ImVec2(colW, childH), true);
            SectionHeader("Info");

            RenderSwitch("Health bar", &settings::health);
            if (settings::health) {
                ImGui::Text("Color");
                ImGui::ColorEdit4("##hcolor", settings::healthColor, ImGuiColorEditFlags_NoInputs);
            }

            ImGui::Dummy(ImVec2(0, 4));
            RenderSwitch("Name", &settings::name);
            if (settings::name) {
                ImGui::Text("Color");
                ImGui::ColorEdit4("##ncolor", settings::nameColor, ImGuiColorEditFlags_NoInputs);
            }

            ImGui::Dummy(ImVec2(0, 4));
            RenderSwitch("Distance", &settings::distance);
            if (settings::distance) {
                ImGui::Text("Color");
                ImGui::ColorEdit4("##dcolor", settings::distColor, ImGuiColorEditFlags_NoInputs);
            }
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        // === Visual ===
        if (ImGui::BeginTabItem("Visual")) {
            ImGui::BeginChild("##visual_left", ImVec2(colW, childH), true);
            SectionHeader("Effects");

            RenderSwitch("No Flash", &settings::noFlash);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextDisabled("FOV");
            ImGui::Separator();
            RenderSwitch("FOV Changer", &settings::fovChanger);
            if (settings::fovChanger) {
                ImGui::Text("Value");
                ImGui::SetNextItemWidth(140);
                ImGui::SliderInt("##fovval", &settings::fovValue, 60, 120, "%d");
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##visual_right", ImVec2(colW, childH), true);
            SectionHeader("World");

            RenderSwitch("No Fog", &settings::noFog);
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        // === Misc ===
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::BeginChild("##misc_left", ImVec2(colW, childH), true);
            SectionHeader("Bhop");

            RenderSwitch("Enabled", &settings::bhop);
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##misc_right", ImVec2(colW, childH), true);
            SectionHeader("Overlay");

            RenderSwitch("Watermark", &settings::watermark);
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        // === Config ===
        if (ImGui::BeginTabItem("Config")) {
            ImGui::BeginChild("##config_left", ImVec2(colW, childH), true);
            SectionHeader("Save / Load");

            if (ImGui::Button("Save Config", ImVec2(160, 28)))
                SaveConfig();
            ImGui::Dummy(ImVec2(0, 4));
            if (ImGui::Button("Load Config", ImVec2(160, 28)))
                LoadConfig();
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##config_right", ImVec2(colW, childH), true);
            SectionHeader("Controls");

            ImGui::Text("INSERT  =  Toggle menu");
            ImGui::Text("END     =  Unload cheat");
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        // === About ===
        if (ImGui::BeginTabItem("About")) {
            float aboutH = 160;

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::SetCursorPosX((winW - ImGui::CalcTextSize("CockEngine").x) * 0.5f);
            ImGui::TextColored(ImVec4(1.f, 0.55f, 0.f, 1.f), "CockEngine");
            ImGui::SetCursorPosX((winW - ImGui::CalcTextSize("v1.0").x) * 0.5f);
            ImGui::TextDisabled("v1.0");
            ImGui::Dummy(ImVec2(0, 2));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 2));

            ImGui::BeginChild("##hooks", ImVec2(colW, aboutH), true);
            SectionHeader("Hooks");

            if (g_Status.presentHooked) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.85f, 0.f, 1.f));
                ImGui::BulletText("Present");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
                ImGui::BulletText("Present");
                ImGui::PopStyleColor();
            }

            if (g_Status.createMoveHooked) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.85f, 0.f, 1.f));
                ImGui::BulletText("CreateMove (RVA: %s)", g_Status.cmRva);
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
                ImGui::BulletText("CreateMove");
                ImGui::PopStyleColor();
            }
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("##memory", ImVec2(colW, aboutH), true);
            SectionHeader("Memory");

            ImGui::Text("client.dll   %s", g_Status.clientDll);
            ImGui::Text("EntityList   0x%llX", g_Status.entityList);
            ImGui::Text("ViewAngles   0x%llX", g_Status.viewAngles);
            ImGui::Text("ViewMatrix   0x%llX", g_Status.viewMatrix);
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::Separator();
            ImGui::TextDisabled("Built: %s", __DATE__);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
