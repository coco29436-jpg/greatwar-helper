// The two drawing boards of the Production screen. The equipment designer: the blueprint of the machine in the
// middle, its module slots around it with leader lines to where each part sits, and its numbers on the side. The
// division designer: the battalions as a grid of cells on the staff table, the numbers on the side.
#include <algorithm>
#include <cmath>
#include <map>

#include "../ui/icons.h"
#include "blueprint.h"
#include <fstream>
#include <sstream>

#include "app.h"
#include "raymath.h"

namespace {
const Color BP_GROUND{14, 36, 64, 255}, BP_GRID{70, 120, 170, 34}, BP_GRID2{90, 145, 200, 62}, BP_INK{176, 222, 250, 240}, BP_FAINT{120, 170, 210, 150};
const Color BP_FILLED{24, 56, 92, 255}, BP_EMPTY{16, 40, 68, 255};

std::string need_label(const Need& nd) {
    std::string s = nd.role >= 0 ? ROLES[nd.role].group : CAT_LABELS[nd.cat];
    if (nd.weight > 0) s = std::string(WEIGHT_LABELS[nd.weight]) + " " + std::string(1, (char)tolower(s[0])) + s.substr(1);
    return s;
}

std::string cap_upper(std::string s) {
    for (auto& ch : s) ch = (char)toupper((unsigned char)ch);
    return s;
}
std::string slot_title(std::string s) {
    std::replace(s.begin(), s.end(), '_', ' ');
    if (s == "weapon2") s = "weapon II";
    if (s == "aa") s = "anti-air";
    if (s == "antitank") s = "anti-tank";
    if (s == "gear") s = "field gear";
    if (s == "body") s = "bodywork";
    if (!s.empty()) s[0] = (char)toupper((unsigned char)s[0]);
    return s;
}
std::string slot_icon(const std::string& s) {
    if (s == "engine") return "gear";
    if (s == "armour") return "shield";
    if (s == "aa" || s == "antitank" || s == "gun" || s == "barrel" || s == "battery" || s == "secondary") return "cannon";
    if (s == "weapon" || s == "weapon2" || s == "support") return "rifle";
    if (s == "torpedo") return "drop";
    if (s == "suspension" || s == "carriage" || s == "body") return "rail";
    if (s == "fire_control") return "eye";
    if (s == "turret") return "tank";
    if (s == "gear") return "helmet";
    if (s == "implement") return "wheat";
    return "radio";
}

// a dimension line with arrowheads, extension lines and its figure
void dim_line(UI& ui, Vector2 a, Vector2 b, const std::string& text, Color c) {
    Vector2 d = Vector2Subtract(b, a);
    float len = Vector2Length(d);
    if (len < 6) return;
    d = Vector2Scale(d, 1 / len);
    Vector2 n{-d.y, d.x};
    DrawLineEx(a, b, 1, c);
    for (int e : {0, 1}) {
        Vector2 at = e ? b : a, in = e ? Vector2Scale(d, -7) : Vector2Scale(d, 7);
        DrawTriangle(at, Vector2Add(Vector2Add(at, in), Vector2Scale(n, 2.4f)), Vector2Add(Vector2Add(at, in), Vector2Scale(n, -2.4f)), c);
        DrawTriangle(at, Vector2Add(Vector2Add(at, in), Vector2Scale(n, -2.4f)), Vector2Add(Vector2Add(at, in), Vector2Scale(n, 2.4f)), c);
        DrawLineEx(Vector2Add(at, Vector2Scale(n, -5)), Vector2Add(at, Vector2Scale(n, 5)), 1, c);
    }
    float tw = ui.measure(text, 12);
    Vector2 m{(a.x + b.x) / 2, (a.y + b.y) / 2};
    DrawRectangleRec(Rectangle{m.x - tw / 2 - 3, m.y - 7, tw + 6, 14}, BP_GROUND);
    ui.text(m.x - tw / 2, m.y - 7.5f, text, 12, c);
}
void dash_dot(Vector2 a, Vector2 b, Color c) {
    Vector2 d = Vector2Subtract(b, a);
    float len = Vector2Length(d);
    if (len < 1) return;
    d = Vector2Scale(d, 1 / len);
    for (float t = 0; t < len; t += 22) {
        DrawLineEx(Vector2Add(a, Vector2Scale(d, t)), Vector2Add(a, Vector2Scale(d, std::min(len, t + 12))), 1, c);
        if (t + 16 < len) DrawCircleV(Vector2Add(a, Vector2Scale(d, t + 16)), 0.8f, c);
    }
}
void blueprint_ground(UI& ui, Rectangle r) {
    DrawRectangleRec(r, BP_GROUND);
    // the paper fades darker towards its edges, as a real print does
    DrawRectangleGradientH((int)r.x, (int)r.y, (int)(r.width * 0.12f), (int)r.height, Color{4, 14, 30, 110}, Color{4, 14, 30, 0});
    DrawRectangleGradientH((int)(r.x + r.width * 0.88f), (int)r.y, (int)(r.width * 0.12f), (int)r.height, Color{4, 14, 30, 0}, Color{4, 14, 30, 110});
    DrawRectangleGradientV((int)r.x, (int)r.y, (int)r.width, (int)(r.height * 0.1f), Color{4, 14, 30, 90}, Color{4, 14, 30, 0});
    DrawRectangleGradientV((int)r.x, (int)(r.y + r.height * 0.9f), (int)r.width, (int)(r.height * 0.1f), Color{4, 14, 30, 0}, Color{4, 14, 30, 90});
    for (float x = r.x + 16; x < r.x + r.width; x += 16) DrawLineEx({x, r.y}, {x, r.y + r.height}, 1, fmodf(x - r.x, 80) < 1 ? BP_GRID2 : BP_GRID);
    for (float y = r.y + 16; y < r.y + r.height; y += 16) DrawLineEx({r.x, y}, {r.x + r.width, y}, 1, fmodf(y - r.y, 80) < 1 ? BP_GRID2 : BP_GRID);
    // the frame: a heavy line, a thin one, zones marked around the edge
    Rectangle f{r.x + 3, r.y + 3, r.width - 6, r.height - 6};
    DrawRectangleLinesEx(f, 1, BP_FAINT);
    DrawRectangleLinesEx(Rectangle{f.x + 12, f.y + 12, f.width - 24, f.height - 24}, 1.6f, Color{150, 200, 235, 170});
    const int ZX = 8, ZY = 6;
    for (int i = 0; i < ZX; i++) {
        float x = f.x + 12 + (f.width - 24) * (i + 0.5f) / ZX;
        ui.text_center(x, f.y, fmt("%d", i + 1), 10, BP_FAINT);
        ui.text_center(x, f.y + f.height - 12, fmt("%d", i + 1), 10, BP_FAINT);
        if (i) {
            float tx = f.x + 12 + (f.width - 24) * i / ZX;
            DrawLineEx({tx, f.y}, {tx, f.y + 12}, 1, BP_FAINT);
            DrawLineEx({tx, f.y + f.height - 12}, {tx, f.y + f.height}, 1, BP_FAINT);
        }
    }
    for (int i = 0; i < ZY; i++) {
        float y = f.y + 12 + (f.height - 24) * (i + 0.5f) / ZY;
        std::string l(1, (char)('A' + i));
        ui.text_center(f.x + 6, y - 6, l, 10, BP_FAINT);
        ui.text_center(f.x + f.width - 6, y - 6, l, 10, BP_FAINT);
        if (i) {
            float ty = f.y + 12 + (f.height - 24) * i / ZY;
            DrawLineEx({f.x, ty}, {f.x + 12, ty}, 1, BP_FAINT);
            DrawLineEx({f.x + f.width - 12, ty}, {f.x + f.width, ty}, 1, BP_FAINT);
        }
    }
}
void dashed_box(Rectangle r, Color c) {
    const float D = 6;
    for (float x = r.x; x < r.x + r.width; x += D * 2) {
        float e = std::min(x + D, r.x + r.width);
        DrawLineEx({x, r.y}, {e, r.y}, 1, c);
        DrawLineEx({x, r.y + r.height}, {e, r.y + r.height}, 1, c);
    }
    for (float y = r.y; y < r.y + r.height; y += D * 2) {
        float e = std::min(y + D, r.y + r.height);
        DrawLineEx({r.x, y}, {r.x, e}, 1, c);
        DrawLineEx({r.x + r.width, y}, {r.x + r.width, e}, 1, c);
    }
}

// a value next to its comparison: label, thin bar against the larger of the two, value, change
void stat_row(UI& ui, float x, float y, float wdt, const std::string& label, const std::string& val, float v, float base, float top, bool lower_better, bool show_diff, const std::string& diff) {
    float d = v - base;
    bool same = fabsf(d) < 0.0005f;
    Color c = same ? pal::TEXT : (d < 0) == lower_better ? pal::GOOD : pal::BAD;
    ui.text(x, y, ui.fit(label, 14, wdt * 0.5f), 14, pal::DIM);
    ui.text_right(x + wdt - (show_diff ? 48 : 0), y, val, 14, c, 1);
    if (show_diff && !same) ui.text_right(x + wdt, y + 1, diff, 12.5f, c);
    float bw = wdt, f = top > 0 ? std::min(1.0f, v / top) : 0;
    DrawRectangleRec(Rectangle{x, y + 17, bw, 2}, Color{0, 0, 0, 90});
    DrawRectangleRec(Rectangle{x, y + 17, bw * f, 2}, same ? Color{pal::ACCENT.r, pal::ACCENT.g, pal::ACCENT.b, 170} : c);
}

// what swapping a module changes, as coloured "+0.2 Soft attack" tokens on one line
void draw_deltas(UI& ui, const EqStats& a, const EqStats& b, float x, float y, float maxw, float fs) {
    float cx = x;
    int shown = 0;
    for (int k = 0; k < S_N; k++) {
        float d = b.v[k] - a.v[k];
        if (fabsf(d) < 0.0005f) continue;
        bool pct = k == S_RELIAB || k == S_HARDNESS;
        std::string val = pct ? fmt("%+.0f%%", d * 100) : fabsf(d) >= 10 ? fmt("%+.0f", d) : fmt("%+.2g", d);
        std::string s = val + " " + STAT_LABELS[k];
        float tw = ui.measure(s, fs);
        if (cx + tw > x + maxw) { if (shown) ui.text(cx, y, "...", fs, pal::DIM); return; }
        bool good = k == S_COST ? d < 0 : d > 0;
        ui.text(cx, y, s, fs, good ? pal::GOOD : pal::BAD);
        cx += tw + 9;
        shown++;
    }
    if (!shown) ui.text(x, y, "No change in its numbers", fs, pal::DIM);
}

}  // namespace

