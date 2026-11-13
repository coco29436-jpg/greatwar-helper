#include "gw_sim.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <vector>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>

#include "world.h"

using godot::Array;
using godot::ClassDB;
using godot::D_METHOD;
using godot::Dictionary;
using godot::PackedByteArray;
using godot::String;

static std::string to_std(const String& s) { return std::string(s.utf8().get_data()); }
static String to_godot(const std::string& s) { return String::utf8(s.c_str()); }

GreatWarSim::GreatWarSim() = default;
GreatWarSim::~GreatWarSim() = default;

void GreatWarSim::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_world", "data_dir"), &GreatWarSim::load_world);
    ClassDB::bind_method(D_METHOD("error"), &GreatWarSim::error);
    ClassDB::bind_method(D_METHOD("start_as", "tag"), &GreatWarSim::start_as);
    ClassDB::bind_method(D_METHOD("advance_days", "days"), &GreatWarSim::advance_days);
    ClassDB::bind_method(D_METHOD("date"), &GreatWarSim::date);
    ClassDB::bind_method(D_METHOD("hour"), &GreatWarSim::hour);
    ClassDB::bind_method(D_METHOD("set_hour", "hour"), &GreatWarSim::set_hour);
    ClassDB::bind_method(D_METHOD("nation"), &GreatWarSim::nation);
    ClassDB::bind_method(D_METHOD("lines"), &GreatWarSim::lines);
    ClassDB::bind_method(D_METHOD("change_factories", "line", "delta"), &GreatWarSim::change_factories);
    ClassDB::bind_method(D_METHOD("designs"), &GreatWarSim::designs);
    ClassDB::bind_method(D_METHOD("open_line", "design"), &GreatWarSim::open_line);
    ClassDB::bind_method(D_METHOD("close_line", "line"), &GreatWarSim::close_line);
    ClassDB::bind_method(D_METHOD("pending"), &GreatWarSim::pending);
    ClassDB::bind_method(D_METHOD("answer", "option"), &GreatWarSim::answer);
    ClassDB::bind_method(D_METHOD("reports"), &GreatWarSim::reports);
    ClassDB::bind_method(D_METHOD("news", "count"), &GreatWarSim::news);
    ClassDB::bind_method(D_METHOD("relations"), &GreatWarSim::relations);
    ClassDB::bind_method(D_METHOD("espionage"), &GreatWarSim::espionage);
    ClassDB::bind_method(D_METHOD("budget"), &GreatWarSim::budget);
    ClassDB::bind_method(D_METHOD("politics"), &GreatWarSim::politics);
    ClassDB::bind_method(D_METHOD("society"), &GreatWarSim::society);
    ClassDB::bind_method(D_METHOD("focus"), &GreatWarSim::focus);
    ClassDB::bind_method(D_METHOD("research"), &GreatWarSim::research);
    ClassDB::bind_method(D_METHOD("forces"), &GreatWarSim::forces);
    ClassDB::bind_method(D_METHOD("capital"), &GreatWarSim::capital);
    ClassDB::bind_method(D_METHOD("nations"), &GreatWarSim::nations);
    ClassDB::bind_method(D_METHOD("laws"), &GreatWarSim::laws);
    ClassDB::bind_method(D_METHOD("set_law", "group", "option"), &GreatWarSim::set_law);
    ClassDB::bind_method(D_METHOD("ministers", "post"), &GreatWarSim::ministers);
    ClassDB::bind_method(D_METHOD("appoint", "minister"), &GreatWarSim::appoint);
    ClassDB::bind_method(D_METHOD("decisions"), &GreatWarSim::decisions);
    ClassDB::bind_method(D_METHOD("take_decision", "id"), &GreatWarSim::take_decision);
    ClassDB::bind_method(D_METHOD("programmes"), &GreatWarSim::programmes);
    ClassDB::bind_method(D_METHOD("start_programme", "id"), &GreatWarSim::start_programme);
    ClassDB::bind_method(D_METHOD("campaign", "ideology"), &GreatWarSim::campaign);
    ClassDB::bind_method(D_METHOD("candidates"), &GreatWarSim::candidates);
    ClassDB::bind_method(D_METHOD("hire_agent", "index"), &GreatWarSim::hire_agent);
    ClassDB::bind_method(D_METHOD("post_agent", "tag", "by"), &GreatWarSim::post_agent);
    ClassDB::bind_method(D_METHOD("fleet_mission", "fleet", "mission"), &GreatWarSim::fleet_mission);
    ClassDB::bind_method(D_METHOD("builds", "province"), &GreatWarSim::builds);
    ClassDB::bind_method(D_METHOD("build", "province", "type"), &GreatWarSim::build);
    ClassDB::bind_method(D_METHOD("build_queue"), &GreatWarSim::build_queue);
    ClassDB::bind_method(D_METHOD("cancel_build", "index"), &GreatWarSim::cancel_build);
    ClassDB::bind_method(D_METHOD("raise_build", "index"), &GreatWarSim::raise_build);
    ClassDB::bind_method(D_METHOD("market"), &GreatWarSim::market);
    ClassDB::bind_method(D_METHOD("buy", "tag", "resource", "amount"), &GreatWarSim::buy);
    ClassDB::bind_method(D_METHOD("cancel_deal", "index"), &GreatWarSim::cancel_deal);
    ClassDB::bind_method(D_METHOD("air_designs"), &GreatWarSim::air_designs);
    ClassDB::bind_method(D_METHOD("form_wing", "design"), &GreatWarSim::form_wing);
    ClassDB::bind_method(D_METHOD("start_focus", "id"), &GreatWarSim::start_focus);
    ClassDB::bind_method(D_METHOD("techs_open"), &GreatWarSim::techs_open);
    ClassDB::bind_method(D_METHOD("start_research", "slot", "tech", "experimental"), &GreatWarSim::start_research);
    ClassDB::bind_method(D_METHOD("shift_budget", "category", "by"), &GreatWarSim::shift_budget);
    ClassDB::bind_method(D_METHOD("diplomacy", "action", "tag"), &GreatWarSim::diplomacy);
    ClassDB::bind_method(D_METHOD("templates"), &GreatWarSim::templates);
    ClassDB::bind_method(D_METHOD("recruit", "tmpl"), &GreatWarSim::recruit);
    ClassDB::bind_method(D_METHOD("province_ids", "step"), &GreatWarSim::province_ids);
    ClassDB::bind_method(D_METHOD("province_palette"), &GreatWarSim::province_palette);
    ClassDB::bind_method(D_METHOD("province", "id"), &GreatWarSim::province);
    ClassDB::bind_method(D_METHOD("places"), &GreatWarSim::places);
    ClassDB::bind_method(D_METHOD("map_width"), &GreatWarSim::map_width);
    ClassDB::bind_method(D_METHOD("map_height"), &GreatWarSim::map_height);
    ClassDB::bind_method(D_METHOD("set_map_mode", "mode"), &GreatWarSim::set_map_mode);
    ClassDB::bind_method(D_METHOD("map_units"), &GreatWarSim::map_units);
    ClassDB::bind_method(D_METHOD("battles"), &GreatWarSim::battles);
    ClassDB::bind_method(D_METHOD("declare_war_now", "tag"), &GreatWarSim::declare_war_now);
    ClassDB::bind_method(D_METHOD("move_divisions", "ids", "province"), &GreatWarSim::move_divisions);
    ClassDB::bind_method(D_METHOD("sail_fleet", "fleet", "province"), &GreatWarSim::sail_fleet);
    ClassDB::bind_method(D_METHOD("country", "tag"), &GreatWarSim::country);
    ClassDB::bind_method(D_METHOD("save_game", "path"), &GreatWarSim::save_game);
    ClassDB::bind_method(D_METHOD("load_game", "path"), &GreatWarSim::load_game);
}

bool GreatWarSim::load_world(const String& data_dir) {
    w = std::make_unique<World>();
    if (!w->load(to_std(data_dir))) {
        last_error = to_godot(w->err);
        w.reset();
        return false;
    }
    return true;
}

bool GreatWarSim::start_as(const String& tag) {
    if (!w) return false;
    int n = w->nation_of(to_std(tag));
    if (n < 0) return false;
    w->player = n;
    w->choices.clear();
    w->completed.clear();
    return true;
}

int GreatWarSim::advance_days(int days) {
    if (!w) return 0;
    int done = 0;
    // as in the game, time stands still while an event waits for its answer
    while (done < days && w->choices.empty()) {
        w->tick_day();
        done++;
    }
    return done;
}

