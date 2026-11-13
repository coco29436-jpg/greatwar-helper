// Daily tick: economy, politics, research, focuses, construction, recruitment and laws.
#include "world.h"

namespace {
const float CONS_RATE[LAW_CONS_N] = {0.010f, 0.025f, 0.050f, 0.090f};
const float ECON_RATE[LAW_ECON_N] = {0.35f, 0.25f, 0.15f};
const float FACTORY_K[ST_N] = {1.0f, 0.6f, 0.5f, 1.0f};
const float RES_K[ST_N] = {1.0f, 1.0f, 0.7f, 1.0f};
const float MP_K[ST_N] = {1.0f, 0.12f, 0.06f, 0.2f};
// food the market must supply per million people: peasants feed themselves, towns and the army do not
const float FOOD_PER_POP[ST_N] = {1.0f, 0.15f, 0.15f, 0.8f};
const float FOOD_K = 0.55f;              // farmland output per point of grain or livestock in provinces.txt
}  // namespace

void World::add_news(const std::string& text, int nation) {
    news.push_back({day, text, nation});
    if (news.size() > 400) news.erase(news.begin());
}

// The civilian factories working for a nation today, from saved state alone: read before recompute, so it must not
// lean on what yesterday's recompute left behind, or a game reloaded in the middle of a war drifts away from the original.
float World::factories_now(int n) const {
    float cf = 0;
    for (auto& p : provs)
        if (p.controller == n) cf += p.cf * (p.owner == n ? FACTORY_K[p.status] : 0.3f) * (1 - p.bomb) * (p.riot ? 0.5f : 1.0f);
    return cf;
}