// =====================================================================================================
void App::prod_designs(Rectangle body) {
    Nation& N = me();
    const float LW = 232, SW = 276;
    Rectangle L{body.x, body.y, LW, body.height};
    Rectangle B{L.x + LW + 8, body.y, body.width - LW - SW - 16, body.height};
    Rectangle R{B.x + B.width + 8, body.y, SW, body.height};

    // an open list owns the mouse: nothing under it reacts, and a click outside closes it
    Vector2 real_mouse = ui.mouse;
    bool real_click = ui.clicked, real_rclick = ui.rclicked;
    float real_wheel = ui.wheel;
    const bool list_was_open = des_pick != -1;
    if (des_pick != -1) { ui.mouse = {-9999, -9999}; ui.clicked = ui.rclicked = false; ui.wheel = 0; }

    // ---- left: the categories research has opened, and our designs in this one
    ui.panel(L);
    float x = L.x + 10, y = L.y + 10, wdt = L.width - 20;
    std::vector<int> cats;
    for (int c = 0; c < CAT_N; c++)
        for (size_t ch = 0; ch < w.chassis.size(); ch++)
            if (w.chassis[ch].cat == c && w.chassis_ok(w.player, (int)ch)) { cats.push_back(c); break; }
    if (cats.empty()) { ui.mouse = real_mouse; ui.clicked = real_click; ui.rclicked = real_rclick; ui.wheel = real_wheel; return; }
    if (std::find(cats.begin(), cats.end(), des_cat) == cats.end()) { des_cat = cats[0]; des_chassis = -1; des_from = -1; }
    float cw = std::min(40.0f, (wdt - 3 * (cats.size() - 1)) / cats.size());
    for (size_t i = 0; i < cats.size(); i++) {
        int c = cats[i];
        Rectangle b{x + i * (cw + 3), y, cw, 30};
        if (ui.button(b, "", true, CAT_LABELS[c], des_cat == c)) { des_cat = c; des_chassis = -1; des_from = -1; }
        draw_icon(CAT_ICON[c], b.x + b.width / 2, b.y + 15, 18, des_cat == c ? pal::ACCENT : pal::TEXT);
    }
    y += 38;
    ui.text(x, y, ui.fit(CAT_LABELS[des_cat], 16, wdt, 1), 16, pal::ACCENT, 1);
    y += 24;
    if (ui.button(Rectangle{x, y, wdt, 26}, "Design the newest of all", true,
                  "For every role, a design on our newest chassis with the best modules we have,\nwhere it beats what we already have. Old designs stay; retire them yourself.")) {
        size_t before = w.designs.size();
        w.refresh_designs(w.player, true);
        size_t made = w.designs.size() - before;
        say(made ? fmt("%zu new design%s.", made, made > 1 ? "s" : "") : std::string("Our designs are already the best we can make."));
    }
    y += 32;
    static float list_off = 0;
    float ly0 = y;
    ui.scissor(Rectangle{L.x, ly0, L.width, L.y + L.height - ly0 - 6});
    y -= list_off;
    for (auto& d : w.designs) {
        if (d.nation != w.player || w.chassis[d.chassis].cat != des_cat) continue;
        if (y + 40 < ly0 || y > L.y + L.height) { y += 43; continue; }
        Rectangle row{x, y, wdt - 26, 40};
        std::string tip = fmt("%s, %d. Click to open it on the drawing board.", w.design_kind(d).c_str(), d.year);
        if (ui.button(row, "", true, tip, des_from == d.id)) {
            des_from = d.id;
            des_chassis = d.chassis;
            des_role = d.role;
            des_mods = d.mods;
            des_name = d.name + " II";
        }
        ui.text(row.x + 8, row.y + 3, ui.fit(d.name, 14.5f, row.width - 14, 1), 14.5f, d.obsolete ? pal::DIM : pal::TEXT, 1);
        ui.text(row.x + 8, row.y + 21, ui.fit(w.design_kind(d) + fmt(" · %d", d.year) + (d.obsolete ? " · retired" : ""), 12, row.width - 14), 12, pal::DIM);
        if (ui.button(Rectangle{x + wdt - 23, y, 23, 40}, d.obsolete ? "+" : "x", true, d.obsolete ? "Restore: use it again" : "Retire: hidden from new lines; divisions prefer newer designs"))
            w.designs[d.id].obsolete = !d.obsolete;
        y += 43;
    }
    EndScissorMode();
    list_off = ui.scroll(Rectangle{L.x, ly0, L.width, L.y + L.height - ly0 - 6}, list_off, y + list_off - ly0 + 6);

    // ---- what is on the board: if nothing yet, the newest chassis we may build, ready with our best modules
    if (des_chassis < 0 || w.chassis[des_chassis].cat != des_cat || !w.chassis_ok(w.player, des_chassis)) {
        int best = -1;
        for (size_t c = 0; c < w.chassis.size(); c++)
            if (w.chassis[c].cat == des_cat && w.chassis_ok(w.player, (int)c) && (best < 0 || w.chassis[c].year >= w.chassis[best].year)) best = (int)c;
        des_chassis = best;
        des_role = w.chassis[best].roles[0];
        des_mods = w.best_modules(w.player, best, des_role);
        des_from = -1;
        des_name.clear();
    }
    const Chassis& C = w.chassis[des_chassis];
    if (std::find(C.roles.begin(), C.roles.end(), des_role) == C.roles.end()) des_role = C.roles[0];
    des_mods.resize(C.slots.size(), -1);
    auto slot_opts = [&](int s) {
        std::vector<int> o;
        for (size_t m = 0; m < w.modules.size(); m++)
            if (w.module_fits((int)m, des_chassis, s, des_role) && w.module_ok(w.player, (int)m)) o.push_back((int)m);
        return o;
    };
    for (int s = 0; s < (int)C.slots.size(); s++) {    // a role change can leave a module that no longer fits
        auto o = slot_opts(s);
        if (des_mods[s] >= 0 && std::find(o.begin(), o.end(), des_mods[s]) == o.end()) des_mods[s] = -1;
    }
    Design t;
    t.chassis = des_chassis;
    t.role = des_role;
    t.mods = des_mods;
    w.design_stats(t);

    // what the sheet and the numbers show: while a module list is open and a row is under the mouse, that row. Filled in by
    // the list below, so read it here
    static int preview_mod = -1, preview_slot = -1;
    Design show = t;
    if (des_pick >= 0 && des_pick == preview_slot && des_pick < (int)show.mods.size()) { show.mods[des_pick] = preview_mod; w.design_stats(show); }
    preview_slot = -1;

    // ---- the middle: the sheet. The machine in three views, its parts numbered, a table of figures and the title block
    blueprint_ground(ui, B);
    if (ui.hover(B)) ui.over = true;
    Rectangle chsel{B.x + 22, B.y + 20, B.width * 0.42f, 28};
    if (ui.button(chsel, "", true, "Change the chassis: what the design is built on") ) { des_pick = -2; des_pick_off = 0; }
    ui.text(chsel.x + 10, chsel.y + 4, ui.fit(C.name, 16, chsel.width - 34, 1), 16, pal::TEXT, 1);
    DrawTriangle({chsel.x + chsel.width - 20, chsel.y + 11}, {chsel.x + chsel.width - 15, chsel.y + 19}, {chsel.x + chsel.width - 10, chsel.y + 11}, pal::ACCENT);
    if (C.roles.size() > 1) {
        float rx = chsel.x + chsel.width + 8, rw = std::min(116.0f, (B.x + B.width - 22 - rx) / C.roles.size());
        for (size_t k = 0; k < C.roles.size(); k++)
            if (ui.button(Rectangle{rx + k * rw, chsel.y, rw - 3, 28}, ui.fit(ROLES[C.roles[k]].label, 14, rw - 10), true, ROLES[C.roles[k]].label, des_role == C.roles[k])) des_role = C.roles[k];
    }
    // what is fitted, as the drawing reads it: a longer gun really lengthens it, a radio brings its aerial
    Fit fit;
    for (int s = 0; s < (int)C.slots.size(); s++) {
        std::string nm = s < (int)show.mods.size() && show.mods[s] >= 0 ? w.modules[show.mods[s]].name : "";
        for (char& ch : nm) ch = (char)tolower((unsigned char)ch);
        fit.by_slot[C.slots[s]] = nm;
    }
    const float BW = std::min(166.0f, B.width * 0.22f), BH = 66, GAP = 8, FOOT_H = 118;
    float top = B.y + 56, avail = B.height - 56 - FOOT_H - 18;
    // a long, low machine (a ship, a train, a bomber's plan) gets the whole width, its slots in rows above and below
    // it; anything else has its slots in columns either side
    bool in_rows;
    {
        BpResult probe = blueprint_render(C, des_role, fit, BPV_MAIN, Rectangle{0, 0, 1000, 1000}, BP_INK, BP_GROUND, 1, false);
        in_rows = probe.box.width > probe.box.height * 2.4f;
    }
    float stage_x = in_rows ? B.x + 20 : B.x + 20 + BW, stage_w = in_rows ? B.width - 40 : B.width - 40 - 2 * BW;
    // the main view takes the upper part of the stage, the two lesser views the lower
    float main_h = avail * (in_rows ? 0.74f : 0.66f), side_h = avail - main_h;
    const float SLOT_ROW_H = 46;        // a slot box in a row: one line for the module's name
    float row_top = top + 2, row_bot = top + main_h - SLOT_ROW_H - 26;
    Rectangle stage = in_rows ? Rectangle{stage_x + 14, row_top + SLOT_ROW_H + 26, stage_w - 60, row_bot - row_top - SLOT_ROW_H - 54}
                           : Rectangle{stage_x + 14, top + 14, stage_w - 46, main_h - 44};
    BpResult lay = blueprint_render(C, des_role, fit, BPV_MAIN, stage, BP_INK, BP_GROUND, 1.5f, false);
    // slots: split left and right by where the part sits on the drawing, top to bottom, so leader lines do not cross
    int ns = (int)C.slots.size();
    std::vector<int> order(ns);
    std::vector<Vector2> anchor(ns);
    for (int s = 0; s < ns; s++) { order[s] = s; anchor[s] = lay.marks[C.slots[s]]; }
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return anchor[a].x < anchor[b].x; });
    int nleft = (ns + 1) / 2;
    std::vector<int> colL(order.begin(), order.begin() + nleft), colR(order.begin() + nleft, order.end());
    auto by_y = [&](int a, int b) { return anchor[a].y < anchor[b].y; };
    std::stable_sort(colL.begin(), colL.end(), by_y);
    std::stable_sort(colR.begin(), colR.end(), by_y);
    bool plan_main = blueprint_plan_main(C, des_role);
    // centre line through the machine, its shadow, then the machine itself
    Rectangle mb = lay.box;
    float cy = plan_main ? mb.y + mb.height / 2 : mb.y + mb.height * 0.5f;
    dash_dot({mb.x - 10, cy}, {mb.x + mb.width + 10, cy}, BP_FAINT);
    if (plan_main) dash_dot({mb.x + mb.width / 2, mb.y - 6}, {mb.x + mb.width / 2, mb.y + mb.height + 6}, BP_FAINT);
    float lw = std::clamp(stage.width / 380, 1.2f, 2.2f);
    blueprint_render(C, des_role, fit, BPV_MAIN, Rectangle{stage.x + 2, stage.y + 3, stage.width, stage.height}, Color{0, 0, 0, 90}, Color{0, 0, 0, 0}, lw, true);
    blueprint_render(C, des_role, fit, BPV_MAIN, stage, BP_INK, BP_GROUND, lw, true);
    int year_now = w.year();
    BpFigures dm = blueprint_figures(C, des_role, fit);
    auto metres = [](float v) { return v >= 100 ? fmt("%.0f m", v) : v >= 10 ? fmt("%.1f m", v) : fmt("%.2f m", v); };
    // overall dimensions, against the drawing
    {
        float dy = mb.y + mb.height + 12, rx = mb.x + mb.width + 12;
        Color dc{150, 200, 235, 220};
        dim_line(ui, {mb.x, dy}, {mb.x + mb.width, dy}, metres(plan_main ? dm.width : dm.length), dc);
        dim_line(ui, {rx, mb.y}, {rx, mb.y + mb.height}, metres(plan_main ? dm.length : dm.height), dc);
    }
    ui.text(stage_x + 14, top - 2, blueprint_view_name(C, des_role, BPV_MAIN), 11, BP_FAINT, 1);
    ui.text_right(stage_x + stage_w - 14, top - 2, ui.fit(cap_upper(dm.model), 11, stage_w * 0.5f), 11, BP_FAINT, 1);
    ui.text_center(stage_x + stage_w / 2, top + main_h - 16, "Click a slot to fit a module. Right-click empties it. Dimensions are illustrative.", 10.5f, Color{120, 170, 210, 120});
    // the two lesser views
    {
        float vy = top + main_h + 4, vh = side_h - 8, vw = (stage_w - 12) / 2;
        Rectangle v1{stage_x + 2, vy + 14, vw, vh - 16}, v2{stage_x + 10 + vw, vy + 14, vw, vh - 16};
        Color thin{176, 222, 250, 210};
        blueprint_render(C, des_role, fit, BPV_SECOND, v1, thin, BP_GROUND, 1.1f, true);
        BpResult fr = blueprint_render(C, des_role, fit, BPV_FRONT, v2, thin, BP_GROUND, 1.1f, true);
        dash_dot({fr.box.x + fr.box.width / 2, fr.box.y - 6}, {fr.box.x + fr.box.width / 2, fr.box.y + fr.box.height + 4}, BP_FAINT);
        ui.text(v1.x + 2, vy, blueprint_view_name(C, des_role, BPV_SECOND), 11, BP_FAINT, 1);
        ui.text(v2.x + 2, vy, blueprint_view_name(C, des_role, BPV_FRONT), 11, BP_FAINT, 1);
        DrawLineEx({stage_x + 2, vy - 3}, {stage_x + stage_w - 2, vy - 3}, 1, Color{120, 170, 210, 70});
    }
    // ---- the foot of the sheet: parts schedule, figures, title block
    Rectangle foot_r{B.x + 16, B.y + B.height - FOOT_H - 16, B.width - 32, FOOT_H};
    {
        DrawRectangleRec(foot_r, Color{10, 28, 50, 235});
        DrawRectangleLinesEx(foot_r, 1.4f, Color{150, 200, 235, 170});
        float w1 = foot_r.width * 0.31f, w2 = foot_r.width * 0.35f;
        Rectangle parts{foot_r.x, foot_r.y, w1, foot_r.height}, spec{foot_r.x + w1, foot_r.y, w2, foot_r.height}, title{foot_r.x + w1 + w2, foot_r.y, foot_r.width - w1 - w2, foot_r.height};
        DrawLineEx({spec.x, spec.y}, {spec.x, spec.y + spec.height}, 1, BP_FAINT);
        DrawLineEx({title.x, title.y}, {title.x, title.y + title.height}, 1, BP_FAINT);
        // parts schedule
        ui.text(parts.x + 6, parts.y + 3, "ITEM  PART", 10.5f, BP_FAINT, 1);
        DrawLineEx({parts.x, parts.y + 16}, {parts.x + parts.width, parts.y + 16}, 1, Color{120, 170, 210, 90});
        float py = parts.y + 18, rowh = std::min(14.0f, (parts.height - 20) / std::max(1, ns));
        for (int i = 0; i < ns; i++) {
            int mid = show.mods[i];
            ui.text(parts.x + 10, py, fmt("%d", i + 1), 11, pal::ACCENT, 1);
            ui.text(parts.x + 30, py, ui.fit(mid >= 0 ? w.modules[mid].name : slot_title(C.slots[i]) + " (none)", 11, parts.width - 34), 11, mid >= 0 ? BP_INK : BP_FAINT);
            py += rowh;
        }
        // the figures: two columns, the machine's size and what it does
        ui.text(spec.x + 6, spec.y + 3, "PARTICULARS", 10.5f, BP_FAINT, 1);
        DrawLineEx({spec.x, spec.y + 16}, {spec.x + spec.width, spec.y + 16}, 1, Color{120, 170, 210, 90});
        float sw = (spec.width - 10) / 2;
        std::string mass = C.cat == CAT_SMALL_ARMS ? fmt("%.1f kg", dm.mass) : dm.mass >= 100 ? fmt("%.0f t", dm.mass) : fmt("%.1f t", dm.mass);
        std::vector<std::pair<std::string, std::string>> L = {{"Length", metres(dm.length)}, {"Height", metres(dm.height)}, {dm.width_name, metres(dm.width)},
                                                              {C.cat == CAT_SHIP ? "Displacement" : "Mass", mass}, {"Crew", dm.crew > 0 ? fmt("%d", dm.crew) : std::string("none")}};
        if (dm.calibre > 0) L.push_back({"Calibre", dm.calibre < 20 ? fmt("%.2f mm", dm.calibre) : fmt("%.0f mm", dm.calibre)});
        std::vector<std::pair<std::string, std::string>> Rr;
        auto add_stat = [&](int k, const char* name, const char* format, float mult = 1) {
            if (show.st.v[k] != 0 && Rr.size() < 6) Rr.push_back({name, fmt(format, show.st.v[k] * mult)});
        };
        add_stat(S_SPEED, C.cat == CAT_SHIP ? "Speed" : "Speed", C.cat == CAT_SHIP ? "%.0f kn" : "%.0f km/h");
        if (show.st.v[S_RANGE] < 20000) add_stat(S_RANGE, "Range", "%.0f km");     // ships' fuel is not worth a figure
        add_stat(S_ARMOR, "Armour", "%.0f mm");
        add_stat(S_PIERCE, "Penetration", "%.0f mm");
        add_stat(S_NAVAL, "Main battery", "%.1f");
        add_stat(S_TORP, "Torpedoes", "%.1f");
        add_stat(S_AIR, "Air attack", "%.1f");
        add_stat(S_BOMB, "Bomb load", "%.1f");
        add_stat(S_SOFT, "Soft attack", "%.1f");
        add_stat(S_RELIAB, "Reliability", "%.0f%%", 100);
        auto cols = [&](const std::vector<std::pair<std::string, std::string>>& v, float cx) {
            float sy = spec.y + 19;
            for (auto& [k, val] : v) {
                ui.text(cx + 5, sy, k, 11, BP_FAINT);
                ui.text_right(cx + sw - 4, sy, val, 11, BP_INK, 1);
                sy += 14;
            }
        };
        cols(L, spec.x);
        cols(Rr, spec.x + sw + 6);
        ui.text(spec.x + 5, spec.y + spec.height - 15, ui.fit(fmt("Cost %.2f · coal %.1f · iron %.1f · oil %.1f", show.st.v[S_COST], show.st.res[0], show.st.res[1], show.st.res[2]), 11, spec.width - 10), 11, BP_FAINT);
        // title block
        bool saved = des_from >= 0 && w.designs[des_from].chassis == des_chassis && w.designs[des_from].role == des_role && w.designs[des_from].mods == des_mods;
        int rev = 0;
        for (auto& d : w.designs) rev += d.nation == w.player && d.chassis == des_chassis;
        float tx = title.x + 7, tw = title.width - 14, ty = title.y + 4;
        ui.text(tx, ty, ui.fit(cap_upper(N.adj) + " WAR OFFICE", 11, tw), 11, BP_FAINT, 1);
        ty += 14;
        ui.text(tx, ty, ui.fit(des_name.empty() ? w.design_kind(t) : des_name, 15, tw, 1), 15, BP_INK, 1);
        ty += 19;
        ui.text(tx, ty, ui.fit(w.design_kind(show), 12, tw), 12, BP_FAINT);
        ty += 16;
        DrawLineEx({title.x, ty}, {title.x + title.width, ty}, 1, Color{120, 170, 210, 90});
        ty += 3;
        auto cell = [&](float cx, float cw, const char* k, const std::string& v) {
            ui.text(cx, ty, k, 9.5f, BP_FAINT);
            ui.text(cx, ty + 11, ui.fit(v, 11.5f, cw), 11.5f, BP_INK, 1);
        };
        float c3 = tw / 3;
        cell(tx, c3 - 4, "DRAWING NO.", fmt("%d-%03d", year_now, des_chassis));
        cell(tx + c3, c3 - 4, "SCALE", fmt("1 : %d", dm.scale));
        cell(tx + c3 * 2, c3, "REV.", std::string(1, (char)('A' + std::min(25, rev))));
        ty += 27;
        ui.text(tx, ty, ui.fit(w.date(), 11, tw * 0.6f), 11, BP_FAINT);
        // the stamps
        Color stamp = saved ? Color{120, 214, 150, 210} : Color{226, 100, 84, 210};
        const char* st = saved ? "APPROVED" : "DRAFT";
        float sz = 15;
        Vector2 ms = MeasureTextEx(ui.bold, st, sz, 1);
        Vector2 at{title.x + title.width - ms.x / 2 - 16, title.y + title.height - 16};
        DrawTextPro(ui.bold, st, at, {ms.x / 2, ms.y / 2}, -7, sz, 1, stamp);
        DrawRectanglePro(Rectangle{at.x, at.y, ms.x + 10, ms.y + 5}, {(ms.x + 10) / 2, (ms.y + 5) / 2}, -7, Color{stamp.r, stamp.g, stamp.b, 26});
        Color sec{226, 100, 84, 170};
        ui.text_right(B.x + B.width - 24, B.y + 3 + 0, "SECRET", 10, sec, 1);
    }

    // one slot's box, with its leader from `edge` by way of `elbow` to the part on the drawing
    auto slot_box = [&](int s, Rectangle b, Vector2 edge, Vector2 elbow) {
        {
            int mid = des_mods[s];
            auto opts = slot_opts(s);
            bool h_ = ui.hover(b), open = des_pick == s;
            Vector2 a = anchor[s];
            Color leader = mid >= 0 ? Color{176, 222, 250, 200} : Color{120, 170, 210, 110};
            if (h_ || open) leader = pal::ACCENT;
            DrawLineEx(edge, elbow, 1.2f, leader);
            DrawLineEx(elbow, a, 1.2f, leader);
            // the item's number in a balloon on the drawing, the same number on its box
            DrawCircleV(a, 8, BP_GROUND);
            DrawRing(a, 7.2f, 8.4f, 0, 360, 24, leader);
            ui.text_center(a.x, a.y - 7, fmt("%d", s + 1), 11, leader, 1);
            DrawRectangleRec({b.x + 2, b.y + 3, b.width, b.height}, Color{0, 0, 0, 100});
            DrawRectangleRec(b, mid >= 0 ? (h_ ? Color{36, 76, 118, 255} : BP_FILLED) : (h_ ? Color{24, 52, 84, 255} : BP_EMPTY));
            if (mid >= 0) DrawRectangleLinesEx(b, open ? 2 : 1, open || h_ ? pal::ACCENT : BP_INK);
            else dashed_box(b, open || h_ ? pal::ACCENT : BP_FAINT);
            ui.text(b.x + 8, b.y + 4, ui.fit(fmt("%d  ", s + 1) + cap_upper(slot_title(C.slots[s])), 11.5f, b.width - 16, 1), 11.5f, mid >= 0 ? pal::ACCENT : BP_FAINT, 1);
            bool low = b.height < 60;
            draw_icon(slot_icon(C.slots[s]), b.x + 19, b.y + (low ? 29 : 38), low ? 18 : 22, mid >= 0 ? BP_INK : BP_FAINT);
            float tx = b.x + 35, tw = b.width - 40;
            if (mid >= 0) {
                const Module& M = w.modules[mid];
                float fs = 14;
                float room = low ? 20 : 38;
                while (fs > 10.5f && ui.wrap(Rectangle{0, 0, tw, 100}, M.name, fs, BLANK, false) > room) fs -= 0.5f;
                if (low) ui.text(tx, b.y + 20, ui.fit(M.name, fs, tw), fs, pal::TEXT);
                else ui.wrap(Rectangle{tx, b.y + 21, tw, 40}, M.name, fs, pal::TEXT);
            } else {
                std::string e = opts.empty() ? "Nothing to fit" : "Empty";
                if (low && !opts.empty()) e += fmt("  (%zu available)", opts.size());
                ui.text(tx, b.y + (low ? 21 : 24), ui.fit(e, 14, tw), 14, BP_FAINT);
                if (!opts.empty() && !low) ui.text(tx, b.y + 42, fmt("%zu available", opts.size()), 12, BP_FAINT);
            }
            if (h_) {
                ui.over = true;
                std::string tip = slot_title(C.slots[s]);
                if (mid >= 0) {
                    const Module& M = w.modules[mid];
                    tip = M.name + (M.desc.empty() ? "" : "\n" + M.desc);
                    for (int k = 0; k < S_N; k++) {
                        if (M.add.v[k] != 0) tip += fmt("\n%s %+g", STAT_LABELS[k], M.add.v[k]);
                        if (M.mult[k] != 1) tip += fmt("\n%s x%.2f", STAT_LABELS[k], M.mult[k]);
                    }
                }
                ui.set_tip(tip);
                if (ui.clicked) { des_pick = s; des_pick_off = 0; }
                if (ui.rclicked) des_mods[s] = -1;
            }
        }
    };
    auto draw_col = [&](const std::vector<int>& col, float bx, bool left_side) {
        float h = col.size() * BH + (col.size() > 1 ? (col.size() - 1) * GAP : 0);
        float gap = GAP, by = top + std::max(0.0f, (avail - h) / 2);
        if (h > avail && col.size() > 1) { gap = std::max(2.0f, (avail - col.size() * BH) / (col.size() - 1)); by = top; }
        for (int s : col) {
            Rectangle b{bx, by, BW, BH};
            Vector2 edge{left_side ? b.x + b.width : b.x, b.y + b.height / 2};
            slot_box(s, b, edge, Vector2{edge.x + (left_side ? 14.f : -14.f), edge.y});
            by += BH + gap;
        }
    };
    auto draw_row = [&](std::vector<int> row, float by, bool above) {
        std::stable_sort(row.begin(), row.end(), [&](int a, int b) { return anchor[a].x < anchor[b].x; });
        float span = stage_w - 20, w1 = std::min(BW, (span - (row.size() - 1) * GAP) / std::max<size_t>(1, row.size()));
        float total = row.size() * w1 + (row.size() > 1 ? (row.size() - 1) * GAP : 0), bx = stage_x + 10 + (span - total) / 2;
        for (int s : row) {
            Rectangle b{bx, by, w1, SLOT_ROW_H};
            Vector2 edge{b.x + b.width / 2, above ? b.y + b.height : b.y};
            slot_box(s, b, edge, Vector2{edge.x, edge.y + (above ? 10.f : -10.f)});
            bx += w1 + GAP;
        }
    };
    if (in_rows) {
        // above or below by where the part sits on the machine; at most four in a row
        std::vector<int> up, down;
        float mid_y = lay.box.y + lay.box.height * 0.5f;
        std::vector<int> by_y = order;
        std::stable_sort(by_y.begin(), by_y.end(), [&](int a, int b) { return anchor[a].y < anchor[b].y; });
        for (int k = 0; k < ns; k++) {
            int s = by_y[k];
            bool want_up = anchor[s].y < mid_y;
            if ((want_up && (int)up.size() < 4) || (int)down.size() >= 4 || (!want_up && false)) up.push_back(s);
            else down.push_back(s);
        }
        while (up.size() > down.size() + 1) { down.push_back(up.back()); up.pop_back(); }
        while (down.size() > up.size() + 1) { up.push_back(down.front()); down.erase(down.begin()); }
        draw_row(up, row_top, true);
        draw_row(down, row_bot, false);
    } else {
        draw_col(colL, B.x + 20, true);
        draw_col(colR, B.x + B.width - 20 - BW, false);
    }

    // ---- right: the numbers, against the design it started from (or our best of the kind)
    ui.panel(R);
    x = R.x + 12, y = R.y + 10, wdt = R.width - 24;
    const Design* base = des_from >= 0 ? &w.designs[des_from] : nullptr;
    if (!base)
        for (auto& d : w.designs)
            if (d.nation == w.player && !d.obsolete && d.role == des_role && w.chassis[d.chassis].cat == des_cat && w.chassis[d.chassis].weight == C.weight &&
                (!base || w.design_score(d) > w.design_score(*base)))
                base = &d;
    if (des_name.empty()) des_name = fmt("%s %s M%d", N.adj.c_str(), w.design_kind(t).c_str(), w.year());
    ui.text(x, y, "Design name", 13, pal::DIM);
    y += 17;
    ui.text_input(Rectangle{x, y, wdt, 30}, des_name, 101);
    y += 38;
    ui.text(x, y, ui.fit(w.design_kind(show), 16, wdt, 1), 16, pal::ACCENT, 1);
    y += 21;
    ui.text(x, y, ui.fit(base ? "Against " + base->name : "No design of this kind yet", 12.5f, wdt), 12.5f, pal::DIM);
    y += 20;
    ui.separator(x, y, wdt);
    y += 6;
    float foot = R.y + R.height - 118;
    ui.scissor(Rectangle{R.x, y, R.width, foot - y});
    for (int k = 0; k < S_N; k++) {
        if (k == S_COST) continue;
        float v = show.st.v[k], b = base ? base->st.v[k] : v;
        if (v == 0 && b == 0) continue;
        if (y + 22 > foot) break;
        bool pct = k == S_RELIAB || k == S_HARDNESS;
        std::string val = pct ? fmt("%.0f%%", v * 100) : fmt(v >= 100 ? "%.0f" : "%.2f", v);
        float d = v - b;
        std::string diff = pct ? fmt("%+.0f%%", d * 100) : fabsf(d) >= 10 ? fmt("%+.0f", d) : fmt("%+.2f", d);
        float top_v = std::max(fabsf(v), fabsf(b));     // the bar: against the best of this category we have designed
        for (auto& d : w.designs)
            if (d.nation == w.player && w.chassis[d.chassis].cat == des_cat) top_v = std::max(top_v, fabsf(d.st.v[k]));
        stat_row(ui, x, y, wdt, STAT_LABELS[k], val, v, b, top_v, false, base != nullptr, diff);
        y += 24;
    }
    EndScissorMode();
    y = foot;
    ui.separator(x, y, wdt);
    y += 6;
    {
        float v = show.st.v[S_COST], b = base ? base->st.v[S_COST] : v;
        ui.text(x, y, "Cost each", 14, pal::DIM);
        Color c = fabsf(v - b) < 0.0005f ? pal::TEXT : v < b ? pal::GOOD : pal::BAD;
        ui.text_right(x + wdt, y, fmt("%.2f", v), 15, c, 1);
        y += 19;
        ui.text(x, y, fmt("Coal %.1f · iron %.1f · oil %.1f", show.st.res[0], show.st.res[1], show.st.res[2]), 13, pal::DIM);
        y += 22;
    }
    float half = (wdt - 6) / 2;
    if (ui.button(Rectangle{x, y, half, 26}, "Best modules", true, "Fill every slot with what adds most for this role")) des_mods = w.best_modules(w.player, des_chassis, des_role);
    if (ui.button(Rectangle{x + half + 6, y, half, 26}, "Clear slots", true, "Take every module out")) std::fill(des_mods.begin(), des_mods.end(), -1);
    y += 30;
    if (ui.button(Rectangle{x, y, wdt, 30}, "Save design", !des_name.empty(), "Add it to our designs; build it on a production line", true)) {
        des_from = w.create_design(w.player, des_chassis, des_role, des_mods, des_name);
        say("Design saved: " + des_name);
        des_name = w.designs[des_from].name + " II";
    }

    // ---- the open list, last, over everything
    ui.mouse = real_mouse;
    bool was_click = real_click;
    ui.clicked = real_click;
    ui.rclicked = real_rclick;
    ui.wheel = real_wheel;
    if (des_pick == -1) return;
    // the click that opened the list this frame must not also count as a click on it or outside it
    if (!list_was_open) { was_click = real_click = false; ui.clicked = false; }
    struct Row { std::string name, sub; int id; bool ok; };
    std::vector<Row> rows;
    std::string title;
    if (des_pick == -2) {
        title = "Chassis: " + std::string(CAT_LABELS[des_cat]);
        std::vector<int> ids;
        for (size_t c = 0; c < w.chassis.size(); c++) if (w.chassis[c].cat == des_cat) ids.push_back((int)c);
        std::stable_sort(ids.begin(), ids.end(), [&](int a, int b) {
            bool oa = w.chassis_ok(w.player, a), ob = w.chassis_ok(w.player, b);
            if (oa != ob) return oa;
            return oa ? w.chassis[a].year > w.chassis[b].year : w.chassis[a].year < w.chassis[b].year;
        });
        for (int c : ids) {
            const Chassis& H = w.chassis[c];
            bool ok = w.chassis_ok(w.player, c);
            std::string sub = fmt("%d", H.year);
            if (H.weight) sub += std::string(" · ") + WEIGHT_LABELS[H.weight];
            sub += fmt(" · %zu slots", H.slots.size());
            if (!ok && H.tech >= 0) sub = "Needs research: " + w.techs[H.tech].name;
            rows.push_back({H.name, sub, c, ok});
        }
    } else {
        title = slot_title(C.slots[des_pick]);
        rows.push_back({"Leave it empty", "", -1, true});
        for (size_t m = 0; m < w.modules.size(); m++)
            if (w.module_fits((int)m, des_chassis, des_pick, des_role) && w.module_ok(w.player, (int)m)) rows.push_back({w.modules[m].name, "", (int)m, true});
        for (size_t m = 0; m < w.modules.size(); m++)
            if (w.module_fits((int)m, des_chassis, des_pick, des_role) && !w.module_ok(w.player, (int)m))
                rows.push_back({w.modules[m].name, w.modules[m].tech >= 0 ? "Needs research: " + w.techs[w.modules[m].tech].name : "Not available", (int)m, false});
    }
    const float RH = 44, PW = 380;
    float ph = std::min(B.height - 60, 44 + rows.size() * RH + 8);
    Rectangle P{B.x + B.width / 2 - PW / 2, B.y + 44, PW, ph};
    ui.panel(P, Color{22, 26, 34, 252});
    ui.text(P.x + 14, P.y + 10, ui.fit(title, 18, PW - 60, 1), 18, pal::ACCENT, 1);
    bool close = ui.button(Rectangle{P.x + PW - 36, P.y + 8, 26, 26}, "x");
    float vy0 = P.y + 42, vh = P.height - 48;
    Rectangle view{P.x, vy0, PW, vh};
    des_pick_off = ui.scroll(view, des_pick_off, rows.size() * RH + 4);
    ui.scissor(view);
    float ry = vy0 - des_pick_off;
    for (auto& rw : rows) {
        Rectangle row{P.x + 8, ry, PW - 16, RH - 3};
        bool hov = ui.hover(row) && ui.hover(view);
        bool cur = des_pick == -2 ? rw.id == des_chassis : rw.id == des_mods[des_pick];
        DrawRectangleRec(row, cur ? Color{62, 54, 36, 255} : hov && rw.ok ? pal::ROW_HI : pal::ROW);
        if (cur) DrawRectangleLinesEx(row, 1, pal::ACCENT);
        ui.text(row.x + 8, row.y + 3, ui.fit(rw.name, 15, row.width - 16, 1), 15, rw.ok ? pal::TEXT : pal::DIM, 1);
        if (!rw.sub.empty()) ui.text(row.x + 8, row.y + 22, ui.fit(rw.sub, 12.5f, row.width - 16), 12.5f, rw.ok ? pal::DIM : Color{170, 120, 100, 255});
        else if (des_pick >= 0 && rw.ok) {
            Design alt = t;
            alt.mods[des_pick] = rw.id;
            w.design_stats(alt);
            draw_deltas(ui, t.st, alt.st, row.x + 8, row.y + 22, row.width - 16, 12.5f);
        }
        if (hov && rw.ok && des_pick >= 0) { preview_slot = des_pick; preview_mod = rw.id; }
        if (hov && rw.ok && was_click) {
            if (des_pick == -2) {
                if (rw.id != des_chassis) {
                    des_chassis = rw.id;
                    des_role = w.chassis[rw.id].roles[0];
                    des_mods = w.best_modules(w.player, rw.id, des_role);
                    des_from = -1;
                    des_name.clear();
                }
            } else des_mods[des_pick] = rw.id;
            close = true;
        }
        ry += RH;
    }
    EndScissorMode();
    if (close || (was_click && !ui.hover(P)) || real_rclick || IsKeyPressed(KEY_ESCAPE)) des_pick = -1;
    ui.clicked = false;
    ui.over = true;
}

