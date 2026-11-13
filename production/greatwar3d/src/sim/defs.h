// Enums, name tables, modifiers, terrain and division templates: the vocabulary of the simulation.
// world.h has the state (provinces, nations, divisions...) and the World class.
#pragma once
#include <string>

#include "../core/util.h"

// The last year the game has content for. Techs, focuses, events, decisions, programmes and equipment dated later stay
// in the data files but are not loaded (World::load). Raise this to bring them back.
constexpr int CONTENT_END_YEAR = 1920;

enum { IDEO_AUT, IDEO_CON, IDEO_SOC, IDEO_NAT, IDEO_N };
enum { R_COAL, R_IRON, R_OIL, R_GRAIN, R_LIVESTOCK, RES_N };
enum { AW_FIGHTER, AW_BOMBER, AW_AIRSHIP, AW_N };
enum { MIS_IDLE, MIS_SUPERIORITY, MIS_SUPPORT, MIS_BOMBING, MIS_N };
enum { ST_CORE, ST_COLONY, ST_PROT, ST_CONC, ST_N };
enum { TER_PLAINS, TER_FOREST, TER_HILLS, TER_MOUNTAIN, TER_DESERT, TER_JUNGLE, TER_ARCTIC, TER_N };
enum { B_CF, B_MF, B_INFRA, B_FORT, B_COAST, B_AIRFIELD, B_AIRBASE, B_AIRSHIP, B_SEAPLANE, B_AIRPORT, B_N };
constexpr int WINGS_PER_LEVEL = 2;       // air wings a level of airfield can hold (a capital holds any number)
// standing orders for divisions (orders.cpp): kept until the player gives a plain move order
enum { ORD_NONE, ORD_HOLD, ORD_FRONT, ORD_ADVANCE, ORD_PLAN, ORD_N };
enum { PLAN_NONE, PLAN_FRONT, PLAN_DEFEND, PLAN_ATTACK, PLAN_N };
enum { LAW_CONS_N = 4, LAW_ECON_N = 3 };
// the cabinet (ministries.cpp): one minister in each post
enum { MIN_INTERIOR, MIN_FOREIGN, MIN_FINANCE, MIN_WAR, MIN_MARINE, MIN_EDUCATION, MIN_INDUSTRY, MIN_N };
extern const char* MINISTRY_NAMES[MIN_N];
extern const char* MINISTRY_KEYS[MIN_N];
extern const char* MINISTRY_ICONS[MIN_N];
constexpr float MINISTER_COST = 25;      // influence to change a minister
constexpr int MINISTER_COOLDOWN = 180;   // days before that post can change again
enum { STATION_RESEARCH = 1, STATION_WHALING = 2, STATION_COAL = 4 };
enum { FLEET_HOME, FLEET_TRADE, FLEET_BLOCKADE, FLEET_N };
// ---- the home front (society.cpp)
// budget categories: each share of the income feeds one development index; the reserve fills the treasury
enum { BUD_EDU, BUD_HEALTH, BUD_INFRA, BUD_AGRI, BUD_WELFARE, BUD_POLICE, BUD_MILITARY, BUD_RESERVE, BUD_N };
enum { DEV_LITERACY, DEV_HEALTH, DEV_TRANSPORT, DEV_FARMING, DEV_LIVING, DEV_ORDER, DEV_READINESS, DEV_N };
enum { CLS_LOWER, CLS_MIDDLE, CLS_UPPER, CLS_N };
// laws beyond conscription and economy: taxation, franchise, press, labour
enum { POL_TAX, POL_VOTE, POL_PRESS, POL_LABOUR, POL_SCIENCE, POL_N };
constexpr int POL_OPTS = 4;              // options per law (the press law has three; its fourth is unused)
enum { PJ_SCHOOL, PJ_HOSPITAL, PJ_FARM, PJ_PASTURE, PJ_HOUSING, PJ_N };
constexpr int PJ_MAX = 3;                // levels per project in one province
// intelligence needed (0-100) to see each group of a foreign nation's figures; below it they stay hidden
enum { INTEL_ECON = 20, INTEL_MIL = 40, INTEL_TECH = 60, INTEL_PLANS = 80 };
const float AGENT_COST = 30;      // influence to train one agent

