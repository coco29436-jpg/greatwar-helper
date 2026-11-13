// Computer-controlled nations: focuses, research, industry, army, diplomacy, trade and fronts.
#include <map>

#include "world.h"

void World::ai_daily(int n) {
    if ((day + n) % 3 == 0 && at_war(n)) ai_military(n);
}

void World::ai_weekly(int n) {
    Nation& N = nations[n];
    bool w = at_war(n);
    ai_cabinet(n);

    // focus
    if (N.focus < 0) {
        int best = -1;
        float bw = 0;
        for (size_t f = 0; f < focuses.size(); f++) {
            if (!focus_available(n, (int)f)) continue;
            float wt = ai_weight(focuses[f].ai, n, 1);
            if (wt < 0.5f && !rng.chance(wt * 0.02f)) continue;     // unlikely paths stay rare
            wt *= (float)rng.uni(0.5, 1.5);
            if (wt > bw) { bw = wt; best = (int)f; }
        }
        if (best >= 0) start_focus(n, best);
    }
    // programme: whichever the moment calls for
    if (N.prog < 0) {
        int best = -1;
        float bw = 0;
        for (size_t p = 0; p < programmes.size(); p++) {
            if (!programme_available(n, (int)p)) continue;
            float wt = ai_weight(programmes[p].ai, n, 1) * (float)rng.uni(0.5, 1.5);
            if (wt > bw) { bw = wt; best = (int)p; }
        }
        if (best >= 0) start_programme(n, best);
    }
    // research: cheapest sensible techs first, ahead-of-time only rarely
    for (size_t s = 0; s < N.slots.size(); s++) {
        if (N.slots[s].tech >= 0) continue;
        int best = -1;
        float bw = 0;
        for (size_t t = 0; t < techs.size(); t++) {
            if (techs[t].experimental || !tech_available(n, (int)t) || techs[t].year > year() + 1) continue;
            float wt = (float)rng.uni(0.5, 1.5) / tech_days(n, (int)t);
            if (w && (techs[t].cat == "infantry" || techs[t].cat == "artillery")) wt *= 1.5f;
            if (wt > bw) { bw = wt; best = (int)t; }
        }
        if (best >= 0) start_research(n, (int)s, best);
    }
    for (size_t s = 0; s < N.xslots.size(); s++) {       // experimental slots: the cheapest experimental tech in reach
        if (N.xslots[s].tech >= 0) continue;
        int best = -1;
        float bw = 0;
        for (size_t t = 0; t < techs.size(); t++) {
            if (!techs[t].experimental || !tech_available(n, (int)t) || techs[t].year > year() + 2) continue;
            float wt = (float)rng.uni(0.5, 1.5) / tech_days(n, (int)t);
            if (wt > bw) { bw = wt; best = (int)t; }
        }
        if (best >= 0) start_research(n, (int)s, best, true);
    }
    ai_society(n);
    // fleet: blockade a weaker enemy, otherwise escort our imports when we depend on them, otherwise guard the coast
    {
        float strongest = 0;
        for (size_t e = 0; e < nations.size(); e++)
            if (war[n][e]) strongest = std::max(strongest, navy((int)e));
        float imports = 0;
        for (int r = 0; r < RES_N; r++) imports += N.res_in[r];
        int mission = w && navy(n) >= 4 && navy(n) > strongest * 1.2f ? FLEET_BLOCKADE : imports >= 8 ? FLEET_TRADE : FLEET_HOME;
        for (auto& f : N.fleets) set_fleet_mission(n, f, mission);
    }
    // espionage: spare influence trains agents; post them against enemies, great powers and rivals,
    // and keep one at home against foreign spies once there is something to hide
    if (N.influence > 80 && agents_total(n) < max_agents(n)) train_agent(n);
    while (N.agents > 0) {
        int best = -1;
        float bs = -1e9f;
        if (N.spies[n] == 0 && (w || N.major)) best = n;
        else
            for (size_t t = 0; t < nations.size(); t++) {
                if ((int)t == n || !nations[t].exists || allied(n, (int)t)) continue;
                if (!war[n][t] && !nations[t].major && relation(n, (int)t) > -30) continue;
                float s = (war[n][t] ? 100.0f : 0.0f) - relation(n, (int)t) + (nations[t].major ? 30.0f : 0.0f) - 15.0f * N.spies[t];
                if (s > bs) { bs = s; best = (int)t; }
            }
        if (best < 0 || !post_agent(n, best, 1)) break;
    }
    // construction
    size_t want_q = N.major ? 4 : 2;
    for (int tries = 0; N.builds.size() < want_q && tries < 6; tries++) {
        float mil_share = w ? 0.75f : (N.war_support > 45 || year() >= 1912 ? 0.45f : 0.25f);
        int type = rng.chance(mil_share) ? B_MF : B_CF;
        if (rng.chance(0.12)) type = B_INFRA;
        if (w && rng.chance(0.1)) type = B_FORT;
        // ports under the guns of a stronger navy get coastal forts; a rich peacetime power flies airlines
        if (rng.chance(w ? 0.1 : 0.02)) {
            float strongest = 0;
            for (size_t e = 0; e < nations.size(); e++) if (war[n][e] || relation(n, (int)e) < -40) strongest = std::max(strongest, navy((int)e));
            if (strongest > navy(n)) type = B_COAST;
        }
        if (!w && N.cf_total > 30 && year() >= 1925 && rng.chance(0.03)) type = B_AIRPORT;
        int best = -1;
        float bs = -1;
        for (size_t p = 0; p < provs.size(); p++) {
            if (!can_build(n, (int)p, type)) continue;
            const Province& P = provs[p];
            float s = (P.status == ST_CORE ? 3.0f : 1.0f) * P.size + (float)rng.uni(0, 1.5);
            if (type == B_FORT) {
                s = 0;
                for (int q : P.adj) if (provs[q].controller >= 0 && war[n][provs[q].controller]) s += 3;
                if (s == 0) continue;
            }
            if (type == B_COAST) s = 1 + (float)P.size * 2 + (P.capital ? 4 : 0) + (float)rng.uni(0, 1);
            if (s > bs) { bs = s; best = (int)p; }
        }
        if (best >= 0) queue_build(n, best, type);
    }
    ai_production(n);
    ai_recruit(n);
    // laws
    if (N.influence > 80) {
        int ec = N.law_econ, co = N.law_cons;
        if (w || N.war_support > 50) { ec = std::min(2, ec + 1); co = std::min(3, co + 1); }
        else if (year() >= 1911 && N.major) ec = std::max(ec, 1);
        while (!law_allowed(n, co, ec) && (co > N.law_cons || ec > N.law_econ)) {
            if (co > N.law_cons) co--;
            else ec--;
        }
        set_law(n, co, ec);
    }
    // decisions
    for (size_t d = 0; d < decisions.size(); d++) {
        if (!decision_available(n, (int)d)) continue;
        float wt = ai_weight(decisions[d].ai, n, 0.3f);
        if (rng.chance(std::min(0.6f, wt * 0.1f))) take_decision(n, (int)d);
    }
    // trade for missing resources
    for (int r = 0; r < RES_N; r++) {
        if (N.res_ratio[r] >= 0.98f || N.cf_free < 2) continue;
        int best = -1;
        float bs = 3.9f;
        for (size_t s = 0; s < nations.size(); s++) {
            if ((int)s == n || !nations[s].exists || war[n][s]) continue;
            float t = tradeable((int)s, r) + relation(n, (int)s) * 0.05f;
            if (t > bs) { bs = t; best = (int)s; }
        }
        if (best >= 0) request_trade(n, best, r, 4);
    }
    ai_air(n);
    ai_diplomacy(n);
    if (!w) {
        // peacetime: bring stray divisions home
        for (auto& d : divs)
            if (d.alive && d.nation == n && d.path.empty() && d.sea_dest < 0 && d.prov >= 0 && provs[d.prov].owner != n && N.capital >= 0 &&
                !order_move(d, N.capital)) {
                unindex_div(d);
                d.prov = N.capital;      // no route home: shipped back by the merchant fleet
                index_div(d);
                d.dig = 0;
            }
    }
}