Dictionary GreatWarSim::pending() const {
    Dictionary out;
    if (!w || w->choices.empty()) return out;
    const Choice& c = w->choices.front();
    Array labels, tips;
    for (size_t i = 0; i < c.labels.size(); i++) {
        labels.push_back(to_godot(c.labels[i]));
        tips.push_back(i < c.tips.size() ? to_godot(c.tips[i]) : String());
    }
    out["title"] = to_godot(c.title);
    out["text"] = to_godot(c.text);
    out["pic"] = to_godot(c.pic);
    out["labels"] = labels;
    out["tips"] = tips;
    out["waiting"] = (int)w->choices.size();
    return out;
}

// as App::draw_choice
bool GreatWarSim::answer(int option) {
    if (!w || w->choices.empty()) return false;
    Choice& c = w->choices.front();
    if (option < 0 || option >= (int)c.actions.size()) return false;
    auto action = c.actions[option];
    w->choices.pop_front();
    if (action) action();
    w->map_dirty = true;
    return true;
}

String GreatWarSim::date() const { return w ? to_godot(w->date()) : String(); }

// the time of day, 0 to 24: the simulation only keeps it (and saves it); the game's clock moves it
float GreatWarSim::hour() const { return w ? w->hour : 0.0f; }
void GreatWarSim::set_hour(float hour) { if (w) w->hour = std::clamp(hour, 0.0f, 23.999f); }

Dictionary GreatWarSim::nation() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    const Nation& N = w->nations[w->player];
    int used = 0;
    for (auto& l : N.lines) used += l.factories;
    out["tag"] = to_godot(N.tag);
    out["name"] = to_godot(N.display_name());
    out["money"] = N.money;
    out["factories_used"] = used;
    out["factories"] = (int)std::floor(N.mf_total + 0.001f);
    out["factories_free"] = std::max(0, (int)std::floor(N.mf_total + 0.001f) - used);
    return out;
}

Array GreatWarSim::lines() const {
    Array out;
    if (!w || w->player < 0) return out;
    const Nation& N = w->nations[w->player];
    for (size_t k = 0; k < N.lines.size(); k++) {
        const ProdLine& l = N.lines[k];
        const Design& d = w->designs[l.design];
        const Chassis& c = w->chassis[d.chassis];
        // as "daily" in the Production screen
        float bonus = 1 + N.mods[M_FACTORY] + (c.cat == CAT_SHIP ? N.mods[M_NAVAL] : 0) + (c.cat == CAT_AIR ? N.mods[M_AIR] : 0);
        float rr = 1;
        for (int r = 0; r < 3; r++)
            if (d.st.res[r] > 0) rr = std::min(rr, N.res_ratio[r]);
        auto st = N.stock.find(d.id);
        Dictionary e;
        e["index"] = (int)k;
        e["design"] = d.id;
        e["name"] = to_godot(d.name);
        e["chassis"] = to_godot(c.id);
        e["kind"] = to_godot(w->design_kind(d));
        e["category"] = String(CAT_LABELS[c.cat]);
        e["factories"] = l.factories;
        e["efficiency"] = l.eff;
        e["daily"] = l.factories * l.eff * bonus * (0.25f + 0.75f * rr) / d.st.v[S_COST];
        e["stock"] = st == N.stock.end() ? 0.0f : st->second;
        out.push_back(e);
    }
    return out;
}

bool GreatWarSim::change_factories(int line, int delta) {
    if (!w || w->player < 0) return false;
    Nation& N = w->nations[w->player];
    if (line < 0 || line >= (int)N.lines.size()) return false;
    int used = 0;
    for (auto& l : N.lines) used += l.factories;
    int free_mf = std::max(0, (int)std::floor(N.mf_total + 0.001f) - used);
    ProdLine& l = N.lines[line];
    if (delta > 0) delta = std::min(delta, free_mf);
    int next = std::max(1, l.factories + delta);        // closing a line needs its own control
    if (next == l.factories) return false;
    l.factories = next;
    return true;
}

Array GreatWarSim::designs() const {
    Array out;
    if (!w || w->player < 0) return out;
    const Nation& N = w->nations[w->player];
    std::vector<const Design*> list;
    for (auto& d : w->designs)
        if (d.nation == w->player && !d.obsolete) list.push_back(&d);
    // by category, then role, the best first: the order of the Production screen's lists
    std::stable_sort(list.begin(), list.end(), [&](const Design* a, const Design* b) {
        int ca = w->chassis[a->chassis].cat, cb = w->chassis[b->chassis].cat;
        if (ca != cb) return ca < cb;
        if (a->role != b->role) return a->role < b->role;
        return a->score > b->score;
    });
    for (const Design* dp : list) {
        const Design& d = *dp;
        const Chassis& c = w->chassis[d.chassis];
        bool on_line = false;
        for (auto& l : N.lines) on_line |= l.design == d.id;
        auto st = N.stock.find(d.id);
        Dictionary e;
        e["design"] = d.id;
        e["name"] = to_godot(d.name);
        e["chassis"] = to_godot(c.id);
        e["kind"] = to_godot(w->design_kind(d));
        e["category"] = String(CAT_LABELS[c.cat]);
        e["year"] = d.year;
        e["cost"] = d.st.v[S_COST];
        e["stock"] = st == N.stock.end() ? 0.0f : st->second;
        e["on_line"] = on_line;
        out.push_back(e);
    }
    return out;
}

// as a click in the Production screen's list of designs
bool GreatWarSim::open_line(int design) {
    if (!w || w->player < 0) return false;
    if (design < 0 || design >= (int)w->designs.size() || w->designs[design].nation != w->player) return false;
    Nation& N = w->nations[w->player];
    int used = 0;
    for (auto& l : N.lines) used += l.factories;
    if ((int)std::floor(N.mf_total + 0.001f) - used <= 0) return false;
    for (auto& l : N.lines)
        if (l.design == design) { l.factories++; return true; }
    N.lines.push_back({design, 1});
    return true;
}

bool GreatWarSim::close_line(int line) {
    if (!w || w->player < 0) return false;
    Nation& N = w->nations[w->player];
    if (line < 0 || line >= (int)N.lines.size()) return false;
    N.lines.erase(N.lines.begin() + line);
    return true;
}


// ---- the papers on the desk

static constexpr int PALETTE_W = 4096;      // as PAL_W in the old map: the most provinces a palette row holds

static String capital_case(const char* s) {
    std::string t = s;
    if (!t.empty()) t[0] = (char)std::toupper((unsigned char)t[0]);
    return to_godot(t);
}

Array GreatWarSim::reports() {
    Array out;
    if (!w) return out;
    for (const Completed& c : w->completed) {
        Dictionary e;
        e["focus"] = c.focus;
        e["name"] = to_godot(c.focus ? w->focuses[c.id].name : w->techs[c.id].name);
        e["text"] = to_godot(c.focus ? w->focuses[c.id].desc : w->techs[c.id].desc);
        e["date"] = to_godot(date_string(c.day));
        out.push_back(e);
    }
    w->completed.clear();
    return out;
}

Array GreatWarSim::news(int count) const {
    Array out;
    if (!w) return out;
    for (int i = (int)w->news.size() - 1; i >= 0 && (int)out.size() < count; i--) {
        const NewsItem& n = w->news[i];
        Dictionary e;
        e["date"] = to_godot(date_string(n.day));
        e["days_ago"] = w->day - n.day;
        e["text"] = to_godot(n.text);
        e["ours"] = n.nation == w->player;
        out.push_back(e);
    }
    return out;
}

Array GreatWarSim::relations() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    std::vector<int> list;
    for (int i = 0; i < (int)w->nations.size(); i++) {
        const Nation& T = w->nations[i];
        if (i == me || !T.exists) continue;
        bool tied = w->war[me][i] || w->allied(me, i) || N.ententes.count(i) || N.guarantees.count(i) || T.guarantees.count(me);
        if (T.major || tied || std::fabs(w->relation(me, i)) >= 40) list.push_back(i);
    }
    // enemies first, then the great powers, then by how much they matter to us
    auto rank = [&](int i) { return (w->war[me][i] ? 0 : 1000) + (w->nations[i].major ? 0 : 500) - (int)std::fabs(w->relation(me, i)); };
    std::stable_sort(list.begin(), list.end(), [&](int a, int b) { return rank(a) < rank(b); });
    for (int i : list) {
        const Nation& T = w->nations[i];
        Dictionary e;
        e["tag"] = to_godot(T.tag);
        e["name"] = to_godot(T.display_name());
        e["relation"] = w->relation(me, i);
        e["war"] = (bool)w->war[me][i];
        e["allied"] = w->allied(me, i);
        e["entente"] = N.ententes.count(i) > 0;
        e["guaranteed"] = N.guarantees.count(i) > 0;
        e["guarantees_us"] = T.guarantees.count(me) > 0;
        auto ct = N.courting.find(i);
        e["improve_cost"] = w->improve_cost(me, i);                 // what the next approach costs in influence
        e["improve_due"] = ct == N.courting.end() ? 0.0f : ct->second.due;      // goodwill bought and still on its way
        e["major"] = T.major;
        e["faction"] = T.faction >= 0 ? to_godot(w->factions[T.faction].name) : String();
        e["government"] = String(IDEO_TITLES[T.ideology]);
        e["colour"] = godot::Color(T.col[0] / 255.0f, T.col[1] / 255.0f, T.col[2] / 255.0f);
        out.push_back(e);
    }
    return out;
}