extern const char* IDEO_NAMES[IDEO_N];     // "autocratic", ...
extern const char* IDEO_TITLES[IDEO_N];    // "Autocratic", ...
extern const char* RES_NAMES[RES_N];
extern const char* STATUS_NAMES[ST_N];
extern const char* TERRAIN_NAMES[TER_N];
// what each terrain does (terrain.cpp); multipliers unless noted
struct TerrainInfo {
    float def, atk, speed;      // defender strength, attacker strength, marching speed
    float attrition;            // strength lost a day by troops standing here (half when in supply)
    float supply;               // how far supply carries across it
    float build, upkeep;        // construction cost; railway and fort upkeep
    float wealth;               // taxable wealth per head
    float cavalry, armour;      // extra attack multiplier for cavalry and tanks
    const char* desc;
};
extern const TerrainInfo TERRAIN[TER_N];
extern const char* BUILD_NAMES[B_N];
extern const char* WING_NAMES[AW_N];       // "Fighter wing", ...
extern const char* MISSION_NAMES[MIS_N];
float wing_size(int type);                 // aircraft in a full wing
float wing_range(int type);                // km from base
extern const char* CONS_LAW_NAMES[LAW_CONS_N];
extern const char* ECON_LAW_NAMES[LAW_ECON_N];
extern const char* FLEET_NAMES[FLEET_N];
extern const char* ORDER_NAMES[ORD_N];
extern const char* PLAN_NAMES[PLAN_N];
extern const char* BUDGET_NAMES[BUD_N];
extern const char* DEV_NAMES[DEV_N];
extern const char* CLASS_NAMES[CLS_N];
extern const char* POL_TITLES[POL_N];
extern const char* POL_NAMES[POL_N][POL_OPTS];   // "" where a law has fewer options
extern const char* PROJECT_NAMES[PJ_N];
extern const char* PROJECT_EFFECTS[PJ_N];   // one line for tooltips
extern const int PJ_DAYS;                   // days a province project takes
int default_law(int ideology, int group);          // the law a new regime of this ideology brings
float vote_weight(int franchise, int cls);         // how much a class's mood counts for stability
// script names: "public_health", "lower_class", law groups "tax" / "vote" / "press" / "labour",
// options by their name or its first words ("universal_male", "free", "progressive"); -1 if unknown
int dev_index(const std::string& s);
float law_satisfaction(int group, int opt, int cls);   // what a law option does to a class's mood
float ideology_affinity(int ideology, int cls);         // how a class feels about a government of that ideology
float budget_preset(int ideology, int cat);            // the budget an ideology runs by default
int class_index(const std::string& s);
int policy_group(const std::string& s);
int policy_index(int group, const std::string& s);
int ideo_index(const std::string& s);
int res_index(const std::string& s);
int eq_index(const std::string& s);
int status_index(const std::string& s);

// ---- modifiers ---------------------------------------------------------------------------------
enum Mod {
    M_STAB, M_WS, M_INFL, M_RESEARCH, M_CONSTRUCT, M_PROD_CAP, M_FACTORY, M_MANPOWER, M_CONSUMER,
    M_ATTACK, M_DEFENSE, M_ORG, M_SPEED, M_FORT, M_COLONIAL, M_RECRUIT, M_SUPPLY, M_NAVAL, M_AIR,
    M_RES, M_POP_AUT, M_POP_CON, M_POP_SOC, M_POP_NAT, M_UNREST, M_AA,
    M_INCOME, M_GRAIN, M_LIVESTOCK, M_LITERACY, M_HEALTH, M_LIVING, M_SAT_LOWER, M_SAT_MIDDLE, M_SAT_UPPER, M_N
};
extern const char* MOD_NAMES[M_N];
extern const char* MOD_LABELS[M_N];
struct Mods {
    float v[M_N] = {};
    void add(const Mods& o, float k = 1) { for (int i = 0; i < M_N; i++) v[i] += o.v[i] * k; }
    float operator[](int i) const { return v[i]; }
    bool empty() const { for (float f : v) if (f != 0) return false; return true; }
};
Mods parse_mods(const Node* n);
std::string describe_mods(const Mods& m);


// ---- map & provinces -------------------------------------------------------------------------------
constexpr int GW = 8192, GH = 4096;            // about 5 km per cell at the equator; data/provinces.map must match
inline float cell_lon(int x) { return -180.0f + (x + 0.5f) * 360.0f / GW; }
inline float cell_lat(int y) { return 90.0f - (y + 0.5f) * 180.0f / GH; }
inline int lon_cell(float lon) { int x = (int)floorf((lon + 180.0f) / 360.0f * GW); return ((x % GW) + GW) % GW; }
inline int lat_cell(float lat) { return clampv((int)floorf((90.0f - lat) / 180.0f * GH), 0, GH - 1); }
