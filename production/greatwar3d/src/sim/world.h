// The whole simulation state. No raylib in here: rendering and UI read from it.
#pragma once
#include <climits>
#include <deque>
#include <functional>
#include <memory>
#include <set>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/util.h"
#include "../script/parser.h"
#include "defs.h"
#include "equipment.h"

enum { REG_ORDINARY, REG_MODEL, REG_PLUNDER };
struct Province {
    std::string name;
    float lat = 0, lon = 0;
    int owner = -1, controller = -1, status = ST_CORE;
    int size = 1, fort = 0, infra = 1, cf = 0, mf = 0;
    int coast = 0, airfield = 0, airbase = 0, airship = 0, seaplane = 0, airport = 0;   // levels 0-5
    int regime = 0;         // how a colony is run (WORLD.md): REG_ORDINARY, REG_MODEL (calm, schooled) or REG_PLUNDER (Congo-style)
    // what a building type counts in this province: factories built, or its level
    int& level(int type) {
        switch (type) {
            case B_CF: return cf;
            case B_MF: return mf;
            case B_INFRA: return infra;
            case B_COAST: return coast;
            case B_AIRFIELD: return airfield;
            case B_AIRBASE: return airbase;
            case B_AIRSHIP: return airship;
            case B_SEAPLANE: return seaplane;
            case B_AIRPORT: return airport;
            default: return fort;
        }
    }
    int level(int type) const { return const_cast<Province*>(this)->level(type); }
    float res[RES_N] = {};
    float pop = 0;          // millions
    int vp = 1;
    bool coastal = false, antarctic = false, capital = false;
    std::string region;     // the admin-1 unit a city's province lies in ("Rhône" for Lyon); empty where the name already is one
    bool town = false;      // countryside named after a small town (tools/gen_towns.py): a few houses on the map, nothing more
    int release = -1;       // nation that can be released from here
    int terrain = TER_PLAINS, cells = 0;
    int stations = 0;       // antarctic station bitmask
    float bomb = 0;         // bomb damage 0..1: lost factory output, repaired over time
    int proj[PJ_N] = {};    // levels of schools, hospitals, farms, pastures and housing
    float unrest = 0;       // 0-100 (unrest.cpp); rioting above 60
    bool riot = false;
    std::vector<int> adj;
    std::vector<float> adj_km;
    std::vector<int> adj_edge;   // km of shared land border with each of adj (0: linked across a strait); from the map
    // Railway lines and upkeep count less in countryside: the towns split it into about twice as many provinces as there
    // were, and a nation's railways should not cost more for that.
    float rail_weight() const { return size == 0 ? 0.45f : 1.0f; }
    int slots() const { return antarctic ? 1 : size == 0 ? 1 : (status == ST_CORE ? size * 4 + 2 : size * 2 + 1); }
};

struct Division {
    int id = 0, nation = -1, prov = -1;
    int tmpl = 0;              // index into the nation's templates
    float str = 1, org = 0;    // str: manpower present (0-1)
    std::vector<std::vector<std::pair<int, float>>> kit;   // per template need: (design, pieces)
    float xp = 0.1f;           // experience 0-1: green .. elite; replacements dilute it
    float fam[TER_N] = {};     // how used it is to each terrain
    float acc_cold = 0, acc_hot = 0, forage = 0, eq_fam = 1;   // used to cold, heat, short supply, its equipment
    float st[DS_N] = {};       // derived: div_stats
    std::vector<int> path;     // provinces still to walk through
    float prog = 0;            // km walked toward path[0]
    int train = 0;             // days until ready (still training if > 0)
    int sea_dest = -1;         // sailing to this province
    float sea_days = 0;
    int attacking = -1;        // province being assaulted
    float dig = 0;             // days dug in (stationary, not fighting); trenches build up defence
    int order = ORD_NONE;      // standing order (orders.cpp)
    int army = -1;             // the army it belongs to (armies.cpp), -1 none
    float sea_total = 0;       // the whole voyage, days
    float cmd_atk = 1, cmd_def = 1, cmd_log = 0;   // derived daily: from its commander and its plan (refresh_commands)
    bool alive = true, in_supply = true;
};

// ---- air wings -------------------------------------------------------------------------------------------
struct AirWing {
    int id = 0, nation = -1, base = -1, type = AW_FIGHTER, mission = MIS_IDLE, target = -1;
    int design = -1;           // the aircraft it flies; type follows from its role
    float planes = 0;
    bool alive = true;
};

// ---- nations ------------------------------------------------------------------------------------------
struct ActiveIdea { int idea; int expires = -1; };
struct ResearchSlot { int tech = -1; float prog = 0; };
struct Project { int prov = -1, type = PJ_SCHOOL; float days = 0; };   // paid up front, done when days runs out
// a national spirit that comes and goes with the state of the nation (diet, debt, literacy ...)
struct DynSpirit {
    std::string name, desc;
    Mods mods;
    bool good = false;
};
struct ProdLine { int design = -1; int factories = 1; float eff = 0.15f; float acc = 0; };
struct Build { int type = B_CF, prov = -1; float prog = 0; };
struct TradeDeal { int buyer, seller, res; float amount; };