void World::recompute(int i) {
    Nation& n = nations[i];
    update_slots(i);
    n.mods = Mods{};
    for (auto& a : n.ideas) n.mods.add(ideas[a.idea].mods);
    for (size_t t = 0; t < techs.size(); t++)
        if (n.techs[t] && n.tech_vis[t]) n.mods.add(techs[t].mods);     // a replaced common tech counts through its national version
    n.mods.add(n.path[n.ideology].mods);
    if (n.sub >= 0) n.mods.add(subs[n.sub].mods);
    for (int m : n.minister) if (m >= 0) n.mods.add(ministers[m].mods);
    development_mods(i);          // society.cpp
    compute_civil_equipment(i);   // equipment.cpp: locomotives, tractors, lorries against the country's needs
    compute_navy(i);
    n.mods.v[M_GRAIN] += 0.3f * n.farm_mech;
    n.mods.v[M_LIVESTOCK] += 0.15f * n.farm_mech;
    n.mods.v[M_SUPPLY] += 0.25f * (std::min(1.0f, n.rail_ratio) - 1) + 0.1f * n.truck_supply;

    n.cf_total = n.mf_total = n.pop_total = n.mp_cap = 0;
    n.vp_total = n.vp_lost = n.num_provs = 0;
    for (int r = 0; r < RES_N; r++) n.res_prod[r] = n.res_need[r] = n.res_in[r] = n.res_out[r] = 0;
    float colonial = 1 + n.mods[M_COLONIAL], food = 0, core_pop = 0;
    for (auto& p : provs) {
        if (p.controller == i) {
            bool own = p.owner == i;
            float regime_k = p.status == ST_CORE ? 1.0f : p.regime == REG_PLUNDER ? 1.35f : p.regime == REG_MODEL ? 0.9f : 1.0f;
            float fk = (own ? FACTORY_K[p.status] * (p.status == ST_COLONY ? colonial : 1) * regime_k : 0.3f) * (1 - p.bomb) * (p.riot ? 0.5f : 1.0f);
            float rk = own ? RES_K[p.status] : 0.5f;
            n.cf_total += p.cf * fk;
            n.mf_total += p.mf * fk;
            for (int r = R_COAL; r <= R_OIL; r++) n.res_prod[r] += p.res[r] * rk * (1 + n.mods[M_RES]);
            float farm = own ? 1 + 0.2f * p.proj[PJ_FARM] : 1, past = own ? 1 + 0.2f * p.proj[PJ_PASTURE] : 1;
            n.res_prod[R_GRAIN] += p.res[R_GRAIN] * FOOD_K * rk * farm * (1 + n.mods[M_GRAIN]);
            n.res_prod[R_LIVESTOCK] += p.res[R_LIVESTOCK] * FOOD_K * rk * past * (1 + n.mods[M_LIVESTOCK]);
            if (own) {
                n.mp_cap += p.pop * 1000 * CONS_RATE[n.law_cons] * MP_K[p.status] * (p.riot ? 0.5f : 1.0f);
                if (p.stations & STATION_RESEARCH) n.mods.v[M_RESEARCH] += 0.03f;
                if (p.stations & STATION_WHALING) n.res_prod[R_LIVESTOCK] += 2;
                if (p.stations & STATION_COAL) n.res_prod[R_COAL] += 2;
            }
            n.num_provs++;
        }
        if (p.owner == i) {
            n.pop_total += p.pop;
            food += p.pop * FOOD_PER_POP[p.status];
            if (p.status == ST_CORE && !p.antarctic) core_pop += p.pop;
            n.vp_total += p.vp;
            if (p.controller != i && p.controller >= 0 && war[i][p.controller]) n.vp_lost += p.vp;
        }
    }
    n.mp_cap *= 1 + n.mods[M_MANPOWER];
    n.consumer = std::max(0.0f, n.cf_total * (ECON_RATE[n.law_econ] + n.mods[M_CONSUMER]));
    n.trade_cf = 0;
    float lost = blockade(i);
    for (auto& d : deals) {
        float cost = ceilf(d.amount / 4.0f);
        if (d.buyer == i) { n.trade_cf -= cost; n.res_in[d.res] += d.amount * (1 - lost); }
        if (d.seller == i) { n.trade_cf += cost; n.res_out[d.res] += d.amount; }
    }
    n.cf_free = std::max(0.0f, n.cf_total - n.consumer + n.trade_cf);
    // production lines draw coal, iron and oil in the proportions their designs are made of, half a unit a factory
    for (auto& l : n.lines) {
        if (l.design < 0) continue;
        const float* rs = designs[l.design].st.res;
        float sum = std::max(0.01f, rs[0] + rs[1] + rs[2]);
        for (int r = 0; r < 3; r++) n.res_need[R_COAL + r] += 0.5f * l.factories * rs[r] / sum;
    }
    n.num_divs = 0;
    float horses = 0;
    for (auto& d : divs)
        if (d.alive && d.nation == i) {
            n.num_divs++;
            horses += tmpl_of(d).horses * 0.025f;
        }
    // food: people and soldiers eat grain and meat in the proportion they are used to; cavalry needs horses.
    // Only those who buy their food count: more of them with industry and a middle class
    float urban = clampv(0.06f + 0.4f * std::min(1.0f, core_pop > 0 ? n.cf_total / core_pop : 0) + 0.6f * n.cls_share[CLS_MIDDLE], 0.06f, 0.85f);
    food = food * urban + n.num_divs * 0.1f;
    n.res_need[R_GRAIN] += food * n.diet;
    n.res_need[R_LIVESTOCK] += food * (1 - n.diet) + horses;
    for (int r = 0; r < RES_N; r++) {
        float avail = n.res_prod[r] + n.res_in[r] - n.res_out[r];
        n.res_ratio[r] = n.res_need[r] <= 0.01f ? 1.0f : clampv(avail / n.res_need[r], 0.0f, 1.0f);
    }
    // what is actually eaten: each kind up to the usual share, then a surplus of one fills a shortage of the other
    float ag = std::max(0.0f, n.res_prod[R_GRAIN] + n.res_in[R_GRAIN] - n.res_out[R_GRAIN]);
    float al = std::max(0.0f, n.res_prod[R_LIVESTOCK] + n.res_in[R_LIVESTOCK] - n.res_out[R_LIVESTOCK] - horses);
    float cg = std::min(ag, food * n.diet), cl = std::min(al, food * (1 - n.diet));
    float short_ = food - cg - cl;
    float sg = std::min(ag - cg, short_);
    short_ -= sg;
    float sl = std::min(al - cl, short_);
    float eaten = cg + cl + sg + sl;
    n.food_ratio = food > 0.01f ? eaten / food : 1;
    n.diet_eaten = eaten > 0.01f ? (cg + sg) / eaten : n.diet;
    compute_income(i);
    compute_spirits(i);
}

