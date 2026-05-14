#pragma once
#include "../core/includes.h"
#include "../core/vector.h"
#include "../core/entity.h"

inline ViewMatrix g_ViewMatrix;

inline void BuildViewMatrixFromLocal(int screenW, int screenH) {
    if (g_Offsets.viewMatrix) {
        __try {
            g_ViewMatrix = *(ViewMatrix*)g_Offsets.viewMatrix;
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
    }
}

// --- Main ESP ---
inline void DrawESP() {
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    BuildViewMatrixFromLocal(screenW, screenH);

    for (int i = 0; i <= 64; i++) {
        if (!players[i].valid) continue;
        if (settings::espTeamCheck && players[i].team == GetLocalTeam()) continue;

        Vector3 foot = players[i].pos;
        Vector3 head = foot + Vector3(0, 0, 72);

        Vector2 footScreen, headScreen;
        if (!WorldToScreen(foot, footScreen, g_ViewMatrix, screenW, screenH)) continue;
        if (!WorldToScreen(head, headScreen, g_ViewMatrix, screenW, screenH)) continue;

        float height = footScreen.y - headScreen.y;
        if (height < 1.f) continue;
        float width = height * 0.5f;

        float x = headScreen.x - width / 2;
        float y = headScreen.y;

        ImDrawList* draw = ImGui::GetBackgroundDrawList();

        auto col = [](float c[4]) { return ImColor(c[0], c[1], c[2], c[3]); };

        if (settings::box) {
            if (settings::boxFilled)
                draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height),
                    col(settings::boxFilledColor));
            draw->AddRect(ImVec2(x, y), ImVec2(x + width, y + height),
                col(settings::boxColor), 0.f, 0, settings::boxThickness);
        } else if (settings::boxCorner) {
            float l = width * 0.25f;
            ImU32 c = col(settings::boxColor);
            // corners
            draw->AddLine(ImVec2(x, y + l), ImVec2(x, y), c, settings::boxThickness);
            draw->AddLine(ImVec2(x, y), ImVec2(x + l, y), c, settings::boxThickness);
            draw->AddLine(ImVec2(x + width - l, y), ImVec2(x + width, y), c, settings::boxThickness);
            draw->AddLine(ImVec2(x + width, y), ImVec2(x + width, y + l), c, settings::boxThickness);
            draw->AddLine(ImVec2(x, y + height - l), ImVec2(x, y + height), c, settings::boxThickness);
            draw->AddLine(ImVec2(x, y + height), ImVec2(x + l, y + height), c, settings::boxThickness);
            draw->AddLine(ImVec2(x + width - l, y + height), ImVec2(x + width, y + height), c, settings::boxThickness);
            draw->AddLine(ImVec2(x + width, y + height - l), ImVec2(x + width, y + height), c, settings::boxThickness);
        }

        if (settings::health) {
            float healthPct = players[i].health / 100.f;
            float healthBarH = height * healthPct;
            draw->AddRectFilled(ImVec2(x - 5, y + height - healthBarH), ImVec2(x - 2, y + height),
                col(settings::healthColor));
            draw->AddRect(ImVec2(x - 5, y), ImVec2(x - 2, y + height),
                IM_COL32(0, 0, 0, 180), 0.f, 0, 1.f);
        }

        // Team tag
        {
            const char* tag = (players[i].team == 2) ? "[T]" : "[CT]";
            ImVec2 ts = ImGui::CalcTextSize(tag);
            draw->AddText(ImVec2(x + width / 2 - ts.x / 2, y - 15),
                players[i].team == 2 ? IM_COL32(200,50,50,255) : IM_COL32(50,130,255,255), tag);
        }

        if (settings::name && players[i].name[0]) {
            ImVec2 textSize = ImGui::CalcTextSize(players[i].name);
            draw->AddText(ImVec2(x + width / 2 - textSize.x / 2, y - 15 - 14),
                col(settings::nameColor), players[i].name);
        }

        if (settings::distance) {
            char distStr[32];
            Vector3 enemyPos = players[i].pos;
            float dist = 0.f;
            __try {
                uintptr_t localPawn = GetLocalPawn();
                if (localPawn && localPawn > 0x100000) {
                    uintptr_t scn = *(uintptr_t*)(localPawn + g_Offsets.m_pGameSceneNode);
                    if (scn && scn > 0x100000) {
                        Vector3 localPos = *(Vector3*)(scn + g_Offsets.m_vecAbsOrigin);
                        Vector3 delta = enemyPos - localPos;
                        dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z) / 52.5f;
                    }
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {}
            sprintf_s(distStr, "%.0fm", dist);
            draw->AddText(ImVec2(x + width + 4, y + height), col(settings::distColor), distStr);
        }

        players[i].headPos = GetBonePos(players[i].pawn, 0);
    }
}