Dictionary GreatWarSim::espionage() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    out["agency"] = to_godot(N.agency_name());
    out["waiting"] = N.agents;
    out["agents"] = w->agents_total(me);
    out["max_agents"] = w->max_agents(me);
    out["influence"] = N.influence;
    Array agents;
    for (const Agent& a : N.agent_list) {
        Dictionary e;
        e["name"] = to_godot(a.name);
        e["post"] = a.post < 0 ? String("at home") : a.post == me ? String("hunting spies") : to_godot(w->nations[a.post].display_name());
        e["infiltration"] = a.infiltration;
        e["counter"] = a.counter;
        e["cover"] = a.cover;
        e["days"] = a.days;
        agents.push_back(e);
    }
    out["list"] = agents;
    Array nets;
    for (int i = 0; i < (int)w->nations.size(); i++) {
        const Nation& T = w->nations[i];
        if (i == me || !T.exists) continue;
        int spies = i < (int)N.spies.size() ? N.spies[i] : 0;
        float know = w->intel_on(me, i);
        if (!T.major && spies == 0 && know < 5) continue;
        Dictionary e;
        e["tag"] = to_godot(T.tag);
        e["name"] = to_godot(T.display_name());
        e["spies"] = spies;
        e["intel"] = know;
        e["knows"] = String(know >= INTEL_PLANS ? "their plans" : know >= INTEL_TECH ? "their research" : know >= INTEL_MIL ? "their forces"
                            : know >= INTEL_ECON ? "their economy" : "almost nothing");
        nets.push_back(e);
    }
    out["networks"] = nets;
    return out;
}

Dictionary GreatWarSim::budget() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    const Nation& N = w->nations[w->player];
    out["money"] = N.money;
    out["income"] = N.income;
    out["upkeep"] = N.upkeep;
    out["civil"] = N.cf_total;
    out["civil_free"] = N.cf_free;
    out["consumer"] = N.consumer;
    out["trade"] = N.trade_cf;
    out["arms"] = N.mf_total;
    out["building"] = (int)N.builds.size();
    Array shares;
    for (int i = 0; i < BUD_N; i++) {
        Dictionary e;
        e["name"] = String(BUDGET_NAMES[i]);
        e["share"] = N.budget[i];
        shares.push_back(e);
    }
    out["shares"] = shares;
    Array res;
    for (int i = 0; i < RES_N; i++) {
        Dictionary e;
        e["name"] = capital_case(RES_NAMES[i]);
        e["made"] = N.res_prod[i];
        e["needed"] = N.res_need[i];
        e["bought"] = N.res_in[i];
        e["sold"] = N.res_out[i];
        e["ratio"] = N.res_ratio[i];
        res.push_back(e);
    }
    out["resources"] = res;
    Array deals;
    for (const TradeDeal& d : w->deals) {
        if (d.buyer != w->player && d.seller != w->player) continue;
        Dictionary e;
        e["index"] = (int)(&d - w->deals.data());
        e["buying"] = d.buyer == w->player;
        e["with"] = to_godot(w->nations[d.buyer == w->player ? d.seller : d.buyer].display_name());
        e["what"] = capital_case(RES_NAMES[d.res]);
        e["amount"] = d.amount;
        deals.push_back(e);
    }
    out["deals"] = deals;
    return out;
}

Dictionary GreatWarSim::politics() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    const IdeologyPath& P = N.path[N.ideology];
    out["government"] = String(IDEO_TITLES[N.ideology]);
    out["current"] = N.sub >= 0 ? to_godot(w->subs[N.sub].name) : String();
    out["leader"] = to_godot(P.leader);
    out["party"] = to_godot(P.party);
    out["title"] = to_godot(P.title);
    out["stability"] = N.stability;
    out["war_support"] = N.war_support;
    out["influence"] = N.influence;
    out["conscription"] = String(CONS_LAW_NAMES[N.law_cons]);
    out["economy"] = String(ECON_LAW_NAMES[N.law_econ]);
    Array support;
    for (int i = 0; i < IDEO_N; i++) {
        Dictionary e;
        e["name"] = String(IDEO_TITLES[i]);
        e["party"] = to_godot(N.path[i].party);
        e["share"] = N.pop[i];
        e["rules"] = i == N.ideology;
        support.push_back(e);
    }
    out["support"] = support;
    out["campaign"] = N.campaign;
    out["tag"] = to_godot(N.tag);
    Array cabinet;
    for (int i = 0; i < MIN_N; i++) {
        int m = N.minister[i];
        Dictionary e;
        e["post"] = String(MINISTRY_NAMES[i]);
        e["name"] = m >= 0 ? to_godot(w->minister_name(me, m)) : String("vacant");
        e["kind"] = m >= 0 ? to_godot(w->ministers[m].name) : String();
        cabinet.push_back(e);
    }
    out["cabinet"] = cabinet;
    Array laws;
    for (int i = 0; i < POL_N; i++) {
        if (N.law[i] < 0) continue;
        Dictionary e;
        e["name"] = String(POL_TITLES[i]);
        e["law"] = String(POL_NAMES[i][N.law[i]]);
        laws.push_back(e);
    }
    out["laws"] = laws;
    return out;
}

Dictionary GreatWarSim::society() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    const Nation& N = w->nations[w->player];
    out["people"] = N.pop_total;
    out["core_people"] = N.core_pop;
    out["wealth"] = N.wealth;
    out["food"] = N.food_ratio;
    out["manpower"] = N.manpower;
    Array classes;
    for (int i = 0; i < CLS_N; i++) {
        Dictionary e;
        e["name"] = String(CLASS_NAMES[i]);
        e["share"] = N.cls_share[i];
        e["mood"] = N.cls_sat[i];
        classes.push_back(e);
    }
    out["classes"] = classes;
    Array dev;
    for (int i = 0; i < DEV_N; i++) {
        Dictionary e;
        e["name"] = String(DEV_NAMES[i]);
        e["value"] = N.dev[i];
        e["target"] = N.dev_target[i];
        dev.push_back(e);
    }
    out["development"] = dev;
    Array spirits;
    for (const DynSpirit& s : N.spirits) {
        Dictionary e;
        e["name"] = to_godot(s.name);
        e["text"] = to_godot(s.desc);
        e["good"] = s.good;
        spirits.push_back(e);
    }
    out["spirits"] = spirits;
    return out;
}

Dictionary GreatWarSim::focus() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    if (N.focus >= 0) {
        const Focus& f = w->focuses[N.focus];
        out["name"] = to_godot(f.name);
        out["text"] = to_godot(f.desc);
        out["days"] = f.days;
        out["done"] = N.focus_prog;
    }
    int finished = 0;
    for (char c : N.focus_done) finished += c != 0;
    out["finished"] = finished;
    Array open;
    for (int i = 0; i < (int)w->focuses.size(); i++) {
        if (i == N.focus || !w->focus_available(me, i)) continue;
        Dictionary e;
        e["id"] = i;
        e["name"] = to_godot(w->focuses[i].name);
        e["text"] = to_godot(w->focuses[i].desc);
        e["days"] = w->focuses[i].days;
        e["branch"] = to_godot(w->focuses[i].branch);
        open.push_back(e);
    }
    out["open"] = open;
    if (N.prog >= 0) {
        out["programme"] = to_godot(w->programmes[N.prog].name);
        out["programme_done"] = N.prog_days > 0 ? N.prog_done / N.prog_days : 0.0f;
    }
    return out;
}

Dictionary GreatWarSim::research() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    Array slots;
    for (int kind = 0; kind < 2; kind++)
        for (const ResearchSlot& s : kind ? N.xslots : N.slots) {
            Dictionary e;
            e["experimental"] = kind == 1;
            e["slot"] = (int)(&s - (kind ? N.xslots : N.slots).data());
            if (s.tech >= 0) {
                e["name"] = to_godot(w->techs[s.tech].name);
                e["text"] = to_godot(w->techs[s.tech].desc);
                e["year"] = w->techs[s.tech].year;
                e["days"] = w->tech_days(me, s.tech);
                e["done"] = s.prog;
            }
            slots.push_back(e);
        }
    out["slots"] = slots;
    int known = 0, open = 0;
    for (int i = 0; i < (int)w->techs.size(); i++) {
        if (i < (int)N.techs.size() && N.techs[i]) known++;
        else if (w->tech_available(me, i)) open++;
    }
    out["known"] = known;
    out["open"] = open;
    out["era"] = w->tech_era(me);
    return out;
}