float World::blockade(int n) const {
    float b = 0, own = navy(n) + navy_on(n, FLEET_TRADE);
    for (size_t e = 0; e < nations.size(); e++) {
        if (!war[n][e]) continue;
        float blk = navy_on((int)e, FLEET_BLOCKADE);
        if (blk > 0) b = std::max(b, 0.8f * blk / (blk + own + 1));
    }
    // coastal forts keep the blockaders further out: up to a third off what they stop
    int guns = 0;
    for (auto& p : provs) if (p.owner == n) guns += p.coast;
    return b * (1 - std::min(0.33f, 0.02f * guns));
}

float World::build_cost(int type) const {
    static const float COST[B_N] = {10800, 9000, 4000, 3500, 4500, 2000, 4500, 3500, 2500, 5000};
    return COST[type];
}

float World::build_cost_at(int n, int prov, int type) const {
    float c = build_cost(type) * (provs[prov].status == ST_CORE ? 1.0f : 1.35f) * TERRAIN[provs[prov].terrain].build;
    // factories need workers: past one factory per million people, each new one is dearer to staff
    if (type == B_CF || type == B_MF) {
        const Nation& N = nations[n];
        float per_head = N.core_pop > 0.1f ? (N.cf_total + N.mf_total) / N.core_pop : 1;
        c *= 1 + 0.8f * std::max(0.0f, per_head - 1);
    }
    return c;
}

void World::tick_economy(int i) {
    Nation& n = nations[i];
    // construction: up to 10 factories of output per project, in queue order
    float avail = n.cf_free;
    for (size_t k = 0; k < n.builds.size() && avail > 0;) {
        Build& b = n.builds[k];
        Province& p = provs[b.prov];
        if (p.controller != i) { k++; continue; }
        float use = std::min(avail, 10.0f);
        avail -= use;
        float cost = build_cost_at(i, b.prov, b.type);
        b.prog += build_rate(i, b.prov) * use / 10;
        if (b.prog >= cost) {
            if (b.type == B_CF || b.type == B_MF) p.level(b.type)++;
            else p.level(b.type) = std::min(5, p.level(b.type) + 1);
            if (i == player) add_news(fmt("%s completed in %s.", BUILD_NAMES[b.type], p.name.c_str()), i);
            n.builds.erase(n.builds.begin() + k);
            continue;
        }
        k++;
    }
    tick_production(i);       // equipment.cpp
    // manpower refills over half a year, never past what the army leaves unused
    float used_mp = 0;
    for (auto& d : divs)
        if (d.alive && d.nation == i) used_mp += d.st[DS_MP] * d.str;
    float room = std::max(0.0f, n.mp_cap - used_mp);
    n.manpower = std::min(room, n.manpower + n.mp_cap / 180.0f);
    n.manpower = std::max(0.0f, n.manpower);
    // influence and war justification
    n.influence = std::min(999.0f, n.influence + 1.0f + (n.major ? 0.25f : 0) + n.mods[M_INFL]);
    if (n.justify_target >= 0) {
        n.justify_days += 1;
        int t = n.justify_target;
        if (!nations[t].exists || allied(i, t)) n.justify_target = -1;
        else if (n.justify_days >= 60) {
            n.wargoals.insert(t);
            n.justify_target = -1;
            add_news(fmt("%s has a war goal against %s.", n.name.c_str(), nations[t].name.c_str()), i);
            add_relation(i, t, -15);
        }
    }
    // timed ideas
    n.ideas.erase(std::remove_if(n.ideas.begin(), n.ideas.end(), [&](const ActiveIdea& a) { return a.expires >= 0 && a.expires <= day; }),
                  n.ideas.end());
}

