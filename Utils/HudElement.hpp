#pragma once

#include "../ImGui/imgui.h"

struct HudElement {
    ImVec2 pos;
    ImVec2 size;

    bool dragging = false;
    bool resizing = false;
    bool resizable = false;
    bool hasConfigPos = false;
    float scale = 1.0f;
    ImVec2 dragOffset;
    ImVec2 resizeCorner;
    float grabStartScale = 1.0f;
    ImVec2 grabStartSize;

    inline static HudElement* s_snapTargets[16] = {};
    inline static int s_snapCount = 0;
    inline static float s_snapThreshold = 8.0f;
    inline static bool s_snapEnabled = true;
    // Set to true this frame when any HudElement was dragged/resized.
    // Present.cpp reads and clears this to trigger auto-save.
    inline static bool s_anyDirty = false;

    static void RegisterSnapTarget(HudElement* el) {
        if (!el || s_snapCount >= 16) return;
        s_snapTargets[s_snapCount++] = el;
    }

    void HandleDrag(bool menuOpen) {
        if (!menuOpen) return;

        ImVec2 mouse = ImGui::GetIO().MousePos;
        float handleSize = 12.0f;

        bool onResize = resizable &&
            mouse.x >= pos.x + size.x - handleSize && mouse.x <= pos.x + size.x &&
            mouse.y >= pos.y + size.y - handleSize && mouse.y <= pos.y + size.y;

        bool hover = !onResize &&
            mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
            mouse.y >= pos.y && mouse.y <= pos.y + size.y;

        if (ImGui::IsMouseClicked(0)) {
            if (onResize) {
                resizing = true;
                grabStartScale = scale;
                grabStartSize = size;
                resizeCorner = mouse;
                dragging = false;
            } else if (hover) {
                dragging = true;
                dragOffset = ImVec2(mouse.x - pos.x, mouse.y - pos.y);
                resizing = false;
            }
        }

        if (!ImGui::IsMouseDown(0)) {
            dragging = false;
            resizing = false;
        }

        if (dragging) {
            pos = ImVec2(mouse.x - dragOffset.x, mouse.y - dragOffset.y);
            if (s_snapEnabled) SnapToOthers();
            s_anyDirty = true;
        }

        if (resizing) {
            float dx = mouse.x - resizeCorner.x;
            size.x = grabStartSize.x + dx;
            if (size.x < 20.0f) size.x = 20.0f;
            float ratio = size.x / grabStartSize.x;
            scale = grabStartScale * ratio;
            if (scale < 0.3f) scale = 0.3f;
            if (scale > 5.0f) scale = 5.0f;
            s_anyDirty = true;
        }
    }

    void SnapToOthers() {
        float r1 = pos.x + size.x;
        float b1 = pos.y + size.y;
        float bestX = s_snapThreshold;
        float bestY = s_snapThreshold;
        float snapX = pos.x;
        float snapY = pos.y;

        for (int i = 0; i < s_snapCount; i++) {
            HudElement* o = s_snapTargets[i];
            if (o == this) continue;

            float l2 = o->pos.x, r2 = o->pos.x + o->size.x;
            float t2 = o->pos.y, b2 = o->pos.y + o->size.y;

            if (!(b1 <= t2 || pos.y >= b2)) {
                float d;
                d = (pos.x > l2) ? (pos.x - l2) : (l2 - pos.x); if (d < bestX) { bestX = d; snapX = l2; }
                d = (r1 > r2) ? (r1 - r2) : (r2 - r1); if (d < bestX) { bestX = d; snapX = r2 - size.x; }
                d = (r1 > l2) ? (r1 - l2) : (l2 - r1); if (d < bestX) { bestX = d; snapX = l2 - size.x; }
                d = (pos.x > r2) ? (pos.x - r2) : (r2 - pos.x); if (d < bestX) { bestX = d; snapX = r2; }
            }

            if (!(r1 <= l2 || pos.x >= r2)) {
                float d;
                d = (pos.y > t2) ? (pos.y - t2) : (t2 - pos.y); if (d < bestY) { bestY = d; snapY = t2; }
                d = (b1 > b2) ? (b1 - b2) : (b2 - b1); if (d < bestY) { bestY = d; snapY = b2 - size.y; }
                d = (b1 > t2) ? (b1 - t2) : (t2 - b1); if (d < bestY) { bestY = d; snapY = t2 - size.y; }
                d = (pos.y > b2) ? (pos.y - b2) : (b2 - pos.y); if (d < bestY) { bestY = d; snapY = b2; }
            }
        }

        if (bestX < s_snapThreshold) pos.x = snapX;
        if (bestY < s_snapThreshold) pos.y = snapY;
    }

    void ClampToScreen() {
        ImVec2 screen = ImGui::GetIO().DisplaySize;

        if (pos.x < 0) pos.x = 0;
        if (pos.y < 0) pos.y = 0;

        if (pos.x + size.x > screen.x)
            pos.x = screen.x - size.x;

        if (pos.y + size.y > screen.y)
            pos.y = screen.y - size.y;

        if (size.x < 20) size.x = 20;
        if (size.y < 20) size.y = 20;
    }

    void RenderHudEditor(ImDrawList* draw) {
        ImVec2 min = pos;
        ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);

        draw->AddRectFilled(min, max, IM_COL32(128, 128, 128, 30));
        draw->AddRect(min, max, IM_COL32(128, 128, 128, 100), 0.0f, 0, 1.5f);

        if (resizable) {
            float hs = 10.0f;
            ImVec2 rMin = ImVec2(max.x - hs, max.y - hs);
            ImVec2 rMax = max;
            draw->AddRectFilled(rMin, rMax, IM_COL32(128, 128, 128, 150));
            draw->AddTriangleFilled(
                ImVec2(max.x - 2, max.y - hs),
                ImVec2(max.x - 2, max.y - 2),
                ImVec2(max.x - hs, max.y - 2),
                IM_COL32(200, 200, 200, 200)
            );
        }
    }
};