// ---- what the head of government can do from the papers on the desk ----------------------------------------------

// every nation one can take office in, the great powers first
Array GreatWarSim::nations() const {
    Array out;
    if (!w) return out;
    std::vector<int> list;
    for (int i = 0; i < (int)w->nations.size(); i++) {
        const Nation& N = w->nations[i];
        if (N.exists && !N.rebel_slot && N.num_provs > 0 && N.capital >= 0) list.push_back(i);
    }
    std::stable_sort(list.begin(), list.end(), [&](int a, int b) {
        const Nation &A = w->nations[a], &B = w->nations[b];
        if (A.major != B.major) return A.major;
        return A.pop_total > B.pop_total;
    });
    for (int i : list) {
        const Nation& N = w->nations[i];
        Dictionary e;
        e["tag"] = to_godot(N.tag);
        e["name"] = to_godot(N.display_name());
        e["great"] = N.major;
        e["people"] = N.pop_total;
        e["government"] = String(IDEO_TITLES[N.ideology]);
        e["leader"] = to_godot(N.path[N.ideology].leader);
        e["lat"] = w->provs[N.capital].lat;
        e["lon"] = w->provs[N.capital].lon;
        e["id"] = i + 1;                                    // as the map's palette numbers its owners
        e["capital"] = to_godot(w->provs[N.capital].name);
        e["colour"] = godot::Color(N.col[0] / 255.0f, N.col[1] / 255.0f, N.col[2] / 255.0f);
        e["blurb"] = to_godot(N.blurb);
        e["faction"] = N.faction >= 0 ? to_godot(w->factions[N.faction].name) : String();
        e["provinces"] = N.num_provs;
        e["navy"] = N.navy_pow;
        e["stability"] = N.stability;
        int cf = 0, mf = 0, divs = 0, colonies = 0, wars = 0;
        // the box its home country fills on the map, for the view that shows it whole
        float lat0 = 90, lat1 = -90;
        double sx = 0, sy = 0;
        std::vector<float> lons;
        for (const Province& P : w->provs) {
            if (P.owner != i) continue;
            cf += P.cf;
            mf += P.mf;
            if (P.status != ST_CORE) { colonies++; continue; }
            if (P.antarctic) continue;
            lat0 = std::min(lat0, P.lat);
            lat1 = std::max(lat1, P.lat);
            sx += std::cos(P.lon * 3.14159265 / 180);
            sy += std::sin(P.lon * 3.14159265 / 180);
            lons.push_back(P.lon);
        }
        float mid = (float)(std::atan2(sy, sx) * 180 / 3.14159265), half = 0;
        for (float lo : lons) {
            float d = std::fmod(lo - mid + 540.0f, 360.0f) - 180.0f;
            half = std::max(half, std::fabs(d));
        }
        if (lat0 > lat1) { lat0 = lat1 = w->provs[N.capital].lat; mid = w->provs[N.capital].lon; }
        e["home_lon"] = mid;
        e["home_half"] = half;
        e["home_south"] = lat0;
        e["home_north"] = lat1;
        for (const Division& d : w->divs) if (d.alive && d.nation == i) divs++;
        for (size_t k = 0; k < w->nations.size(); k++) if (w->war[i][k]) wars++;
        e["factories"] = cf;
        e["arms_works"] = mf;
        e["divisions"] = divs;
        e["colonies"] = colonies;
        e["wars"] = wars;
        out.push_back(e);
    }
    return out;
}

bool GreatWarSim::start_focus(int id) {
    if (!w || w->player < 0 || id < 0 || id >= (int)w->focuses.size()) return false;
    return w->start_focus(w->player, id);
}

// what a laboratory could take up: known to be within reach and not on another bench already
Array GreatWarSim::techs_open() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    auto busy = [&](int t) {
        for (auto& s : N.slots) if (s.tech == t) return true;
        for (auto& s : N.xslots) if (s.tech == t) return true;
        return false;
    };
    for (int i = 0; i < (int)w->techs.size(); i++) {
        if ((i < (int)N.techs.size() && N.techs[i]) || busy(i) || !w->tech_available(me, i)) continue;
        Dictionary e;
        e["id"] = i;
        e["name"] = to_godot(w->techs[i].name);
        e["text"] = to_godot(w->techs[i].desc);
        e["field"] = capital_case(w->techs[i].cat.c_str());
        e["year"] = w->techs[i].year;
        e["days"] = w->tech_days(me, i);
        e["experimental"] = w->techs[i].experimental;
        out.push_back(e);
    }
    return out;
}

bool GreatWarSim::start_research(int slot, int tech, bool experimental) {
    if (!w || w->player < 0 || tech < 0 || tech >= (int)w->techs.size()) return false;
    return w->start_research(w->player, slot, tech, experimental);
}

// a share of the revenue moves between one purpose and the reserve; the shares still sum to one
bool GreatWarSim::shift_budget(int category, float by) {
    if (!w || w->player < 0 || category < 0 || category >= BUD_N || category == BUD_RESERVE) return false;
    Nation& N = w->nations[w->player];
    float d = std::max(-N.budget[category], std::min(by, N.budget[BUD_RESERVE]));
    if (std::fabs(d) < 0.0005f) return false;
    N.budget[category] += d;
    N.budget[BUD_RESERVE] -= d;
    return true;
}

// improve, guarantee, justify (a claim that allows war later) or war. Returns why not, or nothing when done.
String GreatWarSim::diplomacy(const String& action, const String& tag) {
    if (!w || w->player < 0) return "No government.";
    int me = w->player, them = w->nation_of(to_std(tag));
    if (them < 0 || them == me) return "No such government.";
    std::string a = to_std(action), why;
    if (a == "improve") return w->improve_relations(me, them) ? String() : String("We have not the influence for it, or we are at war with them.");
    if (a == "guarantee") return w->guarantee(me, them) ? String() : String("It takes 25 influence, and we may guarantee them only once.");
    if (a == "justify") return w->start_justify(me, them) ? String() : String("It takes 40 influence, and only one claim can be prepared at a time.");
    if (a == "war") {
        if (!w->can_declare_war(me, them, &why)) return to_godot(why);
        w->declare_war(me, them);
        return String();
    }
    return "Unknown.";
}

Array GreatWarSim::templates() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    for (int i = 0; i < (int)N.tmpls.size(); i++) {
        if (N.tmpls[i].obsolete) continue;
        std::string why;
        Dictionary e;
        e["id"] = i;
        e["name"] = to_godot(N.tmpls[i].name);
        e["kind"] = capital_case(N.tmpls[i].kind.c_str());
        e["can"] = w->can_recruit(me, i, &why);
        e["why"] = to_godot(why);
        out.push_back(e);
    }
    return out;
}

String GreatWarSim::recruit(int tmpl) {
    if (!w || w->player < 0) return "No government.";
    std::string why;
    if (!w->can_recruit(w->player, tmpl, &why)) return to_godot(why);
    return w->recruit(w->player, tmpl) ? String() : String("The division could not be raised.");
}

// ---- the rest of the government's business: laws, cabinet, decisions, the service, the fleet, works and trade ----------

// every law with its options: conscription, the economy, then the five policies
Array GreatWarSim::laws() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    auto group = [&](const char* key, const char* title, int count, int now, auto name, auto allowed) {
        Dictionary g;
        g["group"] = String(key);
        g["name"] = String(title);
        Array opts;
        for (int o = 0; o < count; o++) {
            if (!name(o)[0]) continue;
            Dictionary e;
            e["option"] = o;
            e["name"] = String(name(o));
            e["current"] = o == now;
            e["allowed"] = o != now && allowed(o);
            opts.push_back(e);
        }
        g["options"] = opts;
        out.push_back(g);
    };
    group("cons", "Conscription", LAW_CONS_N, N.law_cons, [&](int o) { return CONS_LAW_NAMES[o]; }, [&](int o) { return w->law_allowed(me, o, N.law_econ); });
    group("econ", "Economy", LAW_ECON_N, N.law_econ, [&](int o) { return ECON_LAW_NAMES[o]; }, [&](int o) { return w->law_allowed(me, N.law_cons, o); });
    for (int i = 0; i < POL_N; i++) {
        std::string key = "pol" + std::to_string(i);
        group(key.c_str(), POL_TITLES[i], POL_OPTS, N.law[i], [&](int o) { return POL_NAMES[i][o]; }, [&](int o) { return w->law_ok(me, i, o); });
    }
    return out;
}