void World::tick_politics(int i) {
    Nation& n = nations[i];
    bool w = at_war(i);
    // the classes' mood counts as far as the franchise lets them be heard (society.cpp)
    float mood = 0;
    for (int c = 0; c < CLS_N; c++) mood += vote_weight(n.law[POL_VOTE], c) * (n.cls_sat[c] - 50);
    float target = 42 + n.mods[M_STAB] + (n.pop[n.ideology] - 40) * 0.3f + mood * 0.2f;
    if (n.campaign >= 0) target += CAMPAIGN_STABILITY;
    if (w && n.war_support < 40) target -= (40 - n.war_support) * 0.5f;
    n.stability = clampv(n.stability + (target - n.stability) * 0.02f, 0.0f, 100.0f);
    n.war_days = w ? n.war_days + 1 : std::max(0, n.war_days - 3);
    float wt = 22 + n.mods[M_WS] + (w ? 18 : 0) + n.pop[IDEO_NAT] * 0.2f + (n.faction >= 0 ? 4 : 0);
    if (w && n.vp_total > 0) wt -= 40.0f * n.vp_lost / n.vp_total;
    wt -= n.war_days / 365.0f * 6.0f;          // war exhaustion
    n.war_support = clampv(n.war_support + (wt - n.war_support) * 0.02f, 0.0f, 100.0f);

    n.pop[n.ideology] += 0.012f;
    n.pop[IDEO_AUT] += n.mods[M_POP_AUT];
    n.pop[IDEO_CON] += n.mods[M_POP_CON];
    n.pop[IDEO_SOC] += n.mods[M_POP_SOC];
    n.pop[IDEO_NAT] += n.mods[M_POP_NAT];
    if (n.stability < 45) n.pop[IDEO_SOC] += (45 - n.stability) * 0.0012f;
    if (n.war_support > 45) n.pop[IDEO_NAT] += (n.war_support - 45) * 0.0006f;
    if (n.campaign >= 0) {
        if (n.pop[n.campaign] >= CAMPAIGN_CAP) {
            if (i == player) add_news(fmt("The %s campaign has won over the country and winds down.", IDEO_NAMES[n.campaign]), i);
            n.campaign = -1;
        } else if (n.influence >= CAMPAIGN_DAILY) {     // out of influence it stalls, and costs nothing
            n.influence -= CAMPAIGN_DAILY;
            n.pop[n.campaign] += CAMPAIGN_RATE;
        }
    }
    float sum = 0;
    for (float& p : n.pop) { p = std::max(0.5f, p); sum += p; }
    for (float& p : n.pop) p = p * 100 / sum;
}


bool World::tech_available(int n, int t) const {
    const Nation& N = nations[n];
    if (N.techs[t] || !N.tech_vis[t]) return false;
    for (int r : techs[t].req)
        if (!N.techs[r]) return false;
    for (auto& s : N.slots)
        if (s.tech == t) return false;
    for (auto& s : N.xslots)
        if (s.tech == t) return false;
    return true;
}

float World::tech_days(int n, int t) const {
    float c = techs[t].cost;
    int ahead = techs[t].year - year();
    if (ahead > 0) c *= 1 + 0.5f * ahead;
    return c / std::max(0.2f, 1 + nations[n].mods[M_RESEARCH]);
}

int World::tech_era(int n) const {
    int top[5] = {0, 0, 0, 0, 0};
    for (size_t t = 0; t < techs.size(); t++) {
        if (!nations[n].techs[t]) continue;
        int y = techs[t].year;
        for (int k = 0; k < 5; k++)
            if (y > top[k]) { for (int j = 4; j > k; j--) top[j] = top[j - 1]; top[k] = y; break; }
    }
    return std::max(top[4], 1906);
}

void World::grant_tech(int n, int t) {
    Nation& N = nations[n];
    N.techs[t] = 1;
    if (techs[t].replaces >= 0) N.techs[techs[t].replaces] = 1;
    for (size_t v = 0; v < techs.size(); v++)
        if (techs[v].replaces == t && N.tech_vis[v]) N.techs[v] = 1;
    tech_base(n);
}

// nobody starts with an air base: the tech that opens a kind brings the first one, at the capital
void World::tech_base(int n) {
    Nation& N = nations[n];
    if (N.capital < 0) return;
    Province& cp = provs[N.capital];
    auto knows = [&](const char* tech) { auto it = tech_idx.find(tech); return it != tech_idx.end() && N.techs[it->second]; };
    if (knows("aircraft") && cp.airfield < 1) cp.airfield = 1;
    if (knows("aircraft") && cp.seaplane < 1 && ocean_coast(N.capital)) cp.seaplane = 1;
    if (knows("air_service") && cp.airbase < 1) cp.airbase = 1;
    if (knows("rigid_airships") && cp.airship < 1) cp.airship = 1;
}

// experimental techs only go in experimental slots, and ordinary ones only in ordinary slots
bool World::start_research(int n, int slot, int t, bool experimental) {
    Nation& N = nations[n];
    auto& v = experimental ? N.xslots : N.slots;
    if (slot < 0 || slot >= (int)v.size() || !tech_available(n, t) || techs[t].experimental != experimental) return false;
    v[slot].tech = t;
    v[slot].prog = 0;
    return true;
}