struct IdeologyPath {
    std::string leader, party, title, desc;
    Mods mods;
};

// A way across the water between two ports (sea.cpp): points in lat and lon, with the distance run at each. The
// longitudes run on past 180 so a route over the date line stays continuous.
struct SeaRoute {
    std::vector<float> lat, lon, cum;
    float km = 0;
    void at(float t, float& la, float& lo, float* heading = nullptr) const;   // t 0..1; heading in radians from east, north positive
};

// A share of the nation's warships, with a place and a mission. See fleets.cpp.
struct Fleet {
    int id = 0;
    std::string name;
    int prov = -1;                      // the coast it lies off (or left from, while sailing); -1: no coast at all
    int dest = -1;                      // sailing to this coast
    float days = 0, total = 0;          // days left and the whole voyage
    int mission = FLEET_HOME;           // guard the coast, escort trade or blockade enemies
    float share = 1;                    // of the navy's strength (derived from ships)
    std::map<int, float> ships;         // design -> ships; over all fleets these add up to the stock
    int admiral = -1;                   // commander id
};

// A general or an admiral (armies.cpp).
struct Commander {
    int id = 0;
    std::string name;
    bool admiral = false;
    int attack = 1, defence = 1, logistics = 1;   // 1..5
    float xp = 0;
    int retire = 3000;                  // the year they leave
};
struct PoolCommander { int nation = -1, from = 1906; bool joined = false; Commander c; };
// A group of divisions under a general, with the battle plan they follow.
struct Army {
    int id = 0;
    std::string name;
    int commander = -1;
    int plan = PLAN_NONE;
    int target = -1;                    // PLAN_FRONT: the nation whose border it holds
    std::vector<int> line;              // PLAN_DEFEND: provinces to hold. PLAN_ATTACK: waypoints in order
    int stage = 0;                      // PLAN_ATTACK: the waypoint it is making for
    float ready = 0;                    // PLAN_ATTACK: 0..1, up to +20% attack, grows while preparing and burns off in action
    bool executing = false;
};
// ---- battles (combat.cpp) --------------------------------------------------------------------------------
// A battle lasts as long as a province is under assault. It opens with the guns (if the attacker has them and the
// defender has works worth shelling), goes on to the assault, and wears into a struggle when neither side gives way.
enum { BP_BOMBARD, BP_ASSAULT, BP_STRUGGLE, BP_N };
extern const char* PHASE_NAMES[BP_N];
// what each side is trying, chosen anew every few days by its general
enum { TA_ASSAULT, TA_BARRAGE, TA_INFILTRATE, TA_ENVELOP, TA_ARMOUR, TA_PROBE, TA_N };
enum { TD_HOLD, TD_DEPTH, TD_COUNTER, TD_WITHDRAW, TD_RESERVE, TD_N };
struct TacticInfo { const char* name; const char* about; };
extern const TacticInfo ATTACK_TACTICS[TA_N];
extern const TacticInfo DEFENCE_TACTICS[TD_N];
struct Battle {
    // saved
    int prov = -1, attacker = -1, defender = -1;
    int start = 0, days = 0;            // the day it opened, days fought
    int prep = 0;                       // days of bombardment before the infantry goes in
    int tac_a = TA_ASSAULT, tac_d = TD_HOLD;
    float loss_a = 0, loss_d = 0;       // men lost so far, thousands
    float cut = 0;                      // 0..0.6: how far the guns have broken down the trenches and forts
    int withdraw = 0;                   // days the defence has been falling back (TD_WITHDRAW)
    int seen = 0;                       // the last day it was fought
    // told again every day, for whoever looks at the battle
    int phase = BP_ASSAULT;
    float fire_a = 0, guard_d = 0, fire_d = 0, guard_a = 0;    // the attack's fire against the defence's cover, and the reverse
    float works = 1;                    // what terrain, trenches and forts are worth to the defender today
    float width = 0, air = 0.5f;        // frontage, and the attacker's share of the sky
    int front_a = 0, front_d = 0, res_a = 0, res_d = 0, sides = 1;
    bool outflanked = false, encircled = false, countered = false;      // countered: the defence read the attack's tactic
};

// What one nation can see of others' forces, built once a frame by World::forces_view.
struct World;
struct ForcesView {
    int viewer = -1;                    // -1: everything (no player)
    std::vector<char> known;            // per nation: its own, or military intelligence (allies share theirs)
    std::vector<char> contact;          // per province: our troops or our land touch it
    bool sees(const World& w, int nation, int prov) const;
};