String GreatWarSim::set_law(const String& group, int option) {
    if (!w || w->player < 0) return "No government.";
    int me = w->player;
    Nation& N = w->nations[me];
    std::string g = to_std(group);
    bool done = false;
    if (g == "cons") done = w->set_law(me, option, N.law_econ);
    else if (g == "econ") done = w->set_law(me, N.law_cons, option);
    else if (g.rfind("pol", 0) == 0) done = w->set_policy(me, std::atoi(g.c_str() + 3), option);
    return done ? String() : String("A new law takes 50 influence, and this government must be one that would pass it.");
}

// who could hold a post in the cabinet
Array GreatWarSim::ministers(int post) const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    for (int m = 0; m < (int)w->ministers.size(); m++) {
        const Minister& M = w->ministers[m];
        if (M.ministry != post) continue;
        std::string why;
        Dictionary e;
        e["id"] = m;
        e["kind"] = to_godot(M.name);
        e["text"] = to_godot(M.desc);
        e["name"] = to_godot(w->minister_name(me, m));
        e["holds"] = N.minister[post] == m;
        e["can"] = N.minister[post] != m && w->can_appoint(me, m, &why);
        e["why"] = to_godot(why);
        out.push_back(e);
    }
    return out;
}

String GreatWarSim::appoint(int minister) {
    if (!w || w->player < 0 || minister < 0 || minister >= (int)w->ministers.size()) return "No such minister.";
    std::string why;
    if (!w->can_appoint(w->player, minister, &why)) return to_godot(why);
    return w->appoint_minister(w->player, minister) ? String() : String("The appointment could not be made.");
}

Array GreatWarSim::decisions() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    for (int i = 0; i < (int)w->decisions.size(); i++) {
        if (!w->decision_visible(me, i)) continue;
        const Decision& D = w->decisions[i];
        Dictionary e;
        e["id"] = i;
        e["name"] = to_godot(D.name);
        e["text"] = to_godot(D.desc);
        e["cost"] = D.cost;
        e["days"] = D.days;
        e["ministry"] = D.ministry >= 0 ? String(MINISTRY_NAMES[D.ministry]) : String();
        e["can"] = w->decision_available(me, i);
        out.push_back(e);
    }
    return out;
}

bool GreatWarSim::take_decision(int id) {
    return w && w->player >= 0 && id >= 0 && id < (int)w->decisions.size() && w->take_decision(w->player, id);
}

Array GreatWarSim::programmes() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    for (int i = 0; i < (int)w->programmes.size(); i++) {
        if (!w->programme_visible(me, i)) continue;
        const Programme& P = w->programmes[i];
        Dictionary e;
        e["id"] = i;
        e["name"] = to_godot(P.name);
        e["text"] = to_godot(P.desc);
        e["days"] = P.days;
        e["can"] = w->programme_available(me, i);
        e["running"] = w->nations[me].prog == i;
        out.push_back(e);
    }
    return out;
}

bool GreatWarSim::start_programme(int id) {
    return w && w->player >= 0 && id >= 0 && id < (int)w->programmes.size() && w->start_programme(w->player, id);
}

// open a campaign for an ideology's support (0..3), or stop the one running (-1)
bool GreatWarSim::campaign(int ideology) {
    if (!w || w->player < 0) return false;
    if (ideology < 0) {
        if (w->nations[w->player].campaign < 0) return false;
        w->stop_campaign(w->player);
        return true;
    }
    return w->start_campaign(w->player, ideology);
}

Array GreatWarSim::candidates() const {
    Array out;
    if (!w || w->player < 0) return out;
    const Nation& N = w->nations[w->player];
    for (int i = 0; i < (int)N.candidates.size(); i++) {
        const Agent& a = N.candidates[i];
        Dictionary e;
        e["index"] = i;
        e["name"] = to_godot(a.name);
        e["infiltration"] = a.infiltration;
        e["counter"] = a.counter;
        e["cover"] = a.cover;
        e["cost"] = w->agent_cost(a);
        e["can"] = N.influence >= w->agent_cost(a) && w->agents_total(w->player) < w->max_agents(w->player);
        out.push_back(e);
    }
    return out;
}

bool GreatWarSim::hire_agent(int index) {
    return w && w->player >= 0 && w->hire_agent(w->player, index);
}

// by +1: our best waiting agent goes to that country (our own tag: to hunt spies at home); -1 recalls the weakest there
bool GreatWarSim::post_agent(const String& tag, int by) {
    if (!w || w->player < 0) return false;
    int t = w->nation_of(to_std(tag));
    return t >= 0 && w->post_agent(w->player, t, by);
}

bool GreatWarSim::fleet_mission(int fleet, int mission) {
    if (!w || w->player < 0) return false;
    Nation& N = w->nations[w->player];
    if (fleet < 0 || fleet >= (int)N.fleets.size() || mission < 0 || mission >= FLEET_N || N.fleets[fleet].mission == mission) return false;
    w->set_fleet_mission(w->player, N.fleets[fleet], mission);
    return true;
}

// what could be built in a province of ours
Array GreatWarSim::builds(int province) const {
    Array out;
    if (!w || w->player < 0 || province < 0 || province >= (int)w->provs.size()) return out;
    int me = w->player;
    if (w->provs[province].owner != me) return out;
    for (int t = 0; t < B_N; t++) {
        std::string why;
        Dictionary e;
        e["type"] = t;
        e["name"] = capital_case(BUILD_NAMES[t]);
        e["level"] = w->provs[province].level(t);
        e["can"] = w->can_build(me, province, t, &why);
        e["why"] = to_godot(why);
        e["cost"] = w->build_cost_at(me, province, t);
        out.push_back(e);
    }
    return out;
}

String GreatWarSim::build(int province, int type) {
    if (!w || w->player < 0 || province < 0 || province >= (int)w->provs.size() || type < 0 || type >= B_N) return "Nothing to build there.";
    std::string why;
    if (!w->can_build(w->player, province, type, &why)) return to_godot(why);
    return w->queue_build(w->player, province, type) ? String() : String("The works could not be started.");
}

// our construction queue in order, as tick_economy works through it: up to 10 free civil factories a project
Array GreatWarSim::build_queue() const {
    Array out;
    if (!w || w->player < 0) return out;
    const Nation& N = w->nations[w->player];
    float avail = N.cf_free;
    for (const Build& b : N.builds) {
        const Province& p = w->provs[b.prov];
        float use = p.controller == w->player ? std::min(std::max(avail, 0.0f), 10.0f) : 0.0f;
        avail -= use;
        float cost = w->build_cost_at(w->player, b.prov, b.type);
        float rate = w->build_rate(w->player, b.prov) * use / 10;
        Dictionary e;
        e["name"] = capital_case(BUILD_NAMES[b.type]);
        e["province"] = to_godot(p.name);
        e["done"] = cost > 0 ? b.prog / cost : 1.0f;
        e["factories"] = use;
        e["days"] = rate > 0 ? (int)std::ceil((cost - b.prog) / rate) : -1;
        out.push_back(e);
    }
    return out;
}

bool GreatWarSim::cancel_build(int index) {
    if (!w || w->player < 0) return false;
    auto& q = w->nations[w->player].builds;
    if (index < 0 || index >= (int)q.size()) return false;
    q.erase(q.begin() + index);
    return true;
}

bool GreatWarSim::raise_build(int index) {
    if (!w || w->player < 0) return false;
    auto& q = w->nations[w->player].builds;
    if (index < 1 || index >= (int)q.size()) return false;
    std::swap(q[index], q[index - 1]);
    return true;
}

// each material: what we make and need, and who has some to spare
Array GreatWarSim::market() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    for (int r = 0; r < RES_N; r++) {
        Dictionary e;
        e["resource"] = r;
        e["name"] = capital_case(RES_NAMES[r]);
        e["made"] = N.res_prod[r];
        e["needed"] = N.res_need[r];
        e["bought"] = N.res_in[r];
        e["sold"] = N.res_out[r];
        std::vector<std::pair<float, int>> have;
        for (int i = 0; i < (int)w->nations.size(); i++) {
            if (i == me || !w->nations[i].exists || w->war[me][i] || w->relation(me, i) < -30) continue;
            float spare = w->tradeable(i, r);
            if (spare >= 1.0f) have.push_back({spare, i});
        }
        std::sort(have.rbegin(), have.rend());
        Array sellers;
        for (size_t k = 0; k < have.size() && k < 4; k++) {
            Dictionary s;
            s["tag"] = to_godot(w->nations[have[k].second].tag);
            s["name"] = to_godot(w->nations[have[k].second].display_name());
            s["spare"] = have[k].first;
            sellers.push_back(s);
        }
        e["sellers"] = sellers;
        out.push_back(e);
    }
    return out;
}