// slots: the nation's own (3 for a great power, 2 otherwise), those granted by effects, those of the science law
void World::update_slots(int n) {
    Nation& N = nations[n];
    static const int LAW_SLOTS[POL_OPTS] = {0, 1, 1, 1}, LAW_XSLOTS[POL_OPTS] = {0, 1, 2, 3};
    int law = N.law[POL_SCIENCE] < 0 ? 0 : N.law[POL_SCIENCE];
    size_t want = (size_t)((N.major ? 3 : 2) + N.extra_slots + LAW_SLOTS[law]);
    size_t xwant = (size_t)((N.major ? 1 : 0) + N.extra_xslots + LAW_XSLOTS[law]);   // great powers have a laboratory from the start
    want = std::min<size_t>(want, 7);
    xwant = std::min<size_t>(xwant, 4);
    if (N.slots.size() != want) N.slots.resize(want);
    if (N.xslots.size() != xwant) N.xslots.resize(xwant);
}

void World::tick_research(int i) {
    Nation& n = nations[i];
    bool unlocked = false;
    for (auto* v : {&n.slots, &n.xslots})
        for (auto& s : *v) {
            if (s.tech < 0) continue;
            float total = tech_days(i, s.tech);
            s.prog += 1;
            if (s.prog >= total) {
                grant_tech(i, s.tech);
                if (i == player) {
                    add_news("Research complete: " + techs[s.tech].name, i);
                    completed.push_back({false, s.tech, day});
                }
                for (auto& c : chassis) unlocked |= c.tech == s.tech;
                for (auto& m : modules) unlocked |= m.tech == s.tech;
                for (auto& b : battalions) unlocked |= b.tech == s.tech;
                s.tech = -1;
                s.prog = 0;
            }
        }
    // new chassis, modules or battalions: standard designs and templates follow (the player can make better ones)
    if (unlocked) {
        refresh_designs(i, i != player);
        update_std_templates(i);
        if (i == player) add_news("New equipment and battalions are available: see the Production and Army tabs.", i);
    }
}

bool World::focus_blocked(int n, int f) const {
    const Nation& N = nations[n];
    for (int m : focuses[f].mutex)
        if (N.focus_done[m] || N.focus == m) return true;
    return false;
}

bool World::focus_available(int n, int f) const {
    const Nation& N = nations[n];
    if (!N.focus_vis[f] || N.focus_done[f] || N.focus == f || focus_blocked(n, f)) return false;
    for (auto& grp : focuses[f].req) {
        bool any = false;
        for (int r : grp) any |= N.focus_done[r] != 0;
        if (!any) return false;
    }
    return !focuses[f].avail || trig(focuses[f].avail, n);
}

bool World::start_focus(int n, int f) {
    Nation& N = nations[n];
    if (N.focus >= 0 || !focus_available(n, f)) return false;
    N.focus = f;
    N.focus_prog = 0;
    return true;
}

void World::complete_focus(int n, int f) {
    Nation& N = nations[n];
    N.focus_done[f] = 1;
    if (N.focus == f) { N.focus = -1; N.focus_prog = 0; }
    eff(focuses[f].effect, n);
    const std::string& b = focuses[f].branch;
    if (n == player || (N.major && (b == "politics" || b == "diplomacy" || b == "antarctica")))
        add_news(fmt("%s completes the focus \"%s\".", N.name.c_str(), focuses[f].name.c_str()), n);
}

bool World::programme_visible(int n, int p) const {
    return !programmes[p].visible || trig(programmes[p].visible, n);
}

bool World::programme_available(int n, int p) const {
    const Nation& N = nations[n];
    return N.prog != p && day >= N.prog_next[p] && programme_visible(n, p) && (!programmes[p].avail || trig(programmes[p].avail, n));
}

float World::programme_days(int n, int p) const {
    return std::max(10.0f, programmes[p].days * (programmes[p].speed ? ai_weight(programmes[p].speed, n, 1) : 1.0f));
}

bool World::start_programme(int n, int p) {
    Nation& N = nations[n];
    if (N.prog >= 0 || !programme_available(n, p)) return false;
    N.prog = p;
    N.prog_done = 0;
    float v = programmes[p].variance;
    N.prog_days = programme_days(n, p) * (float)rng.uni(1 - v, 1 + v);
    return true;
}