struct Courting { float due = 0, uses = 0; };   // relation points still to arrive; approaches made lately (fades by one a year)
struct Nation {
    std::string tag, name, adj, blurb;
    std::string flag;                   // flag spec, drawn by the UI (src/ui/flags.cpp)
    bool rebel_slot = false;            // kept free for the rebel side of a civil war (unrest.cpp)
    int origin = -1;                    // rebels: the nation they rose in
    unsigned char col[3] = {128, 128, 128};
    bool major = false, exists = true;
    int capital = -1;
    int ideology = IDEO_AUT;
    int sub = -1;                       // index into World::subs, -1: no particular current
    int campaign = -1;                  // ideology we are openly campaigning for (World::start_campaign), -1 none
    int campaign_day = -1;              // the day it started
    int calm_until = -1;                // a transfer of power won by campaign: no civil war before this day
    float pop[IDEO_N] = {25, 25, 25, 25};
    IdeologyPath path[IDEO_N];
    float stability = 50, war_support = 30, influence = 50, manpower = 0;
    int law_cons = 1, law_econ = 0;
    std::vector<ActiveIdea> ideas;
    std::vector<char> techs, focus_done;
    std::vector<char> tech_vis, focus_vis;   // static, from the data files: which techs and focuses this nation sees
    std::vector<ResearchSlot> slots;
    std::vector<ResearchSlot> xslots;   // experimental research: only experimental techs
    int extra_slots = 0, extra_xslots = 0;   // research slots granted by focuses, decisions and events
    int focus = -1;
    float focus_prog = 0;
    int prog = -1, prog_loop = -1;      // programme running, programme set to repeat
    float prog_done = 0, prog_days = 0; // its progress and its rolled duration
    std::vector<int> prog_next, prog_count;   // per programme: first day it may run again, times completed
    std::set<std::string> flags;
    std::map<int, float> stock;         // design -> pieces in the stockpile
    std::vector<DivTemplate> tmpls;     // division templates
    std::vector<ProdLine> lines;
    std::vector<Build> builds;
    int faction = -1, overlord = -1;
    std::vector<float> rel;
    std::set<int> ententes, guarantees, wargoals;
    std::map<int, Courting> courting;   // nation -> goodwill bought there and still on its way (improve_relations); saved
    int justify_target = -1;
    float justify_days = 0;
    int minister[MIN_N];                // index into World::ministers, per post (ministries.cpp)
    int minister_next[MIN_N] = {};      // first day this post can change again
    std::vector<int> decision_cd;       // per decision: day it becomes available again
    std::vector<std::pair<int, int>> decision_run;  // (decision, finish day)
    bool allow_exports = true;
    // home front (society.cpp)
    float money = 0;                    // treasury, in millions
    float budget[BUD_N] = {};           // shares of income, summing to 1
    float dev[DEV_N] = {};              // development indices 0-100
    float cls_share[CLS_N] = {};        // share of the population in each class
    float cls_sat[CLS_N] = {};          // satisfaction 0-100
    int law[POL_N] = {-1, -1, -1, -1, -1};  // -1 until set: the ideology's default
    float diet = 0.7f;                  // the grain share of what the people are used to eating; drifts over decades
    std::vector<Project> projects;
    std::vector<Fleet> fleets;          // the navy, divided into fleets (fleets.cpp); shares add up to 1
    int next_fleet = 1;
    std::vector<Commander> commanders;  // generals and admirals (armies.cpp)
    std::vector<Army> armies;
    int next_cmd = 1, next_army = 1;
    std::vector<Agent> agent_list;      // our agents (intel.cpp)
    std::vector<Agent> candidates;      // who could be hired; changes every three months
    int candidate_day = -1000;
    int agents = 0;                     // derived: agents waiting at home for a posting
    std::vector<int> spies;             // derived: agents posted in each nation; spies[self] hunt foreign agents at home
    std::vector<float> intel;           // 0-100: what our own network knows about each nation
    std::string agency;                 // the intelligence service's name (empty: agency_name() picks the historical one)
    int logo[4] = {-1, 0, 0, 0};        // its badge: shape, field colour, emblem colour, emblem (-1 shape: from the flag colours)

    // derived every day
    Mods mods;
    float cf_total = 0, mf_total = 0, cf_free = 0, consumer = 0, trade_cf = 0;
    float res_prod[RES_N] = {}, res_need[RES_N] = {}, res_in[RES_N] = {}, res_out[RES_N] = {}, res_ratio[RES_N] = {};
    float mp_cap = 0, pop_total = 0;
    int vp_total = 0, vp_lost = 0, num_provs = 0, num_divs = 0;
    float war_losses = 0;
    int cap_days = 0;          // consecutive days past the surrender threshold
    int war_days = 0;          // consecutive days at war (war exhaustion)
    float upkeep = 0;                   // money a day for railways and forts, more in rough country (derived)
    float navy_pow = 0;                 // naval strength in 1906 warships (derived from the stockpile)
    float rail_ratio = 1, farm_mech = 0, truck_supply = 0;   // locomotives, tractors and lorries against needs (derived)
    float income = 0, food_ratio = 1, diet_eaten = 0.7f;   // money per day; calories eaten / needed; grain share eaten
    float core_pop = 0, wealth = 1;    // millions in core provinces; income per head against a rich nation of 1906
    float dev_target[DEV_N] = {}, sat_target[CLS_N] = {};
    std::vector<DynSpirit> spirits;
    std::string display_name() const;
    std::string agency_name() const;
    Nation() { for (int& m : minister) m = -1; }
};