// =====================================================================================================
void App::prod_templates(Rectangle body) {
    Nation& N = me();
    const float LW = 232, SW = 276;
    Rectangle L{body.x, body.y, LW, body.height};
    Rectangle B{L.x + LW + 8, body.y, body.width - LW - SW - 16, body.height};
    Rectangle R{B.x + B.width + 8, body.y, SW, body.height};
    Vector2 real_mouse = ui.mouse;
    bool real_click = ui.clicked, real_rclick = ui.rclicked;
    float real_wheel = ui.wheel;
    const bool list_was_open = tmpl_pick != -1;
    if (tmpl_pick != -1) { ui.mouse = {-9999, -9999}; ui.clicked = ui.rclicked = false; ui.wheel = 0; }

    // ---- left: our templates, then ways to start a new one
    ui.panel(L);
    float x = L.x + 10, y = L.y + 10, wdt = L.width - 20;
    ui.text(x, y, "Templates", 18, pal::ACCENT, 1);
    y += 28;
    static float list_off = 0;
    float ly0 = y, lbot = L.y + L.height - 186;
    ui.scissor(Rectangle{L.x, ly0, L.width, lbot - ly0});
    y -= list_off;
    for (size_t t = 0; t < N.tmpls.size(); t++) {
        if (y + 40 < ly0 || y > lbot) { y += 43; continue; }
        int in_use = 0;
        for (auto& d : w.divs) in_use += d.alive && d.nation == w.player && d.tmpl == (int)t;
        Rectangle row{x, y, wdt, 40};
        if (ui.button(row, "", true, fmt("%s, %d divisions", N.tmpls[t].kind.c_str(), in_use), tmpl_sel == (int)t)) {
            tmpl_sel = (int)t;
            tmpl_edit = N.tmpls[t];
        }
        ui.text(row.x + 8, row.y + 3, ui.fit(N.tmpls[t].name, 14.5f, row.width - 14, 1), 14.5f, N.tmpls[t].obsolete ? pal::DIM : pal::TEXT, 1);
        ui.text(row.x + 8, row.y + 21, ui.fit(fmt("%d divisions · width %.0f", in_use, N.tmpls[t].width) + (N.tmpls[t].obsolete ? " · retired" : ""), 12, row.width - 14), 12, pal::DIM);
        y += 43;
    }
    EndScissorMode();
    list_off = ui.scroll(Rectangle{L.x, ly0, L.width, lbot - ly0}, list_off, y + list_off - ly0 + 6);
    y = lbot + 8;
    if (ui.button(Rectangle{x, y, wdt, 30}, tmpl_sel >= 0 ? "+ Copy this template" : "+ New template", true, "Start from the selected one, or from six infantry battalions")) {
        tmpl_edit = tmpl_sel >= 0 ? N.tmpls[tmpl_sel] : DivTemplate{};
        tmpl_edit.name = tmpl_sel >= 0 ? tmpl_edit.name + " (new)" : "New Division";
        tmpl_edit.obsolete = false;
        if (tmpl_edit.line.empty()) tmpl_edit.line.assign(6, w.battalion_idx["infantry"]);
        w.template_stats(tmpl_edit);
        tmpl_sel = -2;     // editing a new one
    }
    y += 36;
    ui.text(x, y, "Or from a standard pattern", 13, pal::DIM);
    y += 18;
    float pw = (wdt - 6) / 3;
    int pc = 0;
    std::map<std::string, int> newest;      // the latest pattern of each kind
    for (size_t i = 0; i < w.std_templates.size(); i++)
        if (w.template_ok(w.player, w.std_templates[i])) newest[w.std_templates[i].kind] = (int)i;
    for (auto& [kind, i] : newest) {
        const DivTemplate& S = w.std_templates[i];
        if (pc >= 9) break;
        if (ui.button(Rectangle{x + (pc % 3) * (pw + 3), y + (pc / 3) * 29, pw, 26}, ui.fit(S.kind, 13, pw - 8), true, S.name)) {
            tmpl_edit = S;
            tmpl_edit.name = N.adj + " " + S.name;
            tmpl_edit.obsolete = false;
            w.template_stats(tmpl_edit);
            tmpl_sel = -2;
        }
        pc++;
    }
    if (tmpl_sel == -1 && !N.tmpls.empty()) { tmpl_sel = 0; tmpl_edit = N.tmpls[0]; }
    if (tmpl_sel == -1) {
        ui.mouse = real_mouse; ui.clicked = real_click; ui.rclicked = real_rclick; ui.wheel = real_wheel;
        return;
    }
    DivTemplate& T = tmpl_edit;
    w.template_stats(T);

    // ---- the middle: the staff table. Support companies on the left, five columns of line battalions
    const Color TABLE{42, 44, 36, 255}, TABLE_LINE{120, 120, 88, 90}, PAPER{214, 206, 178, 255}, INK{38, 34, 26, 255};
    DrawRectangleRec(B, TABLE);
    for (float gy = B.y + 22; gy < B.y + B.height; gy += 22) DrawLineEx({B.x, gy}, {B.x + B.width, gy}, 1, Color{0, 0, 0, 30});
    DrawRectangleLinesEx(B, 1, pal::BORDER);
    if (ui.hover(B)) ui.over = true;
    float gx = B.x + 14, gy = B.y + 12, gw = B.width - 28;
    ui.text(gx, gy + 6, "Name", 14, pal::DIM);
    ui.text_input(Rectangle{gx + 44, gy, gw * 0.5f, 30}, T.name, 102);
    float kx = gx + 44 + gw * 0.5f + 16;
    ui.text(kx, gy + 6, "Map symbol", 14, pal::DIM);
    float kw = gx + gw - kx - 78;
    Rectangle kbtn{kx + 76, gy, kw, 30};
    std::string kind_label = T.kind;
    for (int i = 0; i < UNIT_SYMBOLS_N; i++) if (T.kind == UNIT_SYMBOLS[i].key) kind_label = UNIT_SYMBOLS[i].label;
    if (ui.button(kbtn, "", true, "The symbol its counters show on the map. The AI also builds by kind. Click to choose.", tmpl_pick == 200)) { tmpl_pick = 200; tmpl_pick_off = 0; }
    draw_unit_symbol(Rectangle{kbtn.x + 5, kbtn.y + 6, 27, 18}, T.kind, Color{196, 186, 150, 255}, INK);
    ui.text(kbtn.x + 37, kbtn.y + 6, ui.fit(kind_label, 14, kw - 44), 14, pal::TEXT);
    gy += 42;
    const float SUPW = 124, CGAP = 8, FOOTH = 30;
    float cell_h = std::min(84.0f, (B.y + B.height - gy - 24 - FOOTH - 4 * CGAP) / 5);
    float cell_w = (gw - SUPW - 20 - 4 * CGAP) / 5;
    ui.text(gx, gy, fmt("Support (%zu of 5)", T.support.size()), 13.5f, pal::ACCENT, 1);
    ui.text(gx + SUPW + 20, gy, fmt("Line battalions (%zu of 25)", T.line.size()), 13.5f, pal::ACCENT, 1);
    float gy0 = gy + 20;
    DrawLineEx({gx + SUPW + 9, gy0}, {gx + SUPW + 9, gy0 + 5 * cell_h + 4 * CGAP}, 1, TABLE_LINE);
    int removed_line = -1, removed_sup = -1;
    auto cell = [&](Rectangle c, int bid, int code, bool next_free) {
        bool h = ui.hover(c), open = tmpl_pick == code;
        if (bid >= 0) {
            const Battalion& Bt = w.battalions[bid];
            DrawRectangleRec({c.x + 2, c.y + 3, c.width, c.height}, Color{0, 0, 0, 100});
            DrawRectangleRec(c, h ? Color{230, 222, 196, 255} : PAPER);
            bool known = w.battalion_ok(w.player, bid);
            DrawRectangleLinesEx(c, open || !known ? 2 : 1, open ? pal::ACCENT : !known ? pal::BAD : Color{60, 54, 40, 255});
            float sw = std::min(c.width * 0.46f, 44.0f), sh = std::min(sw * 0.66f, c.height - 26);
            draw_unit_symbol(Rectangle{c.x + c.width / 2 - sw / 2, c.y + 5, sw, sh}, Bt.icon, Color{196, 186, 150, 255}, INK);
            float fs = 13;
            std::string nm = ui.fit(Bt.name, fs, c.width - 8);
            if (nm != Bt.name) nm = ui.fit(Bt.short_name, fs, c.width - 8);
            ui.text_center(c.x + c.width / 2, c.y + c.height - 19, nm, fs, INK);
            if (h) {
                ui.over = true;
                std::string tip = Bt.name + (Bt.desc.empty() ? "" : "\n" + Bt.desc) + fmt("\n%.1fk men · width %.0f · organisation %.0f", Bt.mp, Bt.width, Bt.org);
                for (auto& nd : Bt.need) tip += fmt("\nNeeds %.0f %s", nd.count, need_label(nd).c_str());
                if (!known) tip += "\nNot researched yet" + (Bt.tech >= 0 ? ": " + w.techs[Bt.tech].name : std::string()) + ". The division cannot be raised.";
                ui.set_tip(tip + "\nClick to change. Right-click to remove.");
                if (ui.clicked) { tmpl_pick = code; tmpl_pick_off = 0; }
                if (ui.rclicked) { if (code >= 100) removed_sup = code - 100; else removed_line = code; }
            }
        } else {
            DrawRectangleRec(c, h || open ? Color{70, 72, 58, 255} : Color{50, 52, 42, 255});
            dashed_box(c, h || open ? pal::ACCENT : Color{118, 116, 90, 180});
            if (next_free) {
                float m = c.y + c.height / 2;
                DrawRectangleRec({c.x + c.width / 2 - 8, m - 1, 16, 2}, h ? pal::ACCENT : Color{150, 146, 112, 255});
                DrawRectangleRec({c.x + c.width / 2 - 1, m - 8, 2, 16}, h ? pal::ACCENT : Color{150, 146, 112, 255});
            }
            if (h) {
                ui.over = true;
                ui.set_tip(code >= 100 ? "Add a support company" : "Add a battalion");
                if (ui.clicked) { tmpl_pick = code; tmpl_pick_off = 0; }
            }
        }
    };
    for (int i = 0; i < 5; i++)
        cell(Rectangle{gx, gy0 + i * (cell_h + CGAP), SUPW, cell_h}, i < (int)T.support.size() ? T.support[i] : -1, 100 + i, i == (int)T.support.size());
    for (int col = 0; col < 5; col++)
        for (int row = 0; row < 5; row++) {
            int i = col * 5 + row;
            cell(Rectangle{gx + SUPW + 20 + col * (cell_w + CGAP), gy0 + row * (cell_h + CGAP), cell_w, cell_h}, i < (int)T.line.size() ? T.line[i] : -1, i, i == (int)T.line.size());
        }
    if (removed_line >= 0) { T.line.erase(T.line.begin() + removed_line); w.template_stats(T); }
    if (removed_sup >= 0) { T.support.erase(T.support.begin() + removed_sup); w.template_stats(T); }
    ui.text(gx, B.y + B.height - FOOTH + 4, "Click a cell to add or change. Right-click removes.", 13, pal::DIM);

    // ---- right: what it would be with our best designs; while a list is open, what the row under the mouse would make it
    static int preview_bid = -1, preview_code = -1;
    DivTemplate S = T;
    if (tmpl_pick != -1 && tmpl_pick == preview_code) {
        auto& v = tmpl_pick >= 100 ? S.support : S.line;
        int at = tmpl_pick >= 100 ? tmpl_pick - 100 : tmpl_pick;
        if (at < (int)v.size()) v[at] = preview_bid; else v.push_back(preview_bid);
        w.template_stats(S);
    }
    preview_code = -1;
    float st[DS_N], base[DS_N];
    w.template_preview(w.player, S, st);
    w.template_preview(w.player, tmpl_sel >= 0 ? N.tmpls[tmpl_sel] : T, base);
    ui.panel(R);
    x = R.x + 12, y = R.y + 10, wdt = R.width - 24;
    ui.text(x, y, "With our best equipment", 15.5f, pal::ACCENT, 1);
    y += 22;
    if (tmpl_sel >= 0) { ui.text(x, y, "Changes against the saved template", 12.5f, pal::DIM); y += 18; }
    ui.separator(x, y, wdt);
    y += 5;
    const char* LBL[] = {"Soft attack", "Hard attack", "Air attack", "Defence", "Breakthrough", "Armour", "Piercing", "Hardness", "Organisation",
                         "Speed (km/h)", "Width", "Recon", "Manpower (k)"};
    float foot = R.y + R.height - 200;
    for (int k = 0; k < DS_FILL && y + 23 <= foot; k++) {
        bool pct = k == DS_HARDNESS;
        float v = st[k], b = base[k];
        std::string val = pct ? fmt("%.0f%%", v * 100) : fmt("%.1f", v), diff = pct ? fmt("%+.0f%%", (v - b) * 100) : fmt("%+.1f", v - b);
        bool lower = k == DS_WIDTH || k == DS_MP;
        static const float SCALE[DS_FILL] = {60, 30, 10, 80, 40, 200, 300, 1, 60, 30, 40, 3, 25};     // a strong division of its day
        stat_row(ui, x, y, wdt, LBL[k], val, v, b, std::max(SCALE[k], v), lower, true, diff);
        y += 23;
    }
    y = foot;
    ui.separator(x, y, wdt);
    y += 5;
    ui.text(x, y, "Equipment for one division", 14, pal::ACCENT, 1);
    y += 19;
    ui.scissor(Rectangle{R.x, y, R.width, 88});
    float ey0 = y;
    for (auto& nd : S.needs) {
        if (y > ey0 + 88) break;
        float have = w.stock_fitting(w.player, nd);
        ui.text(x, y, ui.fit(need_label(nd), 13, wdt - 88), 13, pal::TEXT);
        ui.text_right(x + wdt, y, fmt("%.0f / %s", nd.count, kfmt(have).c_str()), 13, have >= nd.count ? pal::GOOD : pal::BAD);
        y += 18;
    }
    EndScissorMode();
    y = ey0 + 90;
    bool can_raise = w.template_ok(w.player, S);
    ui.text(x, y, fmt("Training %.0f days · ", S.train) + std::string(can_raise ? "can be raised" : "needs research"), 13, can_raise ? pal::DIM : pal::BAD);
    y += 21;
    bool ok = !T.line.empty() && !T.name.empty();
    float half = (wdt - 6) / 2;
    float sw_ = tmpl_sel >= 0 ? half : wdt;
    if (ui.button(Rectangle{x, y, sw_, 32}, tmpl_sel >= 0 ? "Save changes" : "Save as new", ok,
                  tmpl_sel >= 0 ? "Divisions using it keep the equipment that still fits and send the rest to the stockpile" : "", true)) {
        w.template_stats(T);
        if (tmpl_sel >= 0) {
            N.tmpls[tmpl_sel] = T;
            for (auto& d : w.divs)
                if (d.alive && d.nation == w.player && d.tmpl == tmpl_sel) w.retemplate(d);
        } else {
            N.tmpls.push_back(T);
            tmpl_sel = (int)N.tmpls.size() - 1;
        }
        say("Template saved: " + T.name);
    }
    if (tmpl_sel >= 0 && ui.button(Rectangle{x + half + 6, y, half, 32}, N.tmpls[tmpl_sel].obsolete ? "Restore" : "Retire", true,
                                   "A retired template is hidden from the recruitment list"))
        N.tmpls[tmpl_sel].obsolete = !N.tmpls[tmpl_sel].obsolete;

    // ---- the open list, last, over everything
    ui.mouse = real_mouse;
    ui.clicked = real_click;
    ui.rclicked = real_rclick;
    ui.wheel = real_wheel;
    if (tmpl_pick == -1) return;
    // the click that opened the list this frame must not also count as a click on it or outside it
    if (!list_was_open) { real_click = false; ui.clicked = false; }
    if (tmpl_pick == 200) {     // the map symbols, drawn, by group
        const float CW = 118, CH = 50, PW0 = CW * 5 + 28;
        const char* groups[] = {"Infantry", "Special", "Mobile", "Fire support", "Support"};
        float need = 42;
        for (const char* g : groups) {
            int n = 0;
            for (int i = 0; i < UNIT_SYMBOLS_N; i++) n += std::string(UNIT_SYMBOLS[i].group) == g;
            need += 20 + ((n + 4) / 5) * (CH + 4);
        }
        Rectangle P{B.x + B.width / 2 - PW0 / 2, B.y + 12, PW0, std::min(B.height - 20, need + 8)};
        ui.panel(P, Color{26, 26, 22, 252});
        ui.text(P.x + 14, P.y + 10, "Map symbol", 18, pal::ACCENT, 1);
        bool shut = ui.button(Rectangle{P.x + PW0 - 36, P.y + 8, 26, 26}, "x");
        Rectangle view{P.x, P.y + 40, PW0, P.height - 44};
        tmpl_pick_off = ui.scroll(view, tmpl_pick_off, need - 42 + 4);
        ui.scissor(view);
        float yy = view.y - tmpl_pick_off;
        for (const char* g : groups) {
            ui.text(P.x + 14, yy + 1, g, 13.5f, pal::DIM, 1);
            yy += 20;
            int col = 0;
            for (int i = 0; i < UNIT_SYMBOLS_N; i++) {
                if (std::string(UNIT_SYMBOLS[i].group) != g) continue;
                Rectangle c{P.x + 14 + col * CW, yy, CW - 4, CH};
                bool hov = ui.hover(c) && ui.hover(view), cur = T.kind == UNIT_SYMBOLS[i].key;
                DrawRectangleRec(c, cur ? Color{62, 54, 36, 255} : hov ? pal::ROW_HI : pal::ROW);
                if (cur) DrawRectangleLinesEx(c, 1, pal::ACCENT);
                draw_unit_symbol(Rectangle{c.x + c.width / 2 - 18, c.y + 4, 36, 23}, UNIT_SYMBOLS[i].key, Color{196, 186, 150, 255}, INK);
                ui.text_center(c.x + c.width / 2, c.y + 30, ui.fit(UNIT_SYMBOLS[i].label, 13, c.width - 6), 13, pal::TEXT);
                if (hov && real_click) { T.kind = UNIT_SYMBOLS[i].key; shut = true; }
                if (++col == 5) { col = 0; yy += CH + 4; }
            }
            if (col) yy += CH + 4;
        }
        EndScissorMode();
        if (shut || (real_click && !ui.hover(P)) || real_rclick || IsKeyPressed(KEY_ESCAPE)) tmpl_pick = -1;
        ui.clicked = false;
        ui.over = true;
        return;
    }
    bool support = tmpl_pick >= 100;
    int at = support ? tmpl_pick - 100 : tmpl_pick;
    auto& V = support ? T.support : T.line;
    bool filled = at < (int)V.size();
    struct Row { int id; bool ok; };
    std::vector<Row> rows;
    for (size_t b = 0; b < w.battalions.size(); b++) {
        if (w.battalions[b].support != support) continue;
        bool researched = w.battalion_ok(w.player, (int)b);
        // only what has been researched is offered (the battalion already in the cell stays listed)
        if (!researched && !(filled && V[at] == (int)b)) continue;
        bool dup = support && std::find(V.begin(), V.end(), (int)b) != V.end() && !(filled && V[at] == (int)b);
        rows.push_back({(int)b, researched && !dup});
    }
    const float RH = 46, PW = 400;
    float ph = std::min(B.height - 30, 44 + (rows.size() + (filled ? 1 : 0)) * RH + 8);
    Rectangle P{B.x + B.width / 2 - PW / 2, B.y + 20, PW, ph};
    ui.panel(P, Color{26, 26, 22, 252});
    ui.text(P.x + 14, P.y + 10, support ? "Support company" : "Line battalion", 18, pal::ACCENT, 1);
    bool close = ui.button(Rectangle{P.x + PW - 36, P.y + 8, 26, 26}, "x");
    Rectangle view{P.x, P.y + 42, PW, P.height - 48};
    tmpl_pick_off = ui.scroll(view, tmpl_pick_off, (rows.size() + (filled ? 1 : 0)) * RH + 4);
    ui.scissor(view);
    float ry = view.y - tmpl_pick_off;
    if (filled) {
        Rectangle row{P.x + 8, ry, PW - 16, RH - 3};
        bool hov = ui.hover(row) && ui.hover(view);
        DrawRectangleRec(row, hov ? pal::ROW_HI : pal::ROW);
        ui.text(row.x + 10, row.y + 12, "Remove it", 15, pal::BAD, 1);
        if (hov && real_click) { V.erase(V.begin() + at); w.template_stats(T); close = true; }
        ry += RH;
    }
    for (auto& rw : rows) {
        const Battalion& Bt = w.battalions[rw.id];
        Rectangle row{P.x + 8, ry, PW - 16, RH - 3};
        bool hov = ui.hover(row) && ui.hover(view);
        bool cur = filled && V[at] == rw.id;
        DrawRectangleRec(row, cur ? Color{62, 54, 36, 255} : hov && rw.ok ? pal::ROW_HI : pal::ROW);
        if (cur) DrawRectangleLinesEx(row, 1, pal::ACCENT);
        draw_unit_symbol(Rectangle{row.x + 6, row.y + 8, 40, 26}, Bt.icon, rw.ok ? Color{196, 186, 150, 255} : Color{110, 106, 92, 255}, INK);
        float tx = row.x + 54, tw = row.width - 60;
        ui.text(tx, row.y + 3, ui.fit(Bt.name, 15, tw, 1), 15, rw.ok ? pal::TEXT : pal::DIM, 1);
        std::string sub = fmt("%.1fk men · width %.0f · organisation %.0f", Bt.mp, Bt.width, Bt.org);
        Color sc = pal::DIM;
        if (!w.battalion_ok(w.player, rw.id)) { sub = Bt.tech >= 0 ? "Needs research: " + w.techs[Bt.tech].name : "Not available"; sc = Color{170, 120, 100, 255}; }
        else if (!rw.ok) { sub = "Already in this division"; sc = Color{170, 120, 100, 255}; }
        ui.text(tx, row.y + 23, ui.fit(sub, 12.5f, tw), 12.5f, sc);
        if (hov && rw.ok) {
            preview_code = tmpl_pick;
            preview_bid = rw.id;
            if (real_click) {
                if (filled) V[at] = rw.id; else V.push_back(rw.id);
                w.template_stats(T);
                close = true;
            }
        }
        ry += RH;
    }
    EndScissorMode();
    if (close || (real_click && !ui.hover(P)) || real_rclick || IsKeyPressed(KEY_ESCAPE)) tmpl_pick = -1;
    ui.clicked = false;
    ui.over = true;
}
