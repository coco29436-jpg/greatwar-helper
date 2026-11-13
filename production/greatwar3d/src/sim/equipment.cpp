// Equipment: chassis and modules from data/equipment/*.txt, battalions from data/battalions.txt, standard
// division templates from data/templates.txt; designs, their statistics, the automatic designer the AI uses
// (and the player can start from), the stockpile and production lines.
#include <algorithm>
#include <cmath>

#include "world.h"

const char* CAT_KEYS[CAT_N] = {"small_arms", "artillery", "vehicle", "motor", "air", "ship", "train", "tractor"};
const char* CAT_LABELS[CAT_N] = {"Infantry weapons", "Artillery", "Armoured vehicles", "Motor vehicles", "Aircraft", "Warships",
                                 "Locomotives", "Farm machinery"};
const RoleInfo ROLES[RO_N] = {
    {"infantry", "Infantry Weapons", "Infantry weapons", CAT_SMALL_ARMS},
    {"field", "Field Gun", "Field guns", CAT_ARTILLERY},
    {"howitzer", "Howitzer", "Howitzers", CAT_ARTILLERY},
    {"at", "Anti-Tank Gun", "Anti-tank guns", CAT_ARTILLERY},
    {"aa", "Anti-Aircraft Gun", "Anti-aircraft guns", CAT_ARTILLERY},
    {"rocket", "Rocket Artillery", "Rocket artillery", CAT_ARTILLERY},
    {"tank", "Tank", "Tanks", CAT_VEHICLE},
    {"td", "Tank Destroyer", "Tank destroyers", CAT_VEHICLE},
    {"spg", "Self-Propelled Gun", "Self-propelled guns", CAT_VEHICLE},
    {"spaag", "Self-Propelled Anti-Air", "Self-propelled anti-air", CAT_VEHICLE},
    {"armoured_car", "Armoured Car", "Armoured cars", CAT_VEHICLE},
    {"apc", "Armoured Personnel Carrier", "Personnel carriers", CAT_VEHICLE},
    {"ifv", "Infantry Fighting Vehicle", "Infantry fighting vehicles", CAT_VEHICLE},
    {"truck", "Truck", "Trucks", CAT_MOTOR},
    {"fighter", "Fighter", "Fighters", CAT_AIR},
    {"cas", "Ground-Attack Aircraft", "Ground-attack aircraft", CAT_AIR},
    {"tactical", "Tactical Bomber", "Tactical bombers", CAT_AIR},
    {"strategic", "Strategic Bomber", "Strategic bombers", CAT_AIR},
    {"naval_bomber", "Naval Bomber", "Naval bombers", CAT_AIR},
    {"airship", "Airship", "Airships", CAT_AIR},
    {"helicopter", "Helicopter", "Helicopters", CAT_AIR},
    {"destroyer", "Destroyer", "Destroyers", CAT_SHIP},
    {"cruiser", "Cruiser", "Cruisers", CAT_SHIP},
    {"capital", "Capital Ship", "Capital ships", CAT_SHIP},
    {"carrier", "Aircraft Carrier", "Aircraft carriers", CAT_SHIP},
    {"submarine", "Submarine", "Submarines", CAT_SHIP},
    {"frigate", "Frigate", "Frigates", CAT_SHIP},
    {"locomotive", "Locomotive", "Locomotives", CAT_TRAIN},
    {"armoured_train", "Armoured Train", "Armoured trains", CAT_TRAIN},
    {"tractor", "Tractor", "Tractors and harvesters", CAT_TRACTOR},
};
const char* WEIGHT_KEYS[WT_N] = {"", "light", "medium", "heavy", "superheavy", "mbt"};
const char* WEIGHT_LABELS[WT_N] = {"", "Light", "Medium", "Heavy", "Super-heavy", "Main Battle"};
const char* STAT_KEYS[S_N] = {"soft", "hard", "air", "defense", "breakthrough", "armor", "pierce", "hardness", "speed",
                              "reliability", "cost", "range", "agility", "ground", "bombing", "naval", "torpedo", "sub_attack",
                              "aa", "hp", "supply", "farm", "recon", "stealth", "capacity"};