struct Faction {
    std::string name;
    int leader = -1;
    std::vector<int> members;
    bool alive = true;
};

// ---- scripted content -----------------------------------------------------------------------------------
struct Idea { std::string id, name, desc; Mods mods; };
struct Tech {
    std::string id, name, cat, desc;
    std::string tag;                    // set: only this nation can see it (data/techs/<TAG>.txt)
    std::string icon;                   // optional: overrides the icon picked from the id and category
    int replaces = -1;                  // a national version of this common tech, shown in its place
    int year = 1906;
    float cost = 60;
    bool experimental = false;          // only in experimental research slots
    std::vector<int> req;
    Mods mods;
    int x = 0, y = 0;
};
struct Focus {
    std::string id, name, desc, branch;
    std::string tag;                    // set: from data/focus/<TAG>.txt, only that nation sees it
    std::string icon;                   // optional: overrides the icon picked from the id and branch
    std::string lane;                   // optional: the sub-ideology or doctrine column this focus belongs to (banner in the tree)
    int ideo = -1;                      // optional: ideology whose colour the node takes
    int x = 0, y = 0;
    float days = 70;
    std::vector<std::vector<int>> req;  // AND of OR-groups
    std::vector<int> mutex;
    const Node *avail = nullptr, *effect = nullptr, *ai = nullptr;
};
// A sub-ideology: the current inside a ruling ideology (social democracy, Bolshevism, Taisho democracy...).
// Set by focuses; data/subideologies.txt. Its modifiers apply while the nation has it.
struct SubIdeology {
    std::string id, name, desc;
    std::string tag;                    // set: only this nation's focuses use it (for display only)
    int ideology = 0;
    Mods mods;
};
// A programme: a repeatable national campaign that runs beside the focus tree (data/programmes/). Some are always
// on offer, some only while a condition holds (a war, a deficit, an ideology). Duration varies: `speed` factors and
// `variance` are rolled when it starts. A nation runs one at a time and may set one to repeat by itself.
struct Programme {
    std::string id, name, desc, cat, icon;
    float days = 120, variance = 0.2f;
    int cooldown = 365;                 // days after finishing before it can run again
    const Node *visible = nullptr, *avail = nullptr, *speed = nullptr, *effect = nullptr, *ai = nullptr;
};
struct EventOpt { std::string name; const Node* effect = nullptr; const Node* ai = nullptr; };
struct Event {
    std::string id, title, desc;
    const Node* trigger = nullptr;
    int after = 0, before = INT_MAX;
    float chance = 100;             // % per monthly check
    bool triggered_only = false, once = true, global_once = false, fired_any = false;
    std::vector<EventOpt> opts;
    std::vector<char> fired;        // per nation
};
// A kind of minister: who could hold a post in the cabinet (data/ministries.txt). What they do applies while in office.
struct Minister {
    std::string id, name, desc;
    int ministry = 0;
    unsigned ideologies = (1u << IDEO_N) - 1;   // bitmask of the governments that would appoint one
    Mods mods;
    const Node* ai = nullptr;
};
struct Decision {
    std::string id, name, desc, cat;
    int ministry = -1;                  // the ministry that carries it out, -1 none
    float cost = 0;
    int days = 0, cooldown = 0;
    bool repeatable = false;
    const Node *visible = nullptr, *avail = nullptr, *effect = nullptr, *ai = nullptr;
};

// A choice the player has to answer (events, calls to arms, offers).
struct Choice {
    std::string title, text;
    std::string pic;                 // the event it came from: its picture is data/art/events/<pic>
    std::vector<std::string> labels;
    std::vector<std::function<void()>> actions;
    std::vector<std::string> tips;
};

struct NewsItem { int day; std::string text; int nation; };

// A research or focus the player has just finished: the app pauses and reports it. Display only, never saved.
struct Completed { bool focus; int id, day; };

// A political campaign: the peaceful road to more support for an ideology. Paid for in influence, and instead of
// costing stability (like letting the country slide into crisis) it steadies the nation while it runs.
constexpr float CAMPAIGN_START = 25;    // influence to open it
constexpr float CAMPAIGN_DAILY = 0.5f;  // influence a day (a nation earns 1 to 1.25)
constexpr float CAMPAIGN_RATE = 0.1f;   // points of support a day, before the shares are renormalised
constexpr float CAMPAIGN_STABILITY = 6; // added to the stability the nation drifts toward
constexpr float CAMPAIGN_CAP = 60;      // it ends by itself at this support
constexpr int CAMPAIGN_CALM_DAYS = 365; // no civil war for this long after it brings its ideology to power

// data-file helpers (load.cpp)
std::vector<std::string> files_in(const std::string& dir);    // every .txt file in a folder, sorted
std::string stem_of(const std::string& path);
std::vector<std::string> words(const Node* n);                // the keys of a block, or a single value
struct World;
const Node* load_doc(World& w, const std::string& path);

// ---- the world ----------------------------------------------------------------------------------------------
struct World {
    // province of every map cell (GW x GH), -1 for sea
    std::vector<int16_t> pmap;

