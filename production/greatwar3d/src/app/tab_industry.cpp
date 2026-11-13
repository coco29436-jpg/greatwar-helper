// The Industry tab: factories, the build tool and the construction queue. Buildings can also be ordered from the province panel;
// production lines are on the Production screen.
#include <algorithm>
#include <map>

#include "../ui/icons.h"
#include "app.h"

const char* BUILD_ICON[B_N] = {"factory", "cannon", "rail", "shield", "anchor", "plane", "star", "balloon", "ship", "globe"};
const char* BUILD_SHORT[B_N] = {"Civilian", "Military", "Railway", "Fort", "Coastal", "Airfield", "Air base", "Airship", "Seaplane", "Airport"};
const char* BUILD_ABOUT[B_N] = {
    "Builds, makes consumer goods and trades.",
    "Makes equipment on the production lines.",
    "Faster marching and building, more supply, higher upkeep.",
    "Defence against attacks by land, in this province.",
    "Guns for the shore. Troops landing here arrive disordered and fight at a penalty; every level also weakens enemy blockades a little.",
    "A grass strip with huts: bases fighters. Each level holds 2 wings.",
    "Concrete runways and hangars: bases bombers and fighters. Each level holds 2 wings.",
    "A great shed and a mooring mast: bases airships. Each level holds 2 wings.",
    "A ramp on the open sea: bases naval bombers. Each level holds 2 wings.",
    "Passenger and mail flights: a little more tax income and influence. Bases no wings."};

void App::tab_industry(Rectangle r) {
    Nation& N = me();
    float& off = panel_scroll[TAB_INDUSTRY];
    float x = r.x + 14, wdt = r.width - 28;
    float y = r.y + 10 - off;
    float s0 = y;
    y = ui.section(x, y, wdt, "Factories");
    auto row = [&](const std::string& k, const std::string& v, Color c = pal::TEXT) {
        ui.text(x, y, k, 16, pal::DIM);
        ui.text_right(x + wdt, y, v, 16, c);
        y += 21;
    };
    row("Civilian factories", fmt("%.1f", N.cf_total));
    row(fmt("  consumer goods (%s)", ECON_LAW_NAMES[N.law_econ]), fmt("-%.1f", N.consumer));
    row("  trade", fmt("%+.0f", N.trade_cf));
    row("  free for construction", fmt("%.1f", N.cf_free), pal::GOOD);
    row("Military factories", fmt("%.1f", N.mf_total));
    ui.section_end(x, s0, y, wdt);
    y += 14;
    s0 = y;
    y = ui.section(x, y, wdt, "Build", build_tool >= 0 ? "Esc to stop" : "pick, then click the map");
    {
        const int COLS = 5;
        float bw = (wdt - 3 * (COLS - 1)) / COLS;
        for (int b = 0; b < B_N; b++) {
            Rectangle br{x + (b % COLS) * (bw + 3), y + (b / COLS) * 49, bw, 46};
            std::string tip = fmt("%s: %s\nClick here, then click your provinces.\nClick: build one. Shift + click: fill every free slot or level.\n"
                                  "Right-click: cancel one queued here, or demolish one built. Shift + right-click: all of them.", BUILD_NAMES[b], BUILD_ABOUT[b]);
            if (ui.button(br, "", true, tip, build_tool == b)) build_tool = build_tool == b ? -1 : b;
            draw_icon(BUILD_ICON[b], br.x + bw / 2, br.y + 16, 20, build_tool == b ? pal::ACCENT : pal::TEXT);
            ui.text_center(br.x + bw / 2, br.y + 28, BUILD_SHORT[b], 13, build_tool == b ? pal::ACCENT : pal::DIM);
        }
        y += 49;
        y += 52;
        if (build_tool >= 0) {
            ui.text(x, y, "Green dots: room to build. Orange: queued.", 14, pal::GOOD);
            y += 20;
            ui.text(x, y, "Shift fills a province. Right-click removes.", 14, pal::DIM);
            y += 22;
        }
    }
    ui.section_end(x, s0, y, wdt);
    y += 14;
    s0 = y;
    y = ui.section(x, y, wdt, "Construction queue", fmt("%zu project%s", N.builds.size(), N.builds.size() == 1 ? "" : "s"));
    if (N.builds.empty()) { ui.wrap(Rectangle{x, y, wdt, 60}, "Empty. Pick a building above and click your provinces on the map.", 15, pal::DIM); y += 42; }
    float avail = N.cf_free;
    for (size_t k = 0; k < N.builds.size(); k++) {
        Build& b = N.builds[k];
        Province& p = w.provs[b.prov];
        float cost = w.build_cost_at(w.player, b.prov, b.type);
        float use = std::min(avail, 10.0f);
        avail -= use;
        ui.text(x, y, fmt("%s · %s", BUILD_NAMES[b.type], p.name.c_str()), 15, use > 0 ? pal::TEXT : pal::DIM);
        ui.bar(Rectangle{x, y + 19, wdt - 70, 8}, b.prog / cost, use > 0 ? pal::ACCENT : Color{100, 90, 70, 255});
        float rate = use * 5 * (1 + N.mods[M_CONSTRUCT]) * (0.7f + 0.1f * p.infra);
        ui.tip_if(Rectangle{x, y, wdt - 70, 28}, use > 0 ? fmt("%.0f factories working: about %.0f days left.", use, (cost - b.prog) / std::max(1.0f, rate)) : "Waiting for free civilian factories.");
        if (k > 0 && ui.button(Rectangle{x + wdt - 62, y + 2, 28, 24}, "^", true, "Move up")) std::swap(N.builds[k], N.builds[k - 1]);
        if (ui.button(Rectangle{x + wdt - 30, y + 2, 28, 24}, "x", true, "Cancel")) { N.builds.erase(N.builds.begin() + k); break; }
        y += 34;
    }
    ui.section_end(x, s0, y, wdt);
    y += 14;
    s0 = y;
    y = ui.section(x, y, wdt, "Military production");
    int used = 0;
    for (auto& l : N.lines) used += l.factories;
    int mf = (int)floorf(N.mf_total + 0.001f);
    ui.text(x, y, fmt("%d of %d military factories on %zu production lines", used, mf, N.lines.size()), 15, used < mf ? pal::WARN : pal::DIM);
    y += 22;
    ui.text(x, y, fmt("Locomotives %.0f%% of what the railways need · farm machinery %.0f%%", std::min(1.5f, N.rail_ratio) * 100, N.farm_mech * 100), 14, pal::DIM);
    ui.tip_if(Rectangle{x, y, wdt, 20}, "Locomotives carry supply and troops by rail; short of them, supply reaches less far.\nTractors and harvesters raise grain and livestock output.");
    y += 24;
    for (auto& l : N.lines) {      // the lines at a glance
        const Design& d = w.designs[l.design];
        ui.text(x + 8, y, d.name, 14);
        ui.text_right(x + wdt - 60, y, fmt("%d factor%s", l.factories, l.factories == 1 ? "y" : "ies"), 14, pal::DIM);
        ui.text_right(x + wdt, y, fmt("%.0f%%", l.eff * 100), 14, pal::DIM);
        y += 19;
    }
    y += 6;
    if (ui.button(Rectangle{x, y, wdt, 30}, "Production lines, designs and stockpile...", true, "The Production screen (F6)")) tab = TAB_PRODUCTION;
    y += 36;
    ui.section_end(x, s0, y, wdt);
    y += 10;
    float content = y + off - r.y;
    off = ui.scroll(r, off, content + 10);
}