const char* STAT_LABELS[S_N] = {"Soft attack", "Hard attack", "Air attack", "Defence", "Breakthrough", "Armour", "Piercing",
                                "Hardness", "Speed", "Reliability", "Production cost", "Range", "Agility", "Ground attack",
                                "Bombing", "Naval attack", "Torpedo attack", "Anti-submarine", "Anti-air", "Hull / airframe",
                                "Supply capacity", "Farm output", "Reconnaissance", "Stealth", "Aircraft carried"};
const char* ABIL_KEYS[AB_N] = {"dig", "attrition", "casualties", "org_regen", "suppression", "recon", "supply", "river"};
const char* ABIL_LABELS[AB_N] = {"Entrenchment speed", "Attrition", "Casualties", "Organisation recovery", "Riot suppression",
                                 "Reconnaissance", "Supply use", "River crossing"};

namespace {
int key_index(const char* const* keys, int n, const std::string& s) {
    for (int i = 0; i < n; i++) if (s == keys[i]) return i;
    return -1;
}
int role_index(const std::string& s) {
    for (int r = 0; r < RO_N; r++) if (s == ROLES[r].key) return r;
    return -1;
}
void read_stats(const Node* n, EqStats& st, World& w, const std::string& who) {
    if (!n) return;
    for (const Node& c : n->kids) {
        int s = key_index(STAT_KEYS, S_N, c.key);
        if (s >= 0) st.v[s] += (float)c.numval();
        else w.warnings.push_back(who + ": unknown stat " + c.key);
    }
}
void read_res(const Node* n, EqStats& st) {
    if (!n) return;
    st.res[0] += (float)n->num("coal", 0);
    st.res[1] += (float)n->num("iron", 0);
    st.res[2] += (float)n->num("oil", 0);
}
int tech_of(World& w, const Node& b, const std::string& who) {
    std::string t = b.str("tech");
    if (t.empty()) return -1;
    if (w.cut_techs.count(t)) return -2;          // past the end of the content: the caller drops this item
    auto it = w.tech_idx.find(t);
    if (it == w.tech_idx.end()) { w.warnings.push_back(who + ": unknown tech " + t); return -1; }
    return it->second;
}
Need read_need(const Node& c, World& w, const std::string& who) {
    Need nd;
    nd.cat = key_index(CAT_KEYS, CAT_N, c.str("cat"));
    if (nd.cat < 0) { w.warnings.push_back(who + ": unknown equipment category " + c.str("cat")); nd.cat = 0; }
    nd.role = c.has("role") ? role_index(c.str("role")) : -1;
    nd.weight = c.has("weight") ? key_index(WEIGHT_KEYS, WT_N, c.str("weight")) : -1;
    nd.count = (float)c.num("count", 1);
    nd.units = 1;
    return nd;
}
// what each role is for: how much each statistic is worth when choosing modules or the best design in stock
float role_weight(int role, int s) {
    switch (role) {
        case RO_INFANTRY: { static const float W[S_N] = {1, 1.2f, 0.5f, 1, 0.6f, 0, 0.02f, 0, 0, 2, -0.6f}; return W[s]; }
        case RO_FIELD: case RO_HOWITZER: case RO_ROCKET: return s == S_SOFT ? 1 : s == S_HARD ? 0.4f : s == S_DEF ? 0.4f : s == S_SPEED ? 0.02f : s == S_RELIAB ? 1 : s == S_COST ? -0.08f : 0;
        case RO_AT: case RO_TD: return s == S_HARD ? 1.2f : s == S_PIERCE ? 0.01f : s == S_SOFT ? 0.2f : s == S_ARMOR ? 0.005f : s == S_SPEED ? 0.02f : s == S_RELIAB ? 1 : s == S_COST ? -0.05f : 0;
        case RO_AA: case RO_SPAAG: return s == S_AIR ? 1.2f : s == S_SOFT ? 0.2f : s == S_SPEED ? 0.02f : s == S_RELIAB ? 1 : s == S_COST ? -0.05f : 0;
        case RO_SPG: return s == S_SOFT ? 1.2f : s == S_BRK ? 0.3f : s == S_SPEED ? 0.03f : s == S_RELIAB ? 1 : s == S_COST ? -0.04f : 0;
        case RO_TANK: case RO_ARMOURED_CAR: case RO_APC: case RO_IFV:
            return s == S_SOFT ? 0.8f : s == S_HARD ? 0.8f : s == S_BRK ? 0.8f : s == S_DEF ? 0.5f : s == S_ARMOR ? 0.01f : s == S_PIERCE ? 0.005f
                 : s == S_SPEED ? 0.04f : s == S_RELIAB ? 2 : s == S_RECON ? 0.5f : s == S_COST ? -0.03f : 0;
        case RO_TRUCK: return s == S_SUPPLY ? 1 : s == S_SPEED ? 0.03f : s == S_RELIAB ? 1 : s == S_COST ? -0.2f : 0;
        case RO_FIGHTER: return s == S_AIR ? 1 : s == S_AGILITY ? 0.02f : s == S_SPEED ? 0.002f : s == S_RANGE ? 0.0005f : s == S_HP ? 0.05f : s == S_RELIAB ? 1 : s == S_COST ? -0.02f : 0;
        case RO_CAS: case RO_HELICOPTER: return s == S_GROUND ? 1.2f : s == S_AIR ? 0.3f : s == S_HP ? 0.06f : s == S_AGILITY ? 0.005f : s == S_RELIAB ? 1 : s == S_COST ? -0.02f : 0;
        case RO_TACTICAL: return s == S_BOMB ? 0.8f : s == S_GROUND ? 0.7f : s == S_RANGE ? 0.0008f : s == S_HP ? 0.05f : s == S_RELIAB ? 1 : s == S_COST ? -0.02f : 0;
        case RO_STRATEGIC: case RO_AIRSHIP: return s == S_BOMB ? 1.2f : s == S_RANGE ? 0.001f : s == S_HP ? 0.04f : s == S_RELIAB ? 1 : s == S_COST ? -0.015f : 0;
        case RO_NAVAL_BOMBER: return s == S_NAVAL ? 1 : s == S_TORP ? 1 : s == S_RANGE ? 0.0008f : s == S_HP ? 0.05f : s == S_RELIAB ? 1 : s == S_COST ? -0.02f : 0;
        case RO_DESTROYER: case RO_FRIGATE:
            return s == S_TORP ? 0.8f : s == S_NAVAL ? 0.8f : s == S_SUBATK ? 0.8f : s == S_AA ? 0.6f : s == S_HP ? 0.03f : s == S_SPEED ? 0.03f : s == S_RELIAB ? 1 : s == S_COST ? -0.01f : 0;
        case RO_CRUISER: case RO_CAPITAL:
            return s == S_NAVAL ? 1 : s == S_TORP ? 0.3f : s == S_AA ? 0.5f : s == S_HP ? 0.03f : s == S_ARMOR ? 0.01f : s == S_SPEED ? 0.03f : s == S_RELIAB ? 1 : s == S_COST ? -0.004f : 0;
        case RO_CARRIER: return s == S_CAPACITY ? 0.2f : s == S_AA ? 0.5f : s == S_HP ? 0.03f : s == S_SPEED ? 0.03f : s == S_RELIAB ? 1 : s == S_COST ? -0.004f : 0;
        case RO_SUBMARINE: return s == S_TORP ? 1 : s == S_STEALTH ? 0.03f : s == S_RANGE ? 0.0002f : s == S_HP ? 0.03f : s == S_RELIAB ? 1 : s == S_COST ? -0.02f : 0;
        case RO_LOCOMOTIVE: case RO_ARMOURED_TRAIN: return s == S_SUPPLY ? 1 : s == S_SPEED ? 0.005f : s == S_RELIAB ? 1 : s == S_COST ? -0.1f : 0;
        case RO_TRACTOR: return s == S_FARM ? 1 : s == S_RELIAB ? 0.5f : s == S_COST ? -0.2f : 0;
    }
    return 0;
}
// a design's role shifts its values: a tank destroyer trades soft attack and the turret for a heavier gun
void role_mult(int role, float* m) {
    for (int s = 0; s < S_N; s++) m[s] = 1;
    switch (role) {
        case RO_TD: m[S_SOFT] = 0.5f; m[S_HARD] = 1.35f; m[S_PIERCE] = 1.2f; m[S_DEF] = 0.8f; m[S_BRK] = 0.6f; m[S_COST] = 0.85f; break;
        case RO_SPG: m[S_SOFT] = 1.9f; m[S_HARD] = 0.5f; m[S_BRK] = 0.5f; m[S_ARMOR] = 0.6f; m[S_DEF] = 0.6f; m[S_PIERCE] = 0.6f; m[S_COST] = 0.9f; break;
        case RO_SPAAG: m[S_SOFT] = 0.4f; m[S_HARD] = 0.3f; m[S_BRK] = 0.4f; m[S_PIERCE] = 0.4f; m[S_ARMOR] = 0.7f; m[S_COST] = 0.8f; break;
        case RO_CAS: m[S_GROUND] = 1.4f; m[S_AIR] = 0.7f; m[S_HP] = 1.2f; m[S_AGILITY] = 0.8f; break;
        case RO_NAVAL_BOMBER: m[S_GROUND] = 0.4f; m[S_RANGE] = 1.2f; break;
        case RO_TACTICAL: m[S_RANGE] = 1.1f; break;
        case RO_STRATEGIC: m[S_BOMB] = 1.4f; m[S_GROUND] = 0.5f; m[S_RANGE] = 1.5f; m[S_AGILITY] = 0.6f; m[S_COST] = 1.3f; break;
        default: break;
    }
}
}  // namespace