void World::ai_diplomacy(int n) {
    Nation& N = nations[n];
    const int NN = (int)nations.size();
    bool w = at_war(n);
    // improve relations with the friendliest major we are not allied to
    if (N.influence > 60 && rng.chance(0.3)) {
        int best = -1;
        float bs = -1e9f;
        for (int k = 0; k < NN; k++) {
            if (k == n || !nations[k].exists || !nations[k].major || allied(n, k) || war[n][k]) continue;
            float s = relation(n, k) + (nations[k].ideology == N.ideology ? 20 : 0) + (float)rng.uni(-10, 10);
            if (s > bs && relation(n, k) < 60 && relation(n, k) > -20) { bs = s; best = k; }
        }
        if (best >= 0) improve_relations(n, best);
    }
    // alliance leaders invite friendly nations; majors may found alliances
    if (is_faction_leader(n) && rng.chance(0.04)) {
        for (int k = 0; k < NN; k++) {
            if (k == n || !nations[k].exists || nations[k].faction >= 0 || nations[k].overlord >= 0 || relation(n, k) < 45) continue;
            if (k == player) { if (rng.chance(0.2)) invite_to_faction(n, k); }
            else invite_to_faction(n, k);
            break;
        }
    }
    if (N.major && N.faction < 0 && N.overlord < 0 && !N.flags.count("neutral") && rng.chance(0.03)) {
        for (int k = 0; k < NN; k++)
            if (k != n && nations[k].exists && nations[k].major && relation(n, k) > 55 && nations[k].faction < 0) {
                create_faction(n, N.adj + " League");
                invite_to_faction(n, k);
                break;
            }
    }
    if (N.major && rng.chance(0.05) && N.influence > 60) {
        for (int k = 0; k < NN; k++)
            if (k != n && nations[k].exists && nations[k].major && !N.ententes.count(k) && !allied(n, k) && relation(n, k) > 55 && N.ententes.size() < 2) {
                propose_entente(n, k);
                break;
            }
    }
    // aggressive governments justify and start wars on weaker neighbours
    bool aggressive = N.ideology == IDEO_NAT || (N.ideology == IDEO_AUT && N.pop[IDEO_NAT] > 30);
    if (!w && aggressive && !N.flags.count("neutral") && N.justify_target < 0 && N.wargoals.empty() && N.war_support > 45 && rng.chance(0.02) && N.influence > 60) {
        int best = -1;
        float bs = 0;
        for (auto& p : provs) {
            if (p.owner != n) continue;
            for (int q : p.adj) {
                int t = provs[q].owner;
                if (t < 0 || t == n || allied(n, t) || !nations[t].exists) continue;
                float mine = (float)N.num_divs, theirs = 0;
                for (int b : faction_bloc(t)) theirs += nations[b].num_divs;
                float s = mine / std::max(1.0f, theirs) - relation(n, t) * 0.01f;
                if (s > 1.4f && s > bs) { bs = s; best = t; }
            }
        }
        if (best >= 0) start_justify(n, best);
    }
    auto bloc_strength = [&](int x) {
        float s = 0;
        for (int b : faction_bloc(x)) s += nations[b].num_divs + nations[b].mf_total;
        for (size_t k = 0; k < nations.size(); k++)
            if (nations[k].exists && nations[k].guarantees.count(x) && !allied((int)k, x)) s += nations[k].num_divs * 0.5f;
        return s;
    };
    for (int t : std::vector<int>(N.wargoals.begin(), N.wargoals.end()))
        if (!w && N.war_support > 40 && can_declare_war(n, t) && bloc_strength(n) >= 0.8f * bloc_strength(t) && rng.chance(0.25)) {
            declare_war(n, t);
            break;
        }
    // ask for peace when the war goes badly
    if (w && rng.chance(0.08)) {
        float lost = N.vp_total ? (float)N.vp_lost / N.vp_total : 0;
        bool weary = N.war_support < 20 || (N.war_days > 3 * 365 && N.war_support < 30);
        for (int k = 0; k < NN; k++)
            if (war[n][k] && (lost > 0.2f || weary)) {
                offer_peace(n, k);
                break;
            }
    }
}