void World::tick_programme(int i) {
    Nation& N = nations[i];
    if (N.prog >= 0) {
        if (++N.prog_done < N.prog_days) return;
        int p = N.prog;
        N.prog = -1;
        N.prog_done = 0;
        N.prog_next[p] = day + programmes[p].cooldown;
        N.prog_count[p]++;
        eff(programmes[p].effect, i);
        if (i == player) add_news(fmt("Programme complete: %s.", programmes[p].name.c_str()), i);
    }
    if (N.prog_loop >= 0 && N.prog < 0) start_programme(i, N.prog_loop);
}

int World::unbuild(int n, int prov, int type, bool all) {
    Nation& N = nations[n];
    Province& p = provs[prov];
    if (p.owner != n) return 0;
    int gone = 0;
    for (int k = (int)N.builds.size() - 1; k >= 0 && (all || !gone); k--)
        if (N.builds[k].prov == prov && N.builds[k].type == type) { N.builds.erase(N.builds.begin() + k); gone++; }
    if (gone && !all) return gone;
    int* have = &p.level(type);
    while (*have > 0 && (all || !gone)) { --*have; gone++; }
    recompute(n);
    // lines cannot keep factories that are gone
    int used = 0, mf = (int)floorf(N.mf_total + 0.001f);
    for (auto& l : N.lines) used += l.factories;
    for (int k = (int)N.lines.size() - 1; k >= 0 && used > mf; k--) {
        int cut = std::min(used - mf, N.lines[k].factories);
        N.lines[k].factories -= cut;
        used -= cut;
    }
    map_dirty = true;
    return gone;
}

void World::tick_focus(int i) {
    Nation& n = nations[i];
    if (n.focus < 0) return;
    if (focus_blocked(i, n.focus)) { n.focus = -1; return; }
    n.focus_prog += 1;
    if (n.focus_prog >= focuses[n.focus].days) {
        if (i == player) completed.push_back({true, n.focus, day});
        complete_focus(i, n.focus);
    }
}

bool World::can_build(int n, int prov, int type, std::string* why) const {
    const Province& p = provs[prov];
    auto no = [&](const char* s) { if (why) *why = s; return false; };
    if (p.owner != n || p.controller != n) return no("Not ours.");
    int queued_type = 0;
    for (auto& b : nations[n].builds)
        if (b.prov == prov && b.type == type) queued_type++;
    auto knows = [&](const char* tech) { auto it = tech_idx.find(tech); return it != tech_idx.end() && nations[n].techs[it->second]; };
    if (type == B_CF || type == B_MF) {
        if (p.antarctic) return no("Nothing can be built on the ice.");
        int q = 0;
        for (auto& b : nations[n].builds)
            if (b.prov == prov && (b.type == B_CF || b.type == B_MF)) q++;
        if (p.cf + p.mf + q >= p.slots()) return no("No free slot.");
        return true;
    }
    if (p.level(type) + queued_type >= 5) return no("Already at the highest level.");
    switch (type) {
        case B_INFRA:
            if (p.antarctic) return no("Nothing can be built on the ice.");
            break;
        case B_FORT:
            if (p.antarctic && global_flags.count("antarctic_treaty") && nations[n].flags.count("treaty_member")) return no("The Antarctic Treaty forbids it.");
            break;
        case B_COAST:
            if (!ocean_coast(prov)) return no("Needs a coast on the open sea.");
            if (p.antarctic && global_flags.count("antarctic_treaty") && nations[n].flags.count("treaty_member")) return no("The Antarctic Treaty forbids it.");
            break;
        case B_AIRFIELD:
            if (!knows("aircraft")) return no("Needs research: Heavier-than-Air Flight.");
            break;
        case B_AIRBASE:
            if (!knows("air_service")) return no("Needs research: Military Air Service.");
            break;
        case B_AIRSHIP:
            if (!knows("rigid_airships")) return no("Needs research: Rigid Airships.");
            break;
        case B_SEAPLANE:
            if (!knows("aircraft")) return no("Needs research: Heavier-than-Air Flight.");
            if (!ocean_coast(prov)) return no("Needs a coast on the open sea.");
            break;
        case B_AIRPORT:
            if (!knows("aircraft") || year() < 1919) return no("Airlines start after 1919, with Heavier-than-Air Flight.");
            if (p.antarctic) return no("Nothing can be built on the ice.");
            break;
    }
    return true;
}