// ---------------------------------------------------------------------------------------------------------
bool World::load_equipment(const std::string& dir) {
    for (auto& file : files_in(dir + "/equipment"))
        for (const Node& b : load_doc(*this, file)->kids) {
            std::string who = "equipment " + b.key;
            int cat = key_index(CAT_KEYS, CAT_N, b.str("cat"));
            if (cat < 0) { warnings.push_back(who + ": unknown category " + b.str("cat")); continue; }
            if (b.has("slots")) {
                Chassis c;
                c.id = b.key;
                c.name = b.str("name", b.key);
                c.desc = b.str("desc");
                c.cat = cat;
                c.weight = b.has("weight") ? std::max(0, key_index(WEIGHT_KEYS, WT_N, b.str("weight"))) : WT_NONE;
                for (auto& r : words(b.get("roles"))) {
                    int ro = role_index(r);
                    if (ro < 0 || ROLES[ro].cat != cat) warnings.push_back(who + ": role " + r + " does not fit the category");
                    else c.roles.push_back(ro);
                }
                if (c.roles.empty()) { warnings.push_back(who + ": no roles"); continue; }
                c.tech = tech_of(*this, b, who);
                if (c.tech == -2 || (c.tech < 0 && (int)b.num("year", 1900) > CONTENT_END_YEAR)) continue;
                c.year = c.tech >= 0 ? techs[c.tech].year : (int)b.num("year", 1900);
                c.slots = words(b.get("slots"));
                read_stats(b.get("stats"), c.base, *this, who);
                read_res(b.get("res"), c.base);
                chassis_idx[c.id] = (int)chassis.size();
                chassis.push_back(c);
            } else {
                Module m;
                m.id = b.key;
                m.name = b.str("name", b.key);
                m.desc = b.str("desc");
                m.cat = cat;
                m.slot = b.str("slot");
                if (m.slot.empty()) { warnings.push_back(who + ": needs `slots` (a chassis) or `slot` (a module)"); continue; }
                m.tech = tech_of(*this, b, who);
                if (m.tech == -2) continue;
                read_stats(b.get("stats"), m.add, *this, who);
                read_res(b.get("res"), m.add);
                if (const Node* mu = b.get("mult"))
                    for (const Node& c : mu->kids) {
                        int s = key_index(STAT_KEYS, S_N, c.key);
                        if (s >= 0) m.mult[s] = (float)c.numval();
                    }
                for (auto& r : words(b.get("roles"))) {
                    int ro = role_index(r);
                    if (ro >= 0) m.only.push_back(ro);
                }
                module_idx[m.id] = (int)modules.size();
                modules.push_back(m);
            }
        }
    for (const Node& b : load_doc(*this, dir + "/battalions.txt")->kids) {
        std::string who = "battalion " + b.key;
        Battalion bt;
        bt.id = b.key;
        bt.name = b.str("name", b.key);
        bt.short_name = b.str("short", bt.name);
        bt.icon = b.str("icon", "infantry");
        bt.desc = b.str("desc");
        bt.support = b.yes("support");
        bt.tech = tech_of(*this, b, who);
        if (bt.tech == -2) continue;
        bt.mp = (float)b.num("manpower", 1);
        bt.width = (float)b.num("width", bt.support ? 0 : 2);
        bt.org = (float)b.num("org", 30);
        bt.speed = (float)b.num("speed", 4);
        bt.train = (float)b.num("train", 60);
        bt.horses = (float)b.num("horses", 0);
        EqStats tmp;
        read_stats(b.get("stats"), tmp, *this, who);
        for (int s = 0; s < S_N; s++) bt.base[s] = tmp.v[s];
        if (const Node* t = b.get("terrain"))
            for (const Node& c : t->kids) {
                int k = -1;
                for (int q = 0; q < TER_N; q++) {
                    std::string nm = TERRAIN_NAMES[q];
                    for (char& ch : nm) ch = (char)tolower((unsigned char)ch);
                    if (nm == c.key) k = q;
                }
                if (k >= 0) bt.terrain[k] = (float)c.numval();
                else warnings.push_back(who + ": unknown terrain " + c.key);
            }
        if (const Node* a = b.get("abilities"))
            for (const Node& c : a->kids) {
                int k = key_index(ABIL_KEYS, AB_N, c.key);
                if (k >= 0) bt.abil[k] = (float)c.numval();
                else warnings.push_back(who + ": unknown ability " + c.key);
            }
        for (const Node* nd : b.all("need")) bt.need.push_back(read_need(*nd, *this, who));
        battalion_idx[bt.id] = (int)battalions.size();
        battalions.push_back(bt);
    }
    for (const Node& b : load_doc(*this, dir + "/templates.txt")->kids) {
        DivTemplate t;
        t.name = b.str("name", b.key);
        t.kind = b.str("kind", "infantry");
        // a standard template built on a battalion past the end of the content is left out
        bool cut = false;
        for (const char* list : {"line", "support"})
            for (auto& wd : words(b.get(list))) cut |= !battalion_idx.count(wd.substr(0, wd.find('*')));
        if (cut) continue;
        auto add = [&](const Node* list, std::vector<int>& out) {
            for (auto& wd : words(list)) {
                size_t star = wd.find('*');
                std::string id = wd.substr(0, star);
                int count = star == std::string::npos ? 1 : atoi(wd.c_str() + star + 1);
                auto it = battalion_idx.find(id);
                if (it == battalion_idx.end()) { warnings.push_back("template " + b.key + ": unknown battalion " + id); continue; }
                for (int k = 0; k < count; k++) out.push_back(it->second);
            }
        };
        add(b.get("line"), t.line);
        add(b.get("support"), t.support);
        template_stats(t);
        std_templates.push_back(t);
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------
void World::design_stats(Design& d) const {
    const Chassis& c = chassis[d.chassis];
    d.st = c.base;
    float mult[S_N];
    for (float& m : mult) m = 1;
    for (int m : d.mods) {
        if (m < 0) continue;
        const Module& M = modules[m];
        for (int s = 0; s < S_N; s++) { d.st.v[s] += M.add.v[s]; mult[s] *= M.mult[s]; }
        for (int r = 0; r < 3; r++) d.st.res[r] += M.add.res[r];
    }
    float rm[S_N];
    role_mult(d.role, rm);
    for (int s = 0; s < S_N; s++) d.st.v[s] *= mult[s] * rm[s];
    d.st.v[S_RELIAB] = clampv(d.st.v[S_RELIAB], 0.3f, 0.99f);
    d.st.v[S_HARDNESS] = clampv(d.st.v[S_HARDNESS], 0.0f, 0.98f);
    d.st.v[S_COST] = std::max(0.1f, d.st.v[S_COST]);
    float sc = 0;
    for (int k = 0; k < S_N; k++) sc += d.st.v[k] * role_weight(d.role, k);
    d.score = sc;
}

bool World::chassis_ok(int n, int c) const { return chassis[c].tech < 0 || nations[n].techs[chassis[c].tech]; }
bool World::module_ok(int n, int m) const { return modules[m].tech < 0 || nations[n].techs[modules[m].tech]; }
bool World::battalion_ok(int n, int b) const { return battalions[b].tech < 0 || nations[n].techs[battalions[b].tech]; }

bool World::module_fits(int m, int chassis_i, int slot, int role) const {
    const Module& M = modules[m];
    const Chassis& C = chassis[chassis_i];
    if (M.cat != C.cat || slot < 0 || slot >= (int)C.slots.size() || M.slot != C.slots[slot]) return false;
    return M.only.empty() || std::find(M.only.begin(), M.only.end(), role) != M.only.end();
}

float World::design_score(const Design& d) const { return d.score; }

std::string World::design_kind(const Design& d) const {
    const Chassis& c = chassis[d.chassis];
    // warships go by their traditional class names
    if (c.cat == CAT_SHIP) {
        if (d.role == RO_CAPITAL) return c.weight == WT_MEDIUM ? "Battlecruiser" : "Battleship";
        if (d.role == RO_CRUISER) return c.weight == WT_LIGHT ? "Light Cruiser" : "Heavy Cruiser";
        if (d.role == RO_CARRIER) return c.weight == WT_LIGHT ? "Escort Carrier" : "Aircraft Carrier";
        return ROLES[d.role].label;
    }
    std::string w = c.cat == CAT_VEHICLE || c.cat == CAT_SHIP ? WEIGHT_LABELS[c.weight] : "";
    return (w.empty() ? "" : w + " ") + ROLES[d.role].label;
}

int World::create_design(int n, int ch, int role, const std::vector<int>& mods, const std::string& name) {
    Design d;
    d.id = (int)designs.size();
    d.nation = n;
    d.chassis = ch;
    d.role = role;
    d.year = year();
    d.mods = mods;
    d.mods.resize(chassis[ch].slots.size(), -1);
    for (size_t s = 0; s < d.mods.size(); s++)
        if (d.mods[s] >= 0 && (!module_fits(d.mods[s], ch, (int)s, role) || (n >= 0 && !module_ok(n, d.mods[s])))) d.mods[s] = -1;
    design_stats(d);
    if (!name.empty()) d.name = name;
    else {
        // the first design on a chassis takes the nation's own name for the technology, if it has one
        const Chassis& c = chassis[ch];
        std::string nm;
        if (n >= 0 && c.tech >= 0) {
            for (size_t v = 0; v < techs.size(); v++)
                if (techs[v].replaces == c.tech && techs[v].tag == nations[n].tag) nm = techs[v].name;
            for (auto& o : designs)
                if (o.nation == n && o.chassis == ch && o.role == role) { nm.clear(); break; }
        }
        d.name = !nm.empty() ? nm : fmt("%s %s M%d", n >= 0 ? nations[n].adj.c_str() : "", design_kind(d).c_str(), d.year);
    }
    designs.push_back(d);
    return d.id;
}

// the best design this nation can make on a chassis, for a role: the module for each slot that adds most
std::vector<int> World::best_modules(int n, int ch, int role) const {
    const Chassis& C = chassis[ch];
    std::vector<int> pick(C.slots.size(), -1);
    for (size_t s = 0; s < C.slots.size(); s++) {
        float best = 0;
        for (size_t m = 0; m < modules.size(); m++) {
            if (!module_fits((int)m, ch, (int)s, role) || !module_ok(n, (int)m)) continue;
            Design t;
            t.chassis = ch;
            t.role = role;
            t.mods = pick;
            t.mods[s] = (int)m;
            design_stats(t);
            Design base = t;
            base.mods[s] = -1;
            design_stats(base);
            float gain = design_score(t) - design_score(base);
            if (gain > best) { best = gain; pick[s] = (int)m; }
        }
    }
    return pick;
}

// a new design when the best the nation can make now beats the best it has by a margin; returns its id or -1
int World::auto_design(int n, int ch, int role) {
    if (!chassis_ok(n, ch)) return -1;
    Design t;
    t.chassis = ch;
    t.role = role;
    t.mods = best_modules(n, ch, role);
    design_stats(t);
    float have = -1e9f;
    for (auto& d : designs)
        if (d.nation == n && !d.obsolete && d.role == role && chassis[d.chassis].weight == chassis[ch].weight && chassis[d.chassis].cat == chassis[ch].cat)
            have = std::max(have, design_score(d));
    if (design_score(t) <= have * 1.05f + 0.01f) return -1;
    return create_design(n, ch, role, t.mods);
}

// every role and weight class the nation can build, at its best: run at the start, after research, and by the AI
void World::refresh_designs(int n, bool all_roles) {
    for (size_t c = 0; c < chassis.size(); c++) {
        if (!chassis_ok(n, (int)c)) continue;
        // only the newest chassis of each category, weight and role is worth a design
        for (int role : chassis[c].roles) {
            if (!all_roles && role != chassis[c].roles[0] && chassis[c].cat != CAT_AIR && chassis[c].cat != CAT_VEHICLE) continue;
            bool newer = false;
            for (size_t o = 0; o < chassis.size(); o++)
                if (o != c && chassis_ok(n, (int)o) && chassis[o].cat == chassis[c].cat && chassis[o].weight == chassis[c].weight &&
                    chassis[o].year > chassis[c].year && std::find(chassis[o].roles.begin(), chassis[o].roles.end(), role) != chassis[o].roles.end())
                    newer = true;
            if (!newer) auto_design(n, (int)c, role);
        }
    }
    // older designs of a role step back once a better one exists
    for (auto& d : designs) {
        if (d.nation != n || d.obsolete) continue;
        for (auto& e : designs)
            if (e.nation == n && !e.obsolete && e.id != d.id && e.role == d.role && chassis[e.chassis].weight == chassis[d.chassis].weight &&
                chassis[e.chassis].cat == chassis[d.chassis].cat && design_score(e) > design_score(d) * 1.25f) {
                d.obsolete = true;
                break;
            }
    }
}

bool World::fits(const Design& d, const Need& nd) const {
    const Chassis& c = chassis[d.chassis];
    return c.cat == nd.cat && (nd.role < 0 || d.role == nd.role) && (nd.weight < 0 || c.weight == nd.weight);
}

float World::stock_of(int n, int design) const {
    auto it = nations[n].stock.find(design);
    return it == nations[n].stock.end() ? 0 : it->second;
}

void World::add_stock(int n, int design, float v) {
    float& s = nations[n].stock[design];
    s = std::max(0.0f, s + v);
}

// the best design of a need's kind with at least `min` pieces in the stockpile, or -1
int World::best_in_stock(int n, const Need& nd, float min) const {
    int best = -1;
    float bs = -1e9f;
    for (auto& [id, cnt] : nations[n].stock) {
        if (cnt < min) continue;
        const Design& d = designs[id];
        if (!fits(d, nd)) continue;
        float s = design_score(d);
        if (s > bs) { bs = s; best = id; }
    }
    return best;
}

float World::stock_fitting(int n, const Need& nd) const {
    float t = 0;
    for (auto& [id, cnt] : nations[n].stock)
        if (fits(designs[id], nd)) t += cnt;
    return t;
}

// naval strength in "warships of 1906": what the old game counted
float World::ship_power(const Design& d) const {
    const float* v = d.st.v;
    return (v[S_NAVAL] + 0.6f * v[S_TORP] + 0.3f * v[S_SUBATK] + 0.4f * v[S_AA] + 0.02f * v[S_HP] + 0.03f * v[S_CAPACITY]) / 1.3f;
}

float World::navy(int n) const { return nations[n].navy_pow; }

void World::compute_navy(int n) {
    Nation& N = nations[n];
    float p = 0;
    for (auto& [id, cnt] : N.stock)
        if (chassis[designs[id].chassis].cat == CAT_SHIP) p += cnt * ship_power(designs[id]);
    N.navy_pow = p;
}

// ---------------------------------------------------------------------------------------------------------
// production lines build designs; civilian equipment (locomotives, tractors, trucks) is counted against what
// the country needs: railways to run, fields to work, a motorised army to move
void World::tick_production(int i) {
    Nation& n = nations[i];
    int mf = (int)floorf(n.mf_total + 0.001f), used = 0;
    for (auto& l : n.lines) {
        if (l.design < 0 || l.design >= (int)designs.size()) l.factories = 0;
        l.factories = std::max(0, std::min(l.factories, mf - used));
        used += l.factories;
    }
    float cap = std::min(1.0f, 0.5f + n.mods[M_PROD_CAP]);
    for (auto& l : n.lines) {
        if (l.factories <= 0) continue;
        const Design& d = designs[l.design];
        float rr = 1;
        for (int r = 0; r < 3; r++)
            if (d.st.res[r] > 0) rr = std::min(rr, n.res_ratio[r]);
        int cat = chassis[d.chassis].cat;
        float bonus = 1 + n.mods[M_FACTORY] + (cat == CAT_SHIP ? n.mods[M_NAVAL] : 0) + (cat == CAT_AIR ? n.mods[M_AIR] : 0);
        add_stock(i, l.design, l.factories * l.eff * bonus * (0.25f + 0.75f * rr) / d.st.v[S_COST]);
        // know-how grows fastest on a new line and levels off near the cap
        l.eff = std::min(cap, l.eff + 0.002f + 0.006f * std::max(0.0f, cap - l.eff));
    }
    n.lines.erase(std::remove_if(n.lines.begin(), n.lines.end(), [](const ProdLine& l) { return l.factories <= 0; }), n.lines.end());
}

// switching a line to another design: same chassis keeps most of the know-how
void World::retool_line(int n, int line, int design) {
    ProdLine& l = nations[n].lines[line];
    if (l.design == design) return;
    // the same chassis keeps most of the know-how, the same kind of equipment some of it
    const Design* old = l.design >= 0 ? &designs[l.design] : nullptr;
    bool same = old && old->chassis == designs[design].chassis;
    bool kin = old && chassis[old->chassis].cat == chassis[designs[design].chassis].cat && old->role == designs[design].role;
    l.design = design;
    l.eff = same ? l.eff * 0.85f : kin ? std::max(0.15f, l.eff * 0.5f) : 0.15f;
}

// railways, farms and lorries: how well stocked the country is (derived, in recompute)
void World::compute_civil_equipment(int i) {
    Nation& N = nations[i];
    float rails = 0, farms = 0;
    for (auto& p : provs)
        if (p.owner == i && p.controller == i && !p.antarctic) {
            rails += p.infra * (0.5f + p.size) * p.rail_weight();
            farms += p.res[R_GRAIN] + p.res[R_LIVESTOCK];
        }
    float loco = 0, tract = 0, trucks = 0;
    for (auto& [id, cnt] : N.stock) {
        const Design& d = designs[id];
        int cat = chassis[d.chassis].cat;
        if (cat == CAT_TRAIN) loco += cnt * d.st.v[S_SUPPLY];
        else if (cat == CAT_TRACTOR) tract += cnt * d.st.v[S_FARM];
        else if (cat == CAT_MOTOR) trucks += cnt * d.st.v[S_SUPPLY];
    }
    N.rail_ratio = rails > 0 ? std::min(1.5f, loco / (rails * 2)) : 1;
    N.farm_mech = farms > 0 ? std::min(1.0f, tract / (farms * 3)) : 0;
    N.truck_supply = std::min(1.0f, trucks / (std::max(1, N.num_divs) * 20.0f));
}