String GreatWarSim::buy(const String& tag, int resource, float amount) {
    if (!w || w->player < 0 || resource < 0 || resource >= RES_N) return "No such material.";
    int them = w->nation_of(to_std(tag));
    if (them < 0) return "No such government.";
    if (w->nations[w->player].cf_free < std::ceil(amount / 4.0f)) return "It takes a free civil factory for every four units bought.";
    return w->request_trade(w->player, them, resource, amount) ? String() : String("They have not that much to spare, or will not sell to us.");
}

bool GreatWarSim::cancel_deal(int index) {
    if (!w || w->player < 0 || index < 0 || index >= (int)w->deals.size()) return false;
    const TradeDeal& d = w->deals[index];
    if (d.buyer != w->player && d.seller != w->player) return false;
    w->cancel_deal((size_t)index);
    return true;
}

// aircraft in store that a wing could be formed from
Array GreatWarSim::air_designs() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    for (auto& [design, count] : w->nations[me].stock) {
        if (design < 0 || design >= (int)w->designs.size() || count < 1) continue;
        const Design& d = w->designs[design];
        if (w->chassis[d.chassis].cat != CAT_AIR || d.role == RO_HELICOPTER) continue;
        Dictionary e;
        e["design"] = design;
        e["name"] = to_godot(d.name);
        e["stock"] = (int)count;
        e["can"] = true;                    // the simulation refuses if there are too few for a wing
        out.push_back(e);
    }
    return out;
}

bool GreatWarSim::form_wing(int design) {
    return w && w->player >= 0 && w->form_wing(w->player, design);
}

// What can be seen and heard from a window in the capital: today's weather there, the year, whether the nation is at
// war, and how many air wings it keeps. For the office's ambience; nothing here changes the simulation.
Dictionary GreatWarSim::capital() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    out["year"] = w->year();
    out["at_war"] = w->at_war(me);
    int wings = 0;
    for (const AirWing& a : w->wings)
        if (a.alive && a.nation == me) wings++;
    out["wings"] = wings;
    out["weather"] = N.capital >= 0 ? w->weather(N.capital).cond : (int)WX_CLEAR;
    out["temperature"] = N.capital >= 0 ? w->weather(N.capital).temp : 15.0f;
    out["clear"] = N.capital < 0 || w->weather(N.capital).cond == WX_CLEAR;
    return out;
}

Dictionary GreatWarSim::forces() const {
    Dictionary out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    const Nation& N = w->nations[me];
    out["manpower"] = N.manpower;
    out["navy"] = N.navy_pow;
    // the army: each army under its general, then what belongs to none, by template
    std::map<int, int> in_army;
    std::map<int, int> loose;
    int total = 0, training = 0;
    for (const Division& d : w->divs) {
        if (!d.alive || d.nation != me) continue;
        total++;
        if (d.train > 0) training++;
        if (d.army >= 0) in_army[d.army]++;
        else loose[d.tmpl]++;
    }
    out["divisions"] = total;
    out["training"] = training;
    Array armies;
    for (const Army& a : N.armies) {
        const Commander* c = w->commander(me, a.commander);
        Dictionary e;
        e["name"] = to_godot(a.name);
        e["general"] = c ? to_godot(c->name) : String("no general");
        e["skill"] = c ? String::num_int64(c->attack) + " / " + String::num_int64(c->defence) + " / " + String::num_int64(c->logistics) : String();
        e["divisions"] = in_army[a.id];
        e["plan"] = String(PLAN_NAMES[a.plan]);
        armies.push_back(e);
    }
    out["armies"] = armies;
    Array reserve;
    for (auto& [t, n] : loose) {
        Dictionary e;
        e["name"] = t >= 0 && t < (int)N.tmpls.size() ? to_godot(N.tmpls[t].name) : String("Division");
        e["divisions"] = n;
        reserve.push_back(e);
    }
    out["reserve"] = reserve;
    Array fleets;
    for (const Fleet& f : N.fleets) {
        const Commander* c = w->commander(me, f.admiral);
        float ships = 0;
        Array classes;
        for (auto& [design, count] : f.ships) {
            if (count < 0.5f) continue;
            ships += count;
            Dictionary s;
            s["name"] = to_godot(w->designs[design].name);
            s["kind"] = to_godot(w->design_kind(w->designs[design]));
            s["count"] = (int)std::floor(count + 0.5f);
            classes.push_back(s);
        }
        Dictionary e;
        e["name"] = to_godot(f.name);
        e["admiral"] = c ? to_godot(c->name) : String("no admiral");
        e["ships"] = (int)std::floor(ships + 0.5f);
        e["classes"] = classes;
        e["mission"] = String(FLEET_NAMES[f.mission]);
        e["at"] = f.dest >= 0 ? String("sailing to ") + to_godot(w->provs[f.dest].name) : f.prov >= 0 ? String("off ") + to_godot(w->provs[f.prov].name) : String("in port");
        fleets.push_back(e);
    }
    out["fleets"] = fleets;
    Array wings;
    for (const AirWing& a : w->wings) {
        if (!a.alive || a.nation != me) continue;
        Dictionary e;
        e["name"] = String(WING_NAMES[a.type]);
        e["aircraft"] = a.design >= 0 ? to_godot(w->designs[a.design].name) : String();
        e["planes"] = (int)std::floor(a.planes + 0.5f);
        e["base"] = a.base >= 0 ? to_godot(w->provs[a.base].name) : String();
        e["mission"] = String(MISSION_NAMES[a.mission]);
        e["target"] = a.target >= 0 ? to_godot(w->provs[a.target].name) : String();
        wings.push_back(e);
    }
    out["wings"] = wings;
    return out;
}

// ---- the map

int GreatWarSim::map_width() const { return GW; }
int GreatWarSim::map_height() const { return GH; }

PackedByteArray GreatWarSim::province_ids(int step) const {
    PackedByteArray out;
    if (!w || step < 1) return out;
    int mw = GW / step, mh = GH / step;
    out.resize((int64_t)mw * mh * 2);
    uint8_t* o = out.ptrw();
    for (int y = 0; y < mh; y++)
        for (int x = 0; x < mw; x++) {
            int id = w->pmap[(size_t)(y * step) * GW + x * step] + 1;     // the sea is -1
            *o++ = (uint8_t)(id & 255);
            *o++ = (uint8_t)(id >> 8);
        }
    return out;
}

PackedByteArray GreatWarSim::province_palette() const {
    PackedByteArray out;
    if (!w) return out;
    out.resize(PALETTE_W * 3 * 4);
    out.fill(0);
    uint8_t* o = out.ptrw();
    // column 0 is the sea; province p is column p + 1
    for (int p = 0; p < (int)w->provs.size() && p + 1 < PALETTE_W; p++) {
        const Province& P = w->provs[p];
        uint8_t* fill = o + (p + 1) * 4;
        uint8_t* owner = o + (PALETTE_W + p + 1) * 4;
        uint8_t* held = o + (2 * PALETTE_W + p + 1) * 4;
        if (P.owner >= 0) {
            const Nation& N = w->nations[P.owner];
            auto put = [&](int r, int g, int b, int a) { fill[0] = (uint8_t)r; fill[1] = (uint8_t)g; fill[2] = (uint8_t)b; fill[3] = (uint8_t)a; };
            int me = w->player;
            if (map_mode == 1 && me >= 0) {                 // how every government stands with ours
                if (P.owner == me) put(60, 110, 170, 255);
                else if (w->war[me][P.owner]) put(170, 40, 30, 255);
                else if (w->allied(me, P.owner)) put(70, 140, 150, 255);
                else {
                    float r = std::clamp(w->relation(me, P.owner) / 100.0f, -1.0f, 1.0f);
                    if (r >= 0) put((int)(190 - 110 * r), (int)(180 - 30 * r), (int)(140 - 60 * r), 230);
                    else put((int)(190 + 20 * -r), (int)(180 - 80 * -r), (int)(140 - 70 * -r), 230);
                }
            } else if (map_mode == 2) {                     // the ground
                static const unsigned char TER_COL[TER_N][3] = {{176, 190, 120}, {96, 140, 88}, {170, 150, 104}, {130, 112, 100},
                                                                {222, 196, 130}, {60, 124, 84}, {214, 222, 226}};
                put(TER_COL[P.terrain][0], TER_COL[P.terrain][1], TER_COL[P.terrain][2], 255);
            } else if (map_mode == 3) {                     // works and railways
                float f = std::min(1.0f, (P.cf + P.mf) / 12.0f);
                if (P.cf + P.mf > 0) put((int)(200 - 90 * f), (int)(150 - 90 * f), (int)(110 - 70 * f), (int)(150 + 105 * f));
                else put(200, 190, 160, 60 + 25 * std::min(4, P.infra));
            } else if (map_mode == 4) {                     // unrest
                float u = std::clamp(P.unrest / 100.0f, 0.0f, 1.0f);
                put((int)(150 + 60 * u), (int)(170 - 130 * u), (int)(120 - 80 * u), (int)(90 + 165 * u));
            } else {
                put(N.col[0], N.col[1], N.col[2], P.status == ST_CORE ? 255 : 170);      // colonies and protectorates take a lighter wash
            }
        }
        int id = P.owner + 1;
        owner[0] = (uint8_t)(id & 255);
        owner[1] = (uint8_t)(id >> 8);
        owner[2] = P.owner == w->player ? 255 : 0;
        owner[3] = 255;
        if (P.controller >= 0 && P.controller != P.owner) {
            const Nation& C = w->nations[P.controller];
            held[0] = C.col[0]; held[1] = C.col[1]; held[2] = C.col[2]; held[3] = 255;
        }
    }
    return out;
}