    std::vector<Province> provs;
    std::vector<Nation> nations;
    std::vector<Faction> factions;
    std::vector<Division> divs;
    std::map<int, Battle> battles;      // by province under assault (combat.cpp), saved
    std::vector<AirWing> wings;
    std::vector<Idea> ideas;
    std::vector<Tech> techs;
    std::vector<Focus> focuses;
    std::vector<Programme> programmes;
    std::vector<SubIdeology> subs;
    std::unordered_map<std::string, int> sub_idx;
    std::vector<Event> events;
    std::vector<Decision> decisions;
    std::vector<Minister> ministers;
    std::unordered_map<std::string, int> minister_idx;
    std::vector<TradeDeal> deals;
    std::vector<std::vector<char>> war;      // war[a][b]
    std::vector<std::vector<float>> base_rel; // 1906 relations; current relations drift back toward these
    std::vector<std::unique_ptr<Node>> docs;  // parsed files kept alive for Node pointers
    std::set<std::string> global_flags;

    int day = 0, player = -1, next_div_id = 1, next_wing_id = 1, next_agent_id = 1;
    float hour = 9;                     // time of day, 0-24 (a new game starts in the morning): only the clock and the day-night light use it; the sim ticks once a day
    Rng rng{20240607};
    std::vector<NewsItem> news;
    std::deque<Choice> choices;              // waiting for the player
    std::vector<PoolCommander> commander_pool;   // data/commanders.txt; `joined` is rebuilt from the year and the names on load
    std::vector<Completed> completed;        // finished research and focuses waiting to be reported to the player
    std::set<std::string> cut_techs;         // load only: techs past CONTENT_END_YEAR, so what needs them goes too
    std::string err;                         // first load error
    std::vector<std::string> warnings;
    bool map_dirty = true, borders_dirty = true;
    std::string data_dir;

    // lookups
    std::unordered_map<std::string, int> nation_idx, prov_idx, idea_idx, tech_idx, focus_idx, event_idx, decision_idx;
    std::unordered_map<std::string, int> chassis_idx, module_idx, battalion_idx;

    // equipment and divisions (equipment.cpp, divisions.cpp): static data, then every design ever made
    std::vector<Chassis> chassis;
    std::vector<Module> modules;
    std::vector<Battalion> battalions;
    std::vector<DivTemplate> std_templates;   // data/templates.txt: nations get them when they can field them
    std::vector<Design> designs;
    // weather (weather.cpp): today's, derived from the date
    std::vector<Weather> wx;
    int wx_day = -1;
    int nation_of(const std::string& tag) const { auto it = nation_idx.find(tag); return it == nation_idx.end() ? -1 : it->second; }
    int prov_of(const std::string& n) const { auto it = prov_idx.find(n); return it == prov_idx.end() ? -1 : it->second; }

    // ---- loading (load.cpp, geo.cpp)
    bool load(const std::string& dir);
    bool load_map(const std::string& path);
    void setup_start();
    bool save(const std::string& path) const;                  // save.cpp
    bool load_save(const std::string& path, std::string& why);

    // ---- daily simulation (world.cpp and friends)
    void tick_day();
    void recompute(int n);
    void tick_economy(int n);
    void tick_politics(int n);
    void tick_research(int n);
    void tick_focus(int n);
    void tick_military();
    void fight_battle(int prov, std::vector<int>& atk);         // one day of one battle (combat.cpp)
    void close_battles();                                       // the ones nobody fought today are over
    const Battle* battle_at(int prov) const { auto it = battles.find(prov); return it == battles.end() ? nullptr : &it->second; }
    void tick_supply();
    void tick_air();                                           // air.cpp
    void tick_events_monthly();
    void tick_decisions(int n);
    void tick_trade();
    void check_capitulation();