void World::ai_military(int n) {
    const int NP = (int)provs.size();
    std::vector<char> front(NP, 0);
    std::vector<int> fronts;
    for (int p = 0; p < NP; p++) {
        int c = provs[p].controller;
        if (c < 0 || !friendly(n, c)) continue;
        for (int q : provs[p].adj) {
            int e = provs[q].controller;
            if (e >= 0 && war[n][e]) { front[p] = 1; break; }
        }
        if (front[p]) fronts.push_back(p);
    }
    // threat: enemy strength next to each front province, so reserves go where the enemy masses
    std::vector<float> enemy_at(NP, 0), threat(NP, 0);
    for (auto& d : divs)
        if (d.alive && d.prov >= 0 && d.sea_days <= 0 && war[n][d.nation]) enemy_at[d.prov] += d.str;
    for (int f : fronts)
        for (int q : provs[f].adj) threat[f] += enemy_at[q];
    std::map<int, int> assigned, holding;
    for (auto& d : divs)
        if (d.alive && d.nation == n && d.prov >= 0) {
            assigned[d.path.empty() ? d.prov : d.path.back()]++;
            if (d.path.empty() && d.attacking < 0) holding[d.prov]++;
        }
    std::map<int, int> kept;     // divisions kept back to hold each front province
    int cap = nations[n].capital;
    bool kept_home = false;
    for (auto& d : divs) {
        if (!d.alive || d.nation != n || d.train > 0 || d.sea_days > 0 || d.attacking >= 0 || !d.path.empty() || d.sea_dest >= 0)
            continue;
        if (!kept_home && d.prov == cap && !front[cap]) { kept_home = true; continue; }
        if (front[d.prov]) {
            // always leave a garrison dug in; only the rest may attack
            if (kept[d.prov] < 1 + (int)(provs[d.prov].vp >= 9)) { kept[d.prov]++; continue; }
            // attack the weakest neighbouring enemy province when we have the edge
            float mine = 0;
            for (int i : divs_in(d.prov))
                if (divs[i].nation == n) mine += div_atk(divs[i], 0.2f) * divs[i].str;
            int best = -1;
            float bs = 1.15f;
            for (int q : provs[d.prov].adj) {
                int e = provs[q].controller;
                if (e < 0 || !war[n][e]) continue;
                float theirs = 0;
                for (int i : divs_in(q))
                    if (war[n][divs[i].nation]) theirs += div_def(divs[i]) * divs[i].str * (1 + provs[q].fort * 0.15f);
                float r = theirs <= 0 ? 99 : mine / theirs + (float)rng.uni(-0.2, 0.2);
                if (r > bs) { bs = r; best = q; }
            }
            if (best >= 0) order_move(d, best);
            continue;
        }
        if (!fronts.empty()) {
            int best = -1;
            float bs = 1e30f;
            for (int f : fronts) {
                // fill each stretch up to the enemy facing it (at least one division), nearest first
                float need = 1 + 0.9f * threat[f];
                float km = haversine_km(provs[d.prov].lat, provs[d.prov].lon, provs[f].lat, provs[f].lon);
                float s = (assigned[f] + 1) / need * (1 + km / 1500.0f);
                if (s < bs) { bs = s; best = f; }
            }
            if (best >= 0 && order_move(d, best)) { assigned[best]++; continue; }
            if (cap >= 0 && provs[d.prov].owner != n && find_path(n, d.prov, cap).empty()) { unindex_div(d); d.prov = cap; index_div(d); d.dig = 0; continue; }
        }
        // no land front: occasionally try a landing on an enemy coast
        if (navy(n) >= 4 && rng.chance(0.05)) {
            int best = -1;
            float bs = 1e30f;
            for (int p = 0; p < NP; p++) {
                int e = provs[p].controller;
                if (e < 0 || !war[n][e] || !provs[p].coastal) continue;
                float s = haversine_km(provs[d.prov].lat, provs[d.prov].lon, provs[p].lat, provs[p].lon) * (1 + divs_in(p).size());
                if (s < bs) { bs = s; best = p; }
            }
            if (best >= 0) order_move(d, best);
        }
    }
}