Dictionary GreatWarSim::province(int id) const {
    Dictionary out;
    if (!w || id < 0 || id >= (int)w->provs.size()) return out;
    const Province& P = w->provs[id];
    bool regional = !P.region.empty() && P.region.size() <= 26;     // the region, its chief town beside it; a few modern names are too long to use
    out["name"] = to_godot(regional ? P.region : P.name);
    out["town"] = regional ? to_godot(P.name) : String();
    out["owner"] = P.owner >= 0 ? to_godot(w->nations[P.owner].display_name()) : String("No one");
    out["owner_tag"] = P.owner >= 0 ? to_godot(w->nations[P.owner].tag) : String();
    out["ours"] = P.owner == w->player;
    out["held_by"] = P.controller >= 0 && P.controller != P.owner ? to_godot(w->nations[P.controller].display_name()) : String();
    out["status"] = capital_case(STATUS_NAMES[P.status]);
    out["terrain"] = capital_case(TERRAIN_NAMES[P.terrain]);
    out["people"] = P.pop;
    out["city"] = P.size > 0;
    out["capital"] = P.capital;
    out["factories"] = P.cf;
    out["arms_works"] = P.mf;
    out["railways"] = P.infra;
    out["fort"] = P.fort;
    out["unrest"] = P.unrest;
    out["lat"] = P.lat;
    out["lon"] = P.lon;
    if (P.owner >= 0) {
        const Nation& N = w->nations[P.owner];
        out["government"] = String(IDEO_TITLES[N.ideology]);
        out["leader"] = to_godot(N.path[N.ideology].leader);
        out["nation_people"] = N.pop_total;
        out["nation_provinces"] = N.num_provs;
        out["at_war_with_us"] = w->player >= 0 && (bool)w->war[w->player][P.owner];
        out["relation"] = w->player >= 0 && P.owner != w->player ? w->relation(w->player, P.owner) : 0.0f;
    }
    return out;
}

Array GreatWarSim::places() const {
    Array out;
    if (!w) return out;
    // a nation's name goes over the middle of its home country, not over its capital
    std::vector<double> sx(w->nations.size(), 0), sy(w->nations.size(), 0), sz(w->nations.size(), 0), area(w->nations.size(), 0);
    for (const Province& P : w->provs) {
        if (P.owner < 0 || P.status != ST_CORE || P.antarctic) continue;
        double la = P.lat * 3.14159265 / 180, lo = P.lon * 3.14159265 / 180, c = std::max(1, P.cells);
        sx[P.owner] += c * std::cos(la) * std::cos(lo);
        sy[P.owner] += c * std::cos(la) * std::sin(lo);
        sz[P.owner] += c * std::sin(la);
        area[P.owner] += c * std::cos(la);
    }
    for (size_t i = 0; i < w->nations.size(); i++) {
        const Nation& N = w->nations[i];
        if (!N.exists || N.capital < 0 || N.num_provs <= 0) continue;
        Dictionary e;
        float lat = w->provs[N.capital].lat, lon = w->provs[N.capital].lon;
        if (sx[i] != 0 || sy[i] != 0 || sz[i] != 0) {
            lat = (float)(std::atan2(sz[i], std::sqrt(sx[i] * sx[i] + sy[i] * sy[i])) * 180 / 3.14159265);
            lon = (float)(std::atan2(sy[i], sx[i]) * 180 / 3.14159265);
        }
        e["nation"] = true;
        e["name"] = to_godot(N.display_name());
        e["lat"] = lat;
        e["lon"] = lon;
        e["weight"] = (float)N.vp_total + (N.major ? 1000.0f : 0.0f);
        e["area"] = (float)area[i];           // of the home country, in map cells at the equator's scale
        out.push_back(e);
    }
    for (const Province& P : w->provs) {
        if (P.size <= 0) continue;
        Dictionary e;
        e["nation"] = false;
        e["name"] = to_godot(P.name);
        e["lat"] = P.lat;
        e["lon"] = P.lon;
        e["weight"] = (float)P.size + (P.capital ? 10.0f : 0.0f);
        e["capital"] = P.capital;
        out.push_back(e);
    }
    return out;
}

void GreatWarSim::set_map_mode(int mode) { map_mode = std::clamp(mode, 0, 4); }

// What stands on the map and may be seen by the player (the game's own fog): every province's divisions as one stack
// a nation, and the fleets. Our own stacks carry their divisions' ids, so the hand can pick them up.
Dictionary GreatWarSim::map_units() const {
    Dictionary out;
    Array stacks, fleets;
    out["stacks"] = stacks;
    out["fleets"] = fleets;
    if (!w || w->player < 0) return out;
    int me = w->player;
    ForcesView fog = w->forces_view(me);
    struct Stack { int n = 0, training = 0, moving = 0, dest = -1, fighting = 0; float str = 0, org = 0; godot::PackedInt32Array ids; Array names; };
    std::map<std::pair<int, int>, Stack> by;
    for (size_t i = 0; i < w->divs.size(); i++) {
        const Division& d = w->divs[i];
        if (!d.alive || d.prov < 0 || d.sea_days > 0) continue;
        if (d.nation != me && !fog.sees(*w, d.nation, d.prov)) continue;
        Stack& s = by[{d.prov, d.nation}];
        s.n++;
        s.str += d.str;
        s.org += d.org;
        if (d.train > 0) s.training++;
        if (!d.path.empty()) { s.moving++; s.dest = d.path.back(); }
        if (d.attacking >= 0) s.fighting++;
        if (d.nation == me) {
            s.ids.push_back((int)i);
            const Nation& N = w->nations[me];
            s.names.push_back(d.tmpl >= 0 && d.tmpl < (int)N.tmpls.size() ? to_godot(N.tmpls[d.tmpl].name) : String("Division"));
        }
    }
    for (auto& [key, s] : by) {
        const Province& P = w->provs[key.first];
        const Nation& N = w->nations[key.second];
        Dictionary e;
        e["province"] = key.first;
        e["place"] = to_godot(P.name);
        e["lat"] = P.lat;
        e["lon"] = P.lon;
        e["tag"] = to_godot(N.tag);
        e["nation"] = to_godot(N.display_name());
        e["colour"] = godot::Color(N.col[0] / 255.0f, N.col[1] / 255.0f, N.col[2] / 255.0f);
        e["ours"] = key.second == me;
        e["enemy"] = (bool)w->war[me][key.second];
        e["friend"] = key.second != me && w->allied(me, key.second);
        e["count"] = s.n;
        e["strength"] = s.str / s.n;
        e["training"] = s.training;
        e["moving"] = s.moving;
        e["fighting"] = s.fighting;
        e["ids"] = s.ids;
        e["names"] = s.names;
        if (s.dest >= 0) {
            e["dest"] = s.dest;
            e["dest_name"] = to_godot(w->provs[s.dest].name);
            e["dest_lat"] = w->provs[s.dest].lat;
            e["dest_lon"] = w->provs[s.dest].lon;
        }
        stacks.push_back(e);
    }
    for (size_t n = 0; n < w->nations.size(); n++) {
        const Nation& N = w->nations[n];
        if (!N.exists) continue;
        for (size_t k = 0; k < N.fleets.size(); k++) {
            const Fleet& f = N.fleets[k];
            if (f.prov < 0) continue;
            if ((int)n != me && !fog.sees(*w, (int)n, f.prov)) continue;
            float ships = 0;
            for (auto& [design, count] : f.ships) ships += count;
            if (ships < 0.5f) continue;
            const Province& P = w->provs[f.prov];
            float lat = P.lat, lon = P.lon;
            Dictionary e;
            if (f.dest >= 0 && f.total > 0) {           // under way: somewhere between the two coasts
                const Province& D = w->provs[f.dest];
                float t = std::clamp(1.0f - f.days / f.total, 0.0f, 1.0f);
                float dl = std::fmod(D.lon - P.lon + 540.0f, 360.0f) - 180.0f;
                lat = P.lat + (D.lat - P.lat) * t;
                lon = P.lon + dl * t;
                e["dest_name"] = to_godot(D.name);
                e["dest_lat"] = D.lat;
                e["dest_lon"] = D.lon;
                e["days"] = f.days;
            }
            e["fleet"] = (int)k;
            e["name"] = to_godot(f.name);
            e["place"] = to_godot(P.name);
            e["province"] = f.prov;
            e["lat"] = lat;
            e["lon"] = lon;
            e["sailing"] = f.dest >= 0;
            e["tag"] = to_godot(N.tag);
            e["nation"] = to_godot(N.display_name());
            e["colour"] = godot::Color(N.col[0] / 255.0f, N.col[1] / 255.0f, N.col[2] / 255.0f);
            e["ours"] = (int)n == me;
            e["enemy"] = (bool)w->war[me][n];
            e["ships"] = (int)std::floor(ships + 0.5f);
            e["mission"] = String(FLEET_NAMES[f.mission]);
            fleets.push_back(e);
        }
    }
    return out;
}