    // ---- helpers
    std::string date() const { return date_string(day); }
    int year() const;
    void add_news(const std::string& text, int nation = -1);
    bool at_war(int n) const;
    bool allied(int a, int b) const;        // same faction / subject / overlord
    bool friendly(int a, int b) const;      // allied or entente partners at war together, or same nation
    bool can_enter(int n, int prov) const;
    bool hostile_divs_in(int prov, int n) const;
    // Who is in each province, ascending by index: alive, on land. Anything that changes a division's province,
    // aliveness or voyage must go through unindex_div / index_div, or set div_index_ok = false.
    const std::vector<int>& divs_at(int prov) const;
    void unindex_div(const Division& d);
    void index_div(const Division& d);
    mutable std::vector<std::vector<int>> div_index;
    mutable bool div_index_ok = false;
    std::vector<int> divs_in(int prov) const;
    float relation(int a, int b) const { return nations[a].rel[b]; }
    void add_relation(int a, int b, float v);
    std::vector<int> faction_bloc(int n) const;   // n, faction members, subjects, overlord
    int faction_of(int n) const { return nations[n].faction; }
    bool is_faction_leader(int n) const { int f = nations[n].faction; return f >= 0 && factions[f].leader == n; }
    float navy(int n) const;
    float navy_on(int n, int mission) const;      // fleets.cpp: strength of the fleets on this mission
    int home_port(int n) const;
    float km_to_sea(int prov) const;
    mutable std::vector<float> sea_km;      // km_to_sea's answers
    // sea routes (sea.cpp): derived from the map and the year, built when first needed, never saved
    mutable std::vector<unsigned char> sea_grid;
    mutable std::vector<int> sea_comp, sea_port, sea_prev, sea_stamp;
    mutable std::vector<float> sea_g;
    mutable int sea_ocean = -1, sea_tick = 0;
    mutable std::unordered_map<long long, SeaRoute> sea_routes;
    void sea_build() const;
    int sea_port_cell(int prov) const;
    bool sea_connected(int a, int b) const;                 // the same water: a Caspian port reaches no ocean port
    bool port_latlon(int prov, float& la, float& lo) const; // the spot off a province where its ships lie
    const SeaRoute* sea_route(int from, int to) const;      // nullptr: no way by sea
    float voyage_days(int from, int to) const;
    bool open_water(int cell_x, int cell_y) const;
    int nearest_coast(float lat, float lon, float max_km, int from = -1) const;   // from: must share its water
    int enemy_coast(int n, int from) const;
    Fleet* fleet_by_id(int n, int id);
    void ensure_fleet(int n);
    void tick_fleets(int n);
    bool order_fleet(int n, Fleet& f, int prov);
    void set_fleet_mission(int n, Fleet& f, int mission);
    bool split_fleet(int n, int id);
    bool merge_fleets(int n, int into, int from);
    ForcesView forces_view(int viewer) const;
    float blockade(int n) const;            // 0..0.8: share of n's imports lost to enemy blockades
    float effective_pop(int n, int ideology) const { return nations[n].pop[ideology]; }

    // ---- actions (used by UI, AI and scripts)
    bool tech_available(int n, int t) const;
    void tech_base(int n);                   // the first air base of each kind a known tech opens, at the capital
    // the year a nation's research has reached: the fifth-newest tech it knows (one rushed tech does not count).
    // Focuses open by this, not by the calendar: research ahead of its year is slow, so it stays near the date
    int tech_era(int n) const;
    void grant_tech(int n, int t);           // also marks the common tech a national version stands for, and back
    float tech_days(int n, int t) const;
    bool start_research(int n, int slot, int t, bool experimental = false);
    void update_slots(int n);              // research slots from start, effects and the science law
    bool focus_available(int n, int f) const;
    bool focus_blocked(int n, int f) const;
    bool start_focus(int n, int f);
    bool programme_visible(int n, int p) const;
    bool programme_available(int n, int p) const;
    float programme_days(int n, int p) const;   // before the roll
    bool start_programme(int n, int p);
    void tick_programme(int n);
    int unbuild(int n, int prov, int type, bool all);   // cancel queued builds, else demolish; how many went
    void complete_focus(int n, int f);
    bool can_build(int n, int prov, int type, std::string* why = nullptr) const;
    bool ocean_coast(int prov) const;       // on the open sea: a lake shore or the Caspian does not count
    bool queue_build(int n, int prov, int type);
    float build_rate(int n, int prov) const;
    // terrain (terrain.cpp)
    std::string terrain_summary(int prov) const;
    float build_cost(int type) const;
    float build_cost_at(int n, int prov, int type) const;   // with the province status and the nation's workforce
    bool can_recruit(int n, int tmpl, std::string* why = nullptr) const;
    bool recruit(int n, int tmpl, int prov = -1, bool free = false);   // free: no manpower, no training (setup, rebels)
    bool recruit_at(int n, int tmpl, int prov);     // pays like recruit, trains in one of our core provinces
    void disband(Division& d);                      // men back to the manpower pool, equipment to the stockpile
    bool set_law(int n, int cons, int econ, bool free = false);
    bool law_allowed(int n, int cons, int econ) const;
    void set_ideology(int n, int ideo);
    bool start_campaign(int n, int ideo);    // replaces a campaign already running
    void stop_campaign(int n);
    int campaign_days_to(int n, int ideo, float pct) const;     // days of campaigning until this support, 0 if there already

    void declare_war(int a, int b, bool scripted = false);
    void make_peace(int a, int b);
    void capitulate(int n);
    bool can_declare_war(int a, int b, std::string* why = nullptr) const;
    bool start_justify(int a, int b);
    bool improve_relations(int a, int b);
    int improve_cost(int a, int b) const;   // influence the next approach to b costs a
    void tick_courting();
    bool propose_entente(int a, int b, bool force = false);
    bool guarantee(int a, int b);
    int create_faction(int leader, const std::string& name);
    bool join_faction(int n, int f);
    void leave_faction(int n);
    bool invite_to_faction(int leader, int n, bool force = false);
    bool ai_accepts_faction(int n, int f) const;
    bool offer_peace(int a, int b);
    bool request_trade(int buyer, int seller, int res, float amount);
    float tradeable(int seller, int res) const;
    void cancel_deal(size_t i);
    void transfer_province(int p, int to, bool keep_status = true);
    void annex(int winner, int loser);
    int release_nation(int tag_idx);
    void make_subject(int overlord, int subject);
    bool claim_antarctic(int n);
    bool build_station(int n, int kind);
    void kill_div(Division& d);
    int wing_type(int design) const;        // fighter, bomber or airship, from the design's role
    static int wing_type_of_role(int role) { return role == RO_FIGHTER ? AW_FIGHTER : role == RO_AIRSHIP ? AW_AIRSHIP : AW_BOMBER; }
    bool form_wing(int n, int design, int base = -1);
    // whether a wing of this design can be based here: a matching airfield with room (a capital takes anything).
    // skip: a wing already there that does not count against the room
    bool can_base(int n, int design, int type, int prov, const AirWing* skip = nullptr, std::string* why = nullptr) const;
    int airfield_for(int design, int type) const;   // the building a wing like this needs (a seaplane station or an air base serve naval bombers)
    bool in_range(const AirWing& a, int prov) const;
    bool set_mission(AirWing& a, int mission, int target, std::string* why = nullptr);
    float air_cover(int prov, int nation, int mission) const;   // planes of nation's side over prov

