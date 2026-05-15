#pragma once
#include "../core/includes.h"
#include "../core/config.h"
#include "aimbot.h"

inline void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4 orange(1.f, 0.55f, 0.f, 1.f);
    ImVec4 darkBg(0.07f, 0.07f, 0.09f, 1.f);
    ImVec4 darkPanel(0.11f, 0.11f, 0.14f, 1.f);
    ImVec4 darkFrame(0.16f, 0.16f, 0.20f, 1.f);

    s.WindowRounding     = 6.f;
    s.ChildRounding      = 4.f;
    s.FrameRounding      = 4.f;
    s.PopupRounding      = 4.f;
    s.ScrollbarRounding  = 4.f;
    s.GrabRounding       = 4.f;
    s.TabRounding        = 4.f;

    s.WindowPadding    = ImVec2(14, 14);
    s.FramePadding     = ImVec2(10, 6);
    s.ItemSpacing      = ImVec2(10, 7);
    s.ItemInnerSpacing = ImVec2(8, 5);
    s.ScrollbarSize    = 10.f;
    s.GrabMinSize      = 10.f;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    auto& cl = s.Colors;
    cl[ImGuiCol_WindowBg]          = darkBg;
    cl[ImGuiCol_ChildBg]           = darkPanel;
    cl[ImGuiCol_PopupBg]           = darkPanel;
    cl[ImGuiCol_FrameBg]           = darkFrame;
    cl[ImGuiCol_FrameBgHovered]    = ImVec4(0.22f, 0.22f, 0.28f, 1.f);
    cl[ImGuiCol_FrameBgActive]     = orange;
    cl[ImGuiCol_TitleBg]           = darkPanel;
    cl[ImGuiCol_TitleBgActive]     = ImVec4(0.14f, 0.14f, 0.18f, 1.f);
    cl[ImGuiCol_TitleBgCollapsed]  = darkPanel;
    cl[ImGuiCol_MenuBarBg]         = darkPanel;
    cl[ImGuiCol_ScrollbarBg]       = darkPanel;
    cl[ImGuiCol_ScrollbarGrab]     = orange;
    cl[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.f, 0.65f, 0.1f, 1.f);
    cl[ImGuiCol_ScrollbarGrabActive]  = ImVec4(1.f, 0.7f, 0.2f, 1.f);
    cl[ImGuiCol_CheckMark]         = orange;
    cl[ImGuiCol_SliderGrab]        = orange;
    cl[ImGuiCol_SliderGrabActive]  = ImVec4(1.f, 0.65f, 0.1f, 1.f);
    cl[ImGuiCol_Button]            = darkFrame;
    cl[ImGuiCol_ButtonHovered]     = orange;
    cl[ImGuiCol_ButtonActive]      = ImVec4(1.f, 0.65f, 0.1f, 1.f);
    cl[ImGuiCol_Header]            = orange;
    cl[ImGuiCol_HeaderHovered]     = ImVec4(1.f, 0.65f, 0.1f, 1.f);
    cl[ImGuiCol_HeaderActive]      = ImVec4(1.f, 0.7f, 0.2f, 1.f);
    cl[ImGuiCol_Separator]         = orange;
    cl[ImGuiCol_Tab]               = darkPanel;
    cl[ImGuiCol_TabHovered]        = orange;
    cl[ImGuiCol_TabActive]         = orange;
    cl[ImGuiCol_TabUnfocused]      = darkPanel;
    cl[ImGuiCol_TabUnfocusedActive]= orange;
    cl[ImGuiCol_Text]              = ImVec4(0.92f, 0.92f, 0.94f, 1.f);
    cl[ImGuiCol_TextDisabled]      = ImVec4(0.45f, 0.45f, 0.50f, 1.f);
    cl[ImGuiCol_Border]            = ImVec4(0.18f, 0.18f, 0.22f, 1.f);
    cl[ImGuiCol_BorderShadow]      = ImVec4(0, 0, 0, 0);
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
        if (ImGui::Button(label)) keyWait = true;
        ImGui::SameLine();
        ImGui::Text("Key: %s", AimbotKeyName(key));
    }
}