// Form wings from the stockpile and send them where the fighting is heaviest.
void World::ai_air(int n) {
    Nation& N = nations[n];
    int nw = 0;
    for (auto& a : wings) nw += a.nation == n;
    // a balanced air arm: mostly fighters, some bombers, a few airships; each from the best design in stock
    static const int ORDER[6] = {RO_FIGHTER, RO_TACTICAL, RO_FIGHTER, RO_AIRSHIP, RO_FIGHTER, RO_STRATEGIC};
    for (int k = 0; k < 6 && nw < air_cap(n); k++) {
        Need nd{CAT_AIR, ORDER[(nw + k) % 6], -1, 1, 1};
        int d = best_in_stock(n, nd, wing_size(wing_type_of_role(nd.role)));
        if (d >= 0 && form_wing(n, d)) nw++;
    }
    (void)N;
    if (!at_war(n)) {
        for (auto& a : wings) if (a.nation == n) { a.mission = MIS_IDLE; a.target = -1; }
        return;
    }
    // the hottest front: our province facing the most enemy divisions
    int hot = -1;
    float best = 0;
    for (size_t p = 0; p < provs.size(); p++) {
        if (provs[p].controller != n) continue;
        float e = 0;
        for (int q : provs[p].adj)
            for (int i : divs_in(q)) if (war[n][divs[i].nation]) e += 1;
        if (e > best) { best = e; hot = (int)p; }
    }
    // an enemy industrial province for the bombers
    int factory = -1;
    float fv = 0;
    for (auto& a : wings) {
        if (a.nation != n || a.type == AW_FIGHTER) continue;
        for (size_t p = 0; p < provs.size(); p++) {
            int c = provs[p].controller;
            if (c < 0 || !war[n][c] || !in_range(a, (int)p)) continue;
            float v = (float)(provs[p].cf + provs[p].mf) * (1 - provs[p].bomb);
            if (v > fv) { fv = v; factory = (int)p; }
        }
        break;
    }
    for (auto& a : wings) {
        if (a.nation != n) continue;
        // fly from the matching airfield nearest the front, and if none reaches it, have one built
        if (hot >= 0 && !in_range(a, hot)) {
            int nb = -1, site = -1;
            float bd = 1e30f, sd = 1e30f;
            int need = airfield_for(a.design, a.type);
            for (size_t p = 0; p < provs.size(); p++) {
                if (provs[p].controller != n) continue;
                float d = haversine_km(provs[p].lat, provs[p].lon, provs[hot].lat, provs[hot].lon);
                if (d < bd && can_base(n, a.design, a.type, (int)p, &a)) { bd = d; nb = (int)p; }
                if (d > 60 && d < sd && can_build(n, (int)p, need)) { sd = d; site = (int)p; }   // not right on the line
            }
            if (nb >= 0) a.base = nb;
            AirWing probe = a;
            probe.base = nb;
            if ((nb < 0 || !in_range(probe, hot)) && site >= 0) {
                // in proportion: a few levels more than it has wings, however far its wars spread
                int air_builds = 0, have = 0, own_wings = 0;
                for (auto& q : N.builds) air_builds += q.type >= B_AIRFIELD && q.type <= B_SEAPLANE;
                for (auto& pv : provs) if (pv.owner == n) have += pv.airfield + pv.airbase + pv.airship + pv.seaplane;
                for (auto& o : wings) own_wings += o.alive && o.nation == n;
                if (air_builds < 2 && have + air_builds < own_wings + 3 + 3) queue_build(n, site, need);      // the capital's three don't count
            }
        }
        if (a.type == AW_FIGHTER && hot >= 0) set_mission(a, MIS_SUPERIORITY, hot);
        else if (a.type == AW_BOMBER && hot >= 0 && (factory < 0 || rng.chance(0.6))) set_mission(a, MIS_SUPPORT, hot);
        else if (factory >= 0) set_mission(a, MIS_BOMBING, factory);
    }
}