float World::build_rate(int n, int prov) const {
    return 10 * 5 * (1 + nations[n].mods[M_CONSTRUCT]) * (0.7f + 0.1f * provs[prov].infra);
}

bool World::queue_build(int n, int prov, int type) {
    if (!can_build(n, prov, type)) return false;
    nations[n].builds.push_back({type, prov, 0});
    return true;
}

bool World::law_allowed(int n, int cons, int econ) const {
    const Nation& N = nations[n];
    bool w = at_war(n);
    if (cons >= 2 && !w && N.war_support < 25) return false;
    if (cons >= 3 && !(w && N.war_support >= 45)) return false;
    if (econ >= 1 && !w && N.war_support < 20) return false;
    if (econ >= 2 && !w && N.war_support < 45) return false;
    return true;
}

bool World::set_law(int n, int cons, int econ, bool free) {
    Nation& N = nations[n];
    cons = clampv(cons, 0, LAW_CONS_N - 1);
    econ = clampv(econ, 0, LAW_ECON_N - 1);
    if (cons == N.law_cons && econ == N.law_econ) return false;
    if (!free && (!law_allowed(n, cons, econ) || N.influence < 50)) return false;
    if (!free) N.influence -= 50;
    if (cons > N.law_cons) N.stability -= 3 * (cons - N.law_cons);
    N.law_cons = cons;
    N.law_econ = econ;
    return true;
}

void World::set_ideology(int n, int ideo) {
    Nation& N = nations[n];
    if (ideo < 0 || ideo == N.ideology) return;
    std::string old = N.display_name();
    N.ideology = ideo;
    N.sub = -1;                         // a new regime brings its own current
    bool mandate = N.campaign == ideo;  // the country was asked first: no shock, and nobody takes up arms
    if (mandate) { N.campaign = -1; N.calm_until = day + CAMPAIGN_CALM_DAYS; }
    if (N.pop[ideo] < 45) {
        float rest = 0;
        for (int k = 0; k < IDEO_N; k++) if (k != ideo) rest += N.pop[k];
        for (int k = 0; k < IDEO_N; k++)
            if (k != ideo) N.pop[k] = N.pop[k] * 55 / std::max(1.0f, rest);
        N.pop[ideo] = 45;
    }
    N.stability = std::max(5.0f, N.stability - (mandate ? 2 : 10));
    // laws the new regime will not keep fall back to its own, and so do ministers
    for (int g = 0; g < POL_N; g++)
        if (!law_ok(n, g, N.law[g])) N.law[g] = default_law(ideo, g);
    fill_cabinet(n);
    const IdeologyPath& P = N.path[ideo];
    std::string who = P.leader.empty() ? P.party : P.leader + " (" + P.party + ")";
    add_news(fmt("%s: %s takes power (%s government).", old.c_str(), who.c_str(), IDEO_NAMES[ideo]), n);
    for (size_t k = 0; k < nations.size(); k++)
        if ((int)k != n && nations[k].exists) add_relation(n, (int)k, nations[k].ideology == ideo ? 10.0f : -5.0f);
    map_dirty = true;
}

bool World::start_campaign(int n, int ideo) {
    Nation& N = nations[n];
    if (ideo < 0 || ideo >= IDEO_N || N.campaign == ideo || N.influence < CAMPAIGN_START || N.pop[ideo] >= CAMPAIGN_CAP) return false;
    N.influence -= CAMPAIGN_START;
    N.campaign = ideo;
    N.campaign_day = day;
    if (n == player) add_news(fmt("We open a political campaign for %s support.", IDEO_NAMES[ideo]), n);
    return true;
}

void World::stop_campaign(int n) { nations[n].campaign = -1; }

int World::campaign_days_to(int n, int ideo, float pct) const {
    float p = nations[n].pop[ideo];
    int days = 0;
    while (p < pct && days < 20000) {       // the same sum tick_politics does: the rate falls as the share grows
        p = (p + CAMPAIGN_RATE) * 100 / (100 + CAMPAIGN_RATE);
        days++;
    }
    return days;
}