// Every battle the player may see (combat.cpp): ours and our friends', and any whose attackers the fog lets through.
// One entry a province under assault, with what the generals are trying and how the day went.
Array GreatWarSim::battles() const {
    Array out;
    if (!w || w->player < 0) return out;
    int me = w->player;
    ForcesView fog = w->forces_view(me);
    for (auto& [prov, b] : w->battles) {
        if (b.attacker < 0 || b.defender < 0) continue;
        bool with_a = w->friendly(me, b.attacker), with_d = w->friendly(me, b.defender);
        if (!with_a && !with_d && !fog.sees(*w, b.attacker, prov)) continue;
        const Province& P = w->provs[prov];
        const Nation &A = w->nations[b.attacker], &D = w->nations[b.defender];
        bool guns = b.phase == BP_BOMBARD;
        Dictionary e;
        e["province"] = prov;
        e["place"] = to_godot(P.name);
        e["lat"] = P.lat;
        e["lon"] = P.lon;
        e["attacker"] = to_godot(A.display_name());
        e["defender"] = to_godot(D.display_name());
        e["attacker_adj"] = to_godot(A.adj);
        e["defender_adj"] = to_godot(D.adj);
        e["we_attack"] = with_a;
        e["we_defend"] = with_d;
        e["day"] = b.days;
        e["phase"] = b.phase;
        e["phase_name"] = String(PHASE_NAMES[b.phase]);
        e["attack_tactic"] = guns ? String("Waiting on the guns") : String(ATTACK_TACTICS[b.tac_a].name);
        e["attack_about"] = guns ? String("The guns are cutting down trenches and forts before the infantry goes in.") : String(ATTACK_TACTICS[b.tac_a].about);
        e["defence_tactic"] = guns ? String("Under the guns") : String(DEFENCE_TACTICS[b.tac_d].name);
        e["defence_about"] = guns ? String("The defenders sit out the shelling; their own guns answer.") : String(DEFENCE_TACTICS[b.tac_d].about);
        e["attack_odds"] = b.fire_a / std::max(0.01f, b.guard_d);      // the attack's fire to the defence's cover
        e["defence_odds"] = b.fire_d / std::max(0.01f, b.guard_a);     // the defence's fire to the attack's drive
        e["works"] = b.works;
        e["cut"] = b.cut;
        e["air"] = b.air;
        e["line_a"] = b.front_a;
        e["line_d"] = b.front_d;
        e["reserve_a"] = b.res_a;
        e["reserve_d"] = b.res_d;
        e["sides"] = b.sides;
        e["outflanked"] = b.outflanked;
        e["encircled"] = b.encircled;
        e["read"] = b.countered;
        e["lost_a"] = b.loss_a;         // thousands of men
        e["lost_d"] = b.loss_d;
        out.push_back(e);
    }
    return out;
}

String GreatWarSim::declare_war_now(const String& tag) {
    if (!w || w->player < 0) return "No government.";
    int them = w->nation_of(to_std(tag));
    if (them < 0 || them == w->player) return "There is no such nation.";
    w->declare_war(w->player, them, true);
    return String();
}

String GreatWarSim::move_divisions(const godot::PackedInt32Array& ids, int province) {
    if (!w || w->player < 0) return "No government.";
    if (province < 0 || province >= (int)w->provs.size()) return "There is no such place.";
    int ok = 0, bad = 0, training = 0;
    for (int i = 0; i < ids.size(); i++) {
        int id = ids[i];
        if (id < 0 || id >= (int)w->divs.size()) continue;
        Division& d = w->divs[id];
        if (!d.alive || d.nation != w->player) continue;
        if (d.train > 0) { training++; continue; }
        d.order = ORD_NONE;                      // a plain march ends any standing order
        if (w->order_move(d, province)) ok++;
        else bad++;
    }
    const std::string& name = w->provs[province].name;
    if (ok == 0 && training > 0 && bad == 0) return "They are still in training.";
    if (ok == 0) return to_godot("No way to " + name + ": the land is closed to us, or there are too few ships to cross.");
    if (bad > 0) return String::num_int64(bad) + to_godot(" divisions found no way to " + name + ".");
    return String();
}

String GreatWarSim::sail_fleet(int fleet, int province) {
    if (!w || w->player < 0) return "No government.";
    Nation& N = w->nations[w->player];
    if (fleet < 0 || fleet >= (int)N.fleets.size()) return "There is no such fleet.";
    if (province < 0 || province >= (int)w->provs.size()) return "There is no such place.";
    if (!w->order_fleet(w->player, N.fleets[fleet], province)) return to_godot("The fleet cannot reach the coast of " + w->provs[province].name + ".");
    return String();
}

Dictionary GreatWarSim::country(const String& tag) const {
    Dictionary e;
    if (!w || w->player < 0) return e;
    int me = w->player, i = w->nation_of(to_std(tag));
    if (i < 0) return e;
    const Nation &N = w->nations[me], &T = w->nations[i];
    e["tag"] = to_godot(T.tag);
    e["name"] = to_godot(T.display_name());
    e["ours"] = i == me;
    e["great"] = T.major;
    e["government"] = String(IDEO_TITLES[T.ideology]);
    e["leader"] = to_godot(T.path[T.ideology].leader);
    e["people"] = T.pop_total;
    e["faction"] = T.faction >= 0 ? to_godot(w->factions[T.faction].name) : String();
    e["relation"] = i == me ? 0.0f : w->relation(me, i);
    e["war"] = (bool)w->war[me][i];
    e["allied"] = i != me && w->allied(me, i);
    e["guaranteed"] = N.guarantees.count(i) > 0;
    e["guarantees_us"] = T.guarantees.count(me) > 0;
    {
        auto ct = N.courting.find(i);
        e["improve_cost"] = i == me ? 0 : w->improve_cost(me, i);                 // what the next approach costs in influence
        e["improve_due"] = ct == N.courting.end() ? 0.0f : ct->second.due;      // goodwill bought and still on its way
    }
    std::string why;
    e["can_war"] = i != me && w->can_declare_war(me, i, &why);
    e["why_no_war"] = to_godot(why);
    e["influence"] = N.influence;
    int divs = 0;
    for (const Division& d : w->divs) if (d.alive && d.nation == i) divs++;
    e["divisions"] = divs;
    e["navy"] = T.navy_pow;
    ForcesView fog = w->forces_view(me);
    e["known"] = i == me || (i < (int)fog.known.size() && fog.known[i]);     // their strength is ours to read only with intelligence there
    Array wars;
    for (size_t k = 0; k < w->nations.size(); k++)
        if (w->war[i][k] && w->nations[k].exists) wars.push_back(to_godot(w->nations[k].display_name()));
    e["wars"] = wars;
    return e;
}

bool GreatWarSim::save_game(const String& path) const {
    return w && w->save(to_std(path));
}

String GreatWarSim::load_game(const String& path) {
    if (!w) return "No world.";
    std::string why;
    if (!w->load_save(to_std(path), why)) return to_godot(why.empty() ? std::string("The file could not be read.") : why);
    w->choices.clear();
    w->completed.clear();
    return String();
}