int World::air_cap(int n) const {
    const Nation& N = nations[n];
    return (N.major ? 3 : 1) + (int)(N.mf_total / (N.major ? 12.0f : 20.0f));
}

// Production: idle military factories go to the most needed equipment, and old designs are retooled to the
// newest of the same kind. Needs: what the divisions lack, a reserve for new divisions, the fleet, the air arm,
// locomotives for the railways and tractors for the farms.
void World::ai_production(int n) {
    Nation& N = nations[n];
    std::vector<const Design*> mine;          // our usable designs: the only ones worth looking through
    for (auto& e : designs)
        if (e.nation == n && !e.obsolete) mine.push_back(&e);
    int mf = (int)floorf(N.mf_total + 0.001f), used = 0;
    // retool lines to the best design of their kind
    for (size_t k = 0; k < N.lines.size(); k++) {
        const Design& d = designs[N.lines[k].design];
        Need nd{chassis[d.chassis].cat, d.role, chassis[d.chassis].weight, 1, 1};
        int best = -1;
        float bs = design_score(d) * 1.1f;
        for (auto* e : mine)
            if (fits(*e, nd) && e->score > bs) { bs = e->score; best = e->id; }
        if (best >= 0) retool_line(n, (int)k, best);
    }
    for (auto& l : N.lines) used += l.factories;
    if (used >= mf) return;
    // what is wanted, by need: the gap in the stockpile against the divisions' and a reserve's needs
    struct Want { Need nd; float share; };
    std::vector<Want> wants;
    std::vector<Need> needs;
    for (auto& d : divs) {
        if (!d.alive || d.nation != n) continue;
        const DivTemplate& T = tmpl_of(d);
        for (size_t k = 0; k < T.needs.size(); k++) {
            float gap = T.needs[k].count - kit_have(d, k);
            auto it = std::find_if(needs.begin(), needs.end(), [&](const Need& o) { return o.same(T.needs[k]); });
            if (it == needs.end()) { needs.push_back(T.needs[k]); needs.back().count = gap + T.needs[k].count * 0.15f; }
            else it->count += gap + T.needs[k].count * 0.15f;
        }
    }
    float land_total = 0;
    for (auto& nd : needs) {
        float lack = std::max(0.0f, nd.count - stock_fitting(n, nd));
        nd.count = lack;
        int best = -1;
        float bs = -1e9f;
        for (auto* e : mine)
            if (fits(*e, nd) && e->score > bs) { bs = e->score; best = e->id; }
        nd.units = best >= 0 ? lack * designs[best].st.v[S_COST] : 0;      // factory-days of work
        land_total += nd.units;
    }
    for (auto& nd : needs)
        if (nd.units > 0) wants.push_back({nd, 0.55f * nd.units / std::max(1.0f, land_total) + 0.05f});
    // always some rifles and guns for new divisions
    wants.push_back({Need{CAT_SMALL_ARMS, RO_INFANTRY, -1, 1, 1}, 0.2f});
    wants.push_back({Need{CAT_ARTILLERY, RO_FIELD, -1, 1, 1}, 0.08f});
    bool coast = false;
    for (auto& p : provs) coast |= p.owner == n && p.coastal;
    if (N.major && coast) {
        wants.push_back({Need{CAT_SHIP, RO_CAPITAL, -1, 1, 1}, 0.05f});
        wants.push_back({Need{CAT_SHIP, RO_DESTROYER, -1, 1, 1}, 0.03f});
        wants.push_back({Need{CAT_SHIP, RO_SUBMARINE, -1, 1, 1}, 0.02f});
        wants.push_back({Need{CAT_SHIP, RO_CRUISER, -1, 1, 1}, 0.02f});
        if (year() > 1925) wants.push_back({Need{CAT_SHIP, RO_CARRIER, -1, 1, 1}, 0.02f});
    }
    int nw = 0;
    for (auto& a : wings) nw += a.nation == n;
    if (nw < air_cap(n)) {
        wants.push_back({Need{CAT_AIR, RO_FIGHTER, -1, 1, 1}, 0.06f});
        wants.push_back({Need{CAT_AIR, RO_TACTICAL, -1, 1, 1}, 0.03f});
        if (N.major) wants.push_back({Need{CAT_AIR, year() < 1925 ? RO_AIRSHIP : RO_STRATEGIC, -1, 1, 1}, 0.015f});
    }
    if (N.rail_ratio < 0.95f) wants.push_back({Need{CAT_TRAIN, RO_LOCOMOTIVE, -1, 1, 1}, 0.05f});
    if (N.farm_mech < 0.8f) wants.push_back({Need{CAT_TRACTOR, RO_TRACTOR, -1, 1, 1}, 0.04f});
    while (used < mf) {
        int pick = -1, pick_design = -1;
        float worst = 1e9f;
        for (size_t k = 0; k < wants.size(); k++) {
            int best = -1;
            float bs = -1e9f;
            for (auto* e : mine)
                if (fits(*e, wants[k].nd) && e->score > bs) { bs = e->score; best = e->id; }
            if (best < 0) continue;
            float have = 0;
            for (auto& l : N.lines) if (fits(designs[l.design], wants[k].nd)) have += l.factories;
            float r = have / (wants[k].share * mf);
            if (r < worst) { worst = r; pick = (int)k; pick_design = best; }
        }
        if (pick < 0) break;
        bool found = false;
        for (auto& l : N.lines)
            if (l.design == pick_design) { l.factories++; found = true; break; }
        if (!found) N.lines.push_back({pick_design, 1});
        used++;
    }
}

// Recruitment: roughly what industry and manpower can keep equipped, mostly infantry, then whatever modern
// kinds of division the nation can field
void World::ai_recruit(int n) {
    Nation& N = nations[n];
    bool w = at_war(n);
    int target = (int)(N.mf_total * (N.major ? 1.1f : 1.4f)) + (N.major ? 8 : 2) + (w ? 6 : 0);
    struct Kind { const char* kind; float weight; };
    const Kind KINDS[] = {{"infantry", 1.0f}, {"armour", 0.18f}, {"mechanised", 0.15f}, {"motorised", 0.12f}, {"assault", 0.15f},
                          {"cavalry", year() < 1925 ? 0.12f : 0.02f}, {"mountain", 0.06f}, {"marine", N.major ? 0.04f : 0.0f}, {"airborne", 0.03f}};
    for (int k = 0; k < 4 && N.num_divs < target; k++) {
        int pick = std_template(n, "infantry");
        for (auto& kd : KINDS) {
            if (kd.weight >= 1) continue;
            int t = std_template(n, kd.kind);
            if (t >= 0 && rng.chance(kd.weight) && can_recruit(n, t)) { pick = t; break; }
        }
        if (pick < 0 || !recruit(n, pick)) break;
        N.num_divs++;
    }
}
