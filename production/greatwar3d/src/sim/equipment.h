// Equipment, designs and division templates.
//
// Technologies unlock *chassis* (a rifle, a tank hull, an airframe, a ship hull, a locomotive...) and
// *modules* (guns, engines, armour, radios...). A nation turns them into its own *designs*, which production
// lines build and the stockpile counts one by one. Divisions are built from *templates*: a list of
// battalions and support companies, each needing so many pieces of equipment of a category, role and
// weight class. A division's fighting values come from the designs actually in its hands.
#pragma once
#include <map>
#include <string>
#include <vector>

#include "defs.h"

// ---- what equipment there is -----------------------------------------------------------------------
enum { CAT_SMALL_ARMS, CAT_ARTILLERY, CAT_VEHICLE, CAT_MOTOR, CAT_AIR, CAT_SHIP, CAT_TRAIN, CAT_TRACTOR, CAT_N };
extern const char* CAT_KEYS[CAT_N];      // script names: small_arms, artillery, vehicle ...
extern const char* CAT_LABELS[CAT_N];    // "Infantry weapons", "Artillery", "Armoured vehicles" ...

enum {
    RO_INFANTRY,                                                                    // small arms
    RO_FIELD, RO_HOWITZER, RO_AT, RO_AA, RO_ROCKET,                                 // artillery
    RO_TANK, RO_TD, RO_SPG, RO_SPAAG, RO_ARMOURED_CAR, RO_APC, RO_IFV,              // vehicles
    RO_TRUCK,                                                                       // motor
    RO_FIGHTER, RO_CAS, RO_TACTICAL, RO_STRATEGIC, RO_NAVAL_BOMBER, RO_AIRSHIP, RO_HELICOPTER,   // air
    RO_DESTROYER, RO_CRUISER, RO_CAPITAL, RO_CARRIER, RO_SUBMARINE, RO_FRIGATE,     // ships
    RO_LOCOMOTIVE, RO_ARMOURED_TRAIN,                                               // trains
    RO_TRACTOR,                                                                     // farm machinery
    RO_N
};
struct RoleInfo {
    const char* key;
    const char* label;      // singular, for names: "Tank Destroyer"
    const char* group;      // plural, for the stockpile tree: "Tank destroyers"
    int cat;
};
extern const RoleInfo ROLES[RO_N];

enum { WT_NONE, WT_LIGHT, WT_MEDIUM, WT_HEAVY, WT_SUPERHEAVY, WT_MBT, WT_N };
extern const char* WEIGHT_KEYS[WT_N];
extern const char* WEIGHT_LABELS[WT_N];

// equipment statistics, all additive from chassis and modules (a few modules also multiply)
enum {
    S_SOFT, S_HARD, S_AIR, S_DEF, S_BRK, S_ARMOR, S_PIERCE, S_HARDNESS, S_SPEED, S_RELIAB, S_COST,
    S_RANGE, S_AGILITY, S_GROUND, S_BOMB, S_NAVAL, S_TORP, S_SUBATK, S_AA, S_HP, S_SUPPLY, S_FARM,
    S_RECON, S_STEALTH, S_CAPACITY, S_N
};
extern const char* STAT_KEYS[S_N];
extern const char* STAT_LABELS[S_N];

struct EqStats {
    float v[S_N] = {};
    float res[3] = {};          // coal, iron, oil per unit built
};

struct Chassis {
    std::string id, name, desc;
    int cat = CAT_SMALL_ARMS, weight = WT_NONE;
    std::vector<int> roles;     // what a design on it can be; the first is the default
    int tech = -1;              // needed to build it (-1: always)
    int year = 1900;
    std::vector<std::string> slots;
    EqStats base;
};

struct Module {
    std::string id, name, slot, desc;
    int cat = CAT_SMALL_ARMS;
    int tech = -1;
    EqStats add;
    float mult[S_N];            // 1 = no change
    std::vector<int> only;      // roles it is for (empty: any)
    Module() { for (float& m : mult) m = 1; }
};

struct Design {
    int id = -1, nation = -1, chassis = -1, role = 0, year = 1906;
    std::string name;
    std::vector<int> mods;      // per chassis slot, -1 empty
    EqStats st;                 // derived (design_stats)
    float score = 0;            // derived: worth for its role (design_score)
    bool obsolete = false;      // hidden from new production lines and upgrades stop preferring it
};

// a template's need: so many pieces of a category, of a role and weight class (-1 = any)
struct Need {
    int cat = 0, role = -1, weight = -1;
    float count = 0;
    float units = 0;            // battalions sharing this need: each gets the design's stats once
    bool same(const Need& o) const { return cat == o.cat && role == o.role && weight == o.weight; }
};

enum { AB_DIG, AB_ATTRITION, AB_CASUALTY, AB_ORG_REGEN, AB_SUPPRESS, AB_RECON, AB_SUPPLY, AB_RIVER, AB_N };
extern const char* ABIL_KEYS[AB_N];
extern const char* ABIL_LABELS[AB_N];

struct Battalion {
    std::string id, name, short_name, icon, desc;
    bool support = false;
    int tech = -1;
    float mp = 1, width = 2, org = 30, speed = 4, train = 60, horses = 0;
    float base[S_N] = {};       // flat values, before equipment
    float terrain[TER_N] = {};  // attack modifier by terrain (+0.2 = +20%)
    float abil[AB_N] = {};
    std::vector<Need> need;     // per battalion
};

// a division template: battalions in the line, companies in support
struct DivTemplate {
    std::string name;
    std::vector<int> line, support;   // battalion indices, repeats allowed in the line
    std::string kind;                 // infantry, cavalry, armour ... (for the AI and the map symbol)
    bool obsolete = false;
    // derived (template_stats)
    std::vector<Need> needs;
    float mp = 0, width = 0, train = 60, horses = 0;
};

// division combat values, derived from the template and the equipment in hand
enum { DS_SOFT, DS_HARD, DS_AIR, DS_DEF, DS_BRK, DS_ARMOR, DS_PIERCE, DS_HARDNESS, DS_ORG, DS_SPEED, DS_WIDTH, DS_RECON, DS_MP, DS_FILL,
       DS_GUNS,     // batteries of field guns, howitzers, rockets and self-propelled guns in hand (combat.cpp)
       DS_N };

// ---- weather (weather.cpp) -------------------------------------------------------------------------------
enum { WX_CLEAR, WX_RAIN, WX_STORM, WX_MUD, WX_SNOW, WX_BLIZZARD, WX_HEAT, WX_SANDSTORM, WX_FOG, WX_N };
extern const char* WX_NAMES[WX_N];
struct WeatherInfo { float move, attack, air, attrition, supply; };
extern const WeatherInfo WEATHER[WX_N];
struct Weather { float temp = 15; int cond = WX_CLEAR; };

// ---- espionage agents (intel.cpp) ------------------------------------------------------------------------
struct Agent {
    int id = 0;
    std::string name;
    float infiltration = 1, counter = 1, cover = 1;   // 0.5 (raw) .. 3 (master)
    int post = -1;              // nation they work in: -1 at home waiting, our own index = hunting spies
    int days = 0;               // days in post: they get better at it
};