void World::tick_decisions(int i) {
    Nation& n = nations[i];
    for (size_t k = 0; k < n.decision_run.size();) {
        if (n.decision_run[k].second <= day) {
            int d = n.decision_run[k].first;
            n.decision_run.erase(n.decision_run.begin() + k);
            eff(decisions[d].effect, i);
            if (i == player) add_news("Decision complete: " + decisions[d].name, i);
            continue;
        }
        k++;
    }
}

bool World::decision_visible(int n, int d) const {
    const Decision& D = decisions[d];
    if (!D.repeatable && nations[n].decision_cd[d] == INT_MAX) return false;
    return !D.visible || trig(D.visible, n);
}

bool World::decision_available(int n, int d) const {
    const Nation& N = nations[n];
    const Decision& D = decisions[d];
    if (!decision_visible(n, d) || N.influence < D.cost) return false;
    if (N.decision_cd[d] > day) return false;
    for (auto& r : N.decision_run)
        if (r.first == d) return false;
    return !D.avail || trig(D.avail, n);
}

bool World::take_decision(int n, int d) {
    if (!decision_available(n, d)) return false;
    Nation& N = nations[n];
    const Decision& D = decisions[d];
    N.influence -= D.cost;
    N.decision_cd[d] = D.repeatable ? day + std::max(D.days, D.cooldown) : INT_MAX;
    if (D.days > 0) N.decision_run.push_back({d, day + D.days});
    else eff(D.effect, n);
    return true;
}

void World::tick_events_monthly() {
    for (size_t e = 0; e < events.size(); e++) {
        Event& E = events[e];
        if (E.triggered_only || day < E.after || day > E.before) continue;
        if (E.global_once && E.fired_any) continue;
        for (size_t n = 0; n < nations.size(); n++) {
            if (!nations[n].exists || (E.once && E.fired[n])) continue;
            if (!trig(E.trigger, (int)n)) continue;
            if (!rng.chance(E.chance / 100.0)) continue;
            fire_event((int)e, (int)n);
            if (E.global_once) break;
        }
    }
}

void World::tick_day() {
    day++;
    compute_weather_all();      // weather.cpp: a function of the date
    const int NN = (int)nations.size();
    {   // the yardstick of wealth: the great powers' average, from today's state
        float sum = 0;
        int k = 0;
        for (int i = 0; i < NN; i++)
            if (nations[i].major && nations[i].exists) { sum += core_wealth(i); k++; }
        if (k) ref_wealth = sum / k;
    }
    for (int i = 0; i < NN; i++) {
        if (!nations[i].exists) continue;
        recompute(i);
        tick_economy(i);
        tick_society(i);
        tick_politics(i);
        tick_research(i);
        tick_focus(i);
        tick_programme(i);
        tick_decisions(i);
        tick_fleets(i);
        if (i != player) ai_daily(i);
    }
    tick_trade();
    tick_intel();
    tick_unrest();              // unrest.cpp: riots, risings, civil wars
    tick_courting();
    // relations slowly return to their old footing unless actively tended
    for (int a = 0; a < NN; a++)
        for (int b = a + 1; b < NN; b++) {
            float& r = nations[a].rel[b];
            float target = base_rel[a][b] + (allied(a, b) ? 25.0f : 0) + (nations[a].ententes.count(b) ? 15.0f : 0) - (war[a][b] ? 50.0f : 0);
            r += (target - r) * 0.001f;
            nations[b].rel[a] = r;
        }
    // occupation ends when the war between occupier and owner does
    for (auto& p : provs) {
        if (p.owner >= 0 && p.controller >= 0 && p.controller != p.owner && !war[p.controller][p.owner]) {
            p.controller = p.owner;
            map_dirty = true;
        }
    }
    tick_orders();
    tick_plans();
    if (day % 30 == 0) tick_commanders_all();
    tick_divisions();           // divisions.cpp: training, replacements, upgrades, experience, adaptation
    tick_military();
    tick_air();
    // what the fighting did to each division's kit counts from now, exactly as it would after a reload
    for (auto& d : divs) if (d.alive) div_stats(d);
    tick_supply();         // daily for nations at war, weekly in peace (terrain.cpp)
    for (int i = 0; i < NN; i++)
        if (nations[i].exists && i != player && i % 7 == day % 7) ai_weekly(i);
    int y, m, d;
    civil_from_days(day + days_from_civil(1906, 1, 1), y, m, d);
    if (d == 1) tick_events_monthly();
    check_capitulation();
}