    bool decision_visible(int n, int d) const;
    bool decision_available(int n, int d) const;
    bool take_decision(int n, int d);

    // ---- movement & combat (military.cpp)
    bool order_move(Division& d, int dest);
    // ---- standing orders (orders.cpp)
    void tick_orders();
    // armies, commanders and battle plans (armies.cpp)
    Commander* commander(int n, int id);
    const Commander* commander(int n, int id) const;
    Army* army_by_id(int n, int id);
    const Army* army_by_id(int n, int id) const;
    int commander_post(int n, const Commander& c) const;    // the army or fleet they serve with, -1 free
    int free_commander(int n, bool admiral) const;          // the best one without a post, -1 none
    int promote_officer(int n, bool admiral);
    void load_commanders(const std::string& path);
    void tick_commanders_all();                             // monthly: joiners, retirements, empty armies, the AI's armies
    void ai_armies(int n);
    int form_army(int n, const std::vector<int>& div_ids);
    void assign_divisions(int n, int army, const std::vector<int>& div_ids);   // army -1: unassigned
    void dissolve_army(int n, int army);             // its divisions become unassigned, its general free
    void set_commander(int n, int army, int id);
    void set_admiral(int n, int fleet, int id);
    void refresh_commands();
    void credit_commander(const Division& d, bool attacking);
    float fleet_factor(int n, const Fleet& f) const;        // what its admiral adds
    void set_plan(int n, int army, int kind, int target, const std::vector<int>& line);
    std::vector<std::pair<int, int>> front_pairs(int n, int target) const;
    // how many of the player's divisions a front province should hold when spread along a front: by its length of border
    // with the enemy (target -1: anyone we are at war with) and the enemy massed against it
    float front_need(int n, int prov, int target, const std::vector<float>& enemy) const;
    // sends a division to the most needed of these provinces it can reach; true if it set off
    bool spread_to(Division& d, const std::vector<int>& line, const std::vector<float>& need, std::vector<float>& have);
    void plan_spread(int n, const Army& A, const std::vector<int>& line, const std::vector<float>& enemy);
    void tick_plans();
    // the ships of a fleet (fleets.cpp)
    void sync_fleet_ships(int n);                           // fleets' ships against the stock; new ships join the first fleet
    int detach_ships(int n, int fleet, const std::map<int, int>& pick);   // a new fleet of these (design -> how many); -1 if not
    bool order_retreat(Division& d);                // to the nearest of our provinces out of reach of the enemy
    std::vector<int> front_provinces(int n) const;  // ours, next to land held by an enemy
    std::vector<int> find_path(int n, int from, int to) const;
    float div_speed(const Division& d) const;
    float div_atk(const Division& d, float enemy_hardness) const;
    float div_def(const Division& d) const;
    float div_drive(const Division& d) const;
    float div_max_org(const Division& d) const;
    float div_quality(const Division& d) const;     // experience and familiarity with the kit
    float div_terrain(const Division& d, int terrain) const;
    float div_abil(const Division& d, int ability) const;
    float weather_attrition(const Division& d) const;