inline void RenderMenu(bool& menuOpen) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(550, 420), ImVec2(800, 600));
    ImGui::Begin("CockEngine", &menuOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::BeginTabBar("Tabs")) {

        // === Aimbot ===
        if (ImGui::BeginTabItem("Aimbot")) {
            if (ImGui::BeginChild("##aimbot_left", ImVec2(ImGui::GetWindowWidth() / 2 - 20, 0), true)) {
                ImGui::TextColored(ImVec4(1, 0.55f, 0, 1), "Aimbot");
                ImGui::Separator();

                ImGui::Checkbox("Enable", &settings::aimbotEnabled);
                ImGui::Checkbox("Team check", &settings::aimbotTeamCheck);
                ImGui::Combo("Hitbox", &settings::aimbotHitbox, "Head\0Neck\0Chest\0");

                ImGui::Separator();
                ImGui::TextDisabled("-- Key --");
                RenderKeyBinder("Bind key", settings::aimbotKey, settings::aimbotKeyWait);

                ImGui::Separator();
                ImGui::TextDisabled("-- FOV --");
                ImGui::Checkbox("Show circle", &settings::aimbotShowFov);
                if (settings::aimbotShowFov)
                    ImGui::SliderFloat("Radius", &settings::aimbotFov, 0.5f, 30.f, "%.1f");

                ImGui::Separator();
                ImGui::TextDisabled("-- Smoothing --");
                ImGui::Checkbox("Smooth", &settings::aimbotSmoothEnabled);
                if (settings::aimbotSmoothEnabled)
                    ImGui::SliderFloat("Value", &settings::aimbotSmooth, 0.f, 30.f, "%.1f");
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("##triggerbot_right", ImVec2(ImGui::GetWindowWidth() / 2 - 20, 0), true)) {
                ImGui::TextColored(ImVec4(1, 0.55f, 0, 1), "Triggerbot");
                ImGui::Separator();

                ImGui::Checkbox("Enable", &settings::triggerbotEnabled);
                ImGui::Checkbox("Team check", &settings::triggerbotTeamCheck);

                ImGui::Separator();
                ImGui::TextDisabled("-- Key --");
                RenderKeyBinder("Bind key", settings::triggerbotKey, settings::triggerbotKeyWait);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // === ESP ===
        if (ImGui::BeginTabItem("ESP")) {
            ImGui::Checkbox("Enable ESP", &settings::espEnabled);
            ImGui::SameLine();
            ImGui::Checkbox("Team check", &settings::espTeamCheck);
            ImGui::Separator();

            ImGui::TextDisabled("-- Box --");
            ImGui::Checkbox("Box", &settings::box);
            ImGui::SameLine();
            ImGui::Checkbox("Corner", &settings::boxCorner);
            ImGui::SameLine();
            ImGui::Checkbox("Filled", &settings::boxFilled);
            if (settings::box || settings::boxCorner) {
                ImGui::SliderFloat("Thickness", &settings::boxThickness, 0.5f, 5.f, "%.1f");
                ImGui::ColorEdit4("Box Color", settings::boxColor, ImGuiColorEditFlags_NoInputs);
            }
            if (settings::boxFilled)
                ImGui::ColorEdit4("Fill Color", settings::boxFilledColor, ImGuiColorEditFlags_NoInputs);
            ImGui::Separator();

            ImGui::TextDisabled("-- Health --");
            ImGui::Checkbox("Health bar", &settings::health);
            if (settings::health)
                ImGui::ColorEdit4("Health Color", settings::healthColor, ImGuiColorEditFlags_NoInputs);
            ImGui::Separator();

            ImGui::TextDisabled("-- Name --");
            ImGui::Checkbox("Name", &settings::name);
            if (settings::name)
                ImGui::ColorEdit4("Name Color", settings::nameColor, ImGuiColorEditFlags_NoInputs);
            ImGui::Separator();

            ImGui::TextDisabled("-- Distance --");
            ImGui::Checkbox("Distance", &settings::distance);
            if (settings::distance)
                ImGui::ColorEdit4("Dist. Color", settings::distColor, ImGuiColorEditFlags_NoInputs);

            ImGui::EndTabItem();
        }

        // === Visual ===
        if (ImGui::BeginTabItem("Visual")) {
            ImGui::Checkbox("No Flash", &settings::noFlash);
            ImGui::Separator();

            ImGui::Checkbox("FOV Changer", &settings::fovChanger);
            if (settings::fovChanger)
                ImGui::SliderFloat("Value", &settings::fovValue, 60.f, 120.f, "%.0f");
            ImGui::Separator();

            ImGui::Checkbox("No Fog", &settings::noFog);

            ImGui::EndTabItem();
        }

        // === Misc ===
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::TextDisabled("Hold space to auto-jump");
            ImGui::Separator();
            ImGui::Checkbox("Watermark", &settings::watermark);
            ImGui::EndTabItem();
        }

        // === Config ===
        if (ImGui::BeginTabItem("Config")) {
            ImGui::TextColored(ImVec4(1, 0.55f, 0, 1), "Configuration");
            ImGui::Separator();

            if (ImGui::Button("Save Config", ImVec2(180, 0)))
                SaveConfig();
            ImGui::SameLine();
            if (ImGui::Button("Load Config", ImVec2(180, 0)))
                LoadConfig();

            ImGui::Separator();
            ImGui::TextDisabled("-- Keys --");
            ImGui::Text("INSERT  =  Toggle menu");
            ImGui::Text("END     =  Unload cheat");

            ImGui::EndTabItem();
        }

        // === About ===
        if (ImGui::BeginTabItem("About")) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::TextColored(ImVec4(1, 0.55f, 0, 1), "CockEngine v1.0");
            ImGui::Separator();
            ImGui::TextDisabled("Internal cheat for Counter-Strike 2");
            ImGui::Separator();
            ImGui::BulletText("Interfaces via CreateInterface");
            ImGui::BulletText("SchemaSystem runtime resolution");
            ImGui::BulletText("Pattern scanning + RVA fallback");
            ImGui::BulletText("D3D11 Present hook (Kiero)");
            ImGui::Separator();
            ImGui::Text("Build: %s", __DATE__);
            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