    // ---- equipment (equipment.cpp)
    bool load_equipment(const std::string& dir);
    void design_stats(Design& d) const;
    bool chassis_ok(int n, int c) const;
    bool module_ok(int n, int m) const;
    bool battalion_ok(int n, int b) const;
    bool module_fits(int m, int chassis, int slot, int role) const;
    float design_score(const Design& d) const;      // worth for its role: picks modules and the best design in stock
    std::string design_kind(const Design& d) const; // "Medium Tank Destroyer"
    int create_design(int n, int chassis, int role, const std::vector<int>& mods, const std::string& name = "");
    std::vector<int> best_modules(int n, int chassis, int role) const;
    int auto_design(int n, int chassis, int role);
    void refresh_designs(int n, bool all_roles);
    bool fits(const Design& d, const Need& need) const;
    float stock_of(int n, int design) const;
    void add_stock(int n, int design, float v);
    int best_in_stock(int n, const Need& need, float min) const;
    float stock_fitting(int n, const Need& need) const;
    float ship_power(const Design& d) const;
    void compute_navy(int n);
    void tick_production(int n);
    void retool_line(int n, int line, int design);
    void compute_civil_equipment(int n);
    // ---- divisions (divisions.cpp)
    void template_stats(DivTemplate& t) const;
    bool template_ok(int n, const DivTemplate& t) const;
    void update_std_templates(int n);
    int std_template(int n, const std::string& kind) const;
    const DivTemplate& tmpl_of(const Division& d) const;
    float kit_have(const Division& d, size_t need) const;
    void kit_fill(Division& d, float fill, float cap);
    void kit_upgrade(Division& d);
    void kit_lose(Division& d, float frac);
    void retemplate(Division& d);
    float kit_fraction(const Division& d) const;
    float kit_fraction_t(const Division& d, const DivTemplate& t) const;
    void div_stats(Division& d) const;
    void div_stats_t(Division& d, const DivTemplate& t) const;
    void template_preview(int n, const DivTemplate& t, float* out) const;   // with the best designs everywhere
    void tick_divisions();
    // which nations have divisions on land in each province (built at the start of tick_divisions, used the same day)
    std::vector<std::vector<int>> presence;
    void build_presence();
    bool hostile_here(int prov, int n) const;
    // ---- weather (weather.cpp)
    Weather compute_weather(int prov, int day) const;
    void compute_weather_all();
    mutable std::vector<float> inland;      // km over land from the moderating sea; from the map, never saved
    const std::vector<float>& inland_km() const;
    void sky_at(float lat, float lon, double day, float& cloud, float& temp) const;   // day may be fractional   // for the view: anywhere, sea too
    const Weather& weather(int prov) const;
    // ---- unrest, risings and civil wars (unrest.cpp)
    float nationalism() const;              // 0 in 1900 .. 1 in 1960: how strongly subject peoples want their own state
    void tick_unrest();
    void arm_new_nation(int n, int divisions);
    void independence_war(int owner, int people);
    int free_rebel_slot() const;
    void start_civil_war(int n, int ideology);
    bool end_civil_war(int loser);          // from capitulate: the winner takes the whole country
    bool negotiate_independence(int subject);

    // ---- scripting (script.cpp)
    bool trig(const Node* n, int nat) const;
    bool trig_one(const Node& n, int nat) const;
    void eff(const Node* n, int nat);
    void eff_one(const Node& n, int nat);
    std::string describe_trig(const Node* n, int depth = 0) const;
    std::string describe_eff(const Node* n, int depth = 0) const;
    void fire_event(int ev, int nat);
    float ai_weight(const Node* ai, int nat, float base = 1) const;

    // ---- the cabinet (ministries.cpp)
    void load_ministers(const std::string& path);
    bool minister_allowed(int n, int m) const;
    std::string minister_name(int n, int m) const;   // from a hash: never touches the RNG
    bool can_appoint(int n, int m, std::string* why = nullptr) const;
    bool appoint_minister(int n, int m, bool free = false);
    void fill_cabinet(int n);                        // every post without an allowed minister gets the likeliest one
    void ai_cabinet(int n);

    // ---- home front (society.cpp)
    void society_start();                   // after loading: starting development, classes, budget, diet
    float core_wealth(int n) const;         // taxable wealth per head of the core population
    float factories_now(int n) const;
    float ref_wealth = 1;                   // the great powers' average core_wealth, set each day
    void development_mods(int n);           // what development and the civic laws do, from saved state
    void compute_income(int n);             // income, core population and wealth (end of recompute)
    void compute_spirits(int n);            // spirits from today's food, money and moods (end of recompute)
    void tick_society(int n);
    void ai_society(int n);
    float diet_gap(int n) const;            // how far the diet eaten is from the one the people are used to
    float project_cost(int prov, int type) const;
    bool can_project(int n, int prov, int type, std::string* why = nullptr) const;
    bool start_project(int n, int prov, int type);
    bool law_ok(int n, int group, int opt) const;
    bool set_policy(int n, int group, int opt, bool free = false);
    void set_budget(int n, int cat, float share);   // other shares scale to keep the sum at 1

    // ---- espionage (intel.cpp)
    void tick_intel();
    float intel_on(int n, int t) const;    // 0-100: own network, or what allies and ententes share
    int max_agents(int n) const;
    int agents_total(int n) const;         // waiting + posted
    float agent_cost(const Agent& a) const;   // influence to hire
    void count_agents(int n);
    void refresh_candidates(int n);
    bool hire_agent(int n, int candidate);
    bool train_agent(int n);               // the AI's way: hire the best candidate it can afford
    bool post_agent(int n, int t, int delta);   // +1 sends the best waiting agent to t, -1 recalls the weakest
    bool post_agent_id(int n, int agent_id, int t);   // t = -1: home
    bool dismiss_agent(int n, int agent_id);

    // ---- AI (ai.cpp)
    void ai_daily(int n);
    void ai_weekly(int n);
    void ai_military(int n);
    void ai_diplomacy(int n);
    void ai_air(int n);
    void ai_production(int n);
    void ai_recruit(int n);
    int air_cap(int n) const;          // how many wings the AI keeps
};
