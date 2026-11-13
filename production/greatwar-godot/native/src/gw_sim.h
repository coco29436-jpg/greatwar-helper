// The simulation of Great War 1906, as a Godot object. Godot draws and takes the clicks; every rule stays in src/sim.
#pragma once

#include <memory>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/string.hpp>

struct World;

class GreatWarSim : public godot::RefCounted {
    GDCLASS(GreatWarSim, godot::RefCounted)

    std::unique_ptr<World> w;
    godot::String last_error;

protected:
    static void _bind_methods();

public:
    GreatWarSim();
    ~GreatWarSim();

    bool load_world(const godot::String& data_dir);     // data_dir: the game's data/ folder
    godot::String error() const { return last_error; }
    bool start_as(const godot::String& tag);            // false if there is no such nation
    int advance_days(int days);                         // the game's daily tick; stops early when something waits for an
                                                        // answer, and says how many days went by

    godot::String date() const;
    float hour() const;
    void set_hour(float hour);
    godot::Dictionary nation() const;                   // the player: name, tag, money, factories at work and in all
    godot::Array lines() const;                         // the production lines
    bool change_factories(int line, int delta);         // same rules as the Production screen
    godot::Array designs() const;                       // what the player can put on a line
    bool open_line(int design);                         // one idle factory starts on it, or joins the line it has
    bool close_line(int line);

    godot::Dictionary pending() const;                  // the event waiting for an answer, or empty
    bool answer(int option);

    // ---- the papers on the desk: what each one says today. Reading only.
    godot::Array reports();                             // research and focuses finished since the last call
    godot::Array news(int count) const;                 // the newest first
    godot::Array relations() const;                     // every nation that matters to the player
    godot::Dictionary espionage() const;
    godot::Dictionary budget() const;
    godot::Dictionary politics() const;
    godot::Dictionary society() const;
    godot::Dictionary focus() const;
    godot::Dictionary research() const;
    godot::Dictionary forces() const;
    godot::Dictionary capital() const;
    godot::Array nations() const;
    godot::Array laws() const;
    godot::String set_law(const godot::String& group, int option);
    godot::Array ministers(int post) const;
    godot::String appoint(int minister);
    godot::Array decisions() const;
    bool take_decision(int id);
    godot::Array programmes() const;
    bool start_programme(int id);
    bool campaign(int ideology);
    godot::Array candidates() const;
    bool hire_agent(int index);
    bool post_agent(const godot::String& tag, int by);
    bool fleet_mission(int fleet, int mission);
    godot::Array builds(int province) const;
    godot::String build(int province, int type);
    godot::Array build_queue() const;
    bool cancel_build(int index);
    bool raise_build(int index);
    godot::Array market() const;
    godot::String buy(const godot::String& tag, int resource, float amount);
    bool cancel_deal(int index);
    godot::Array air_designs() const;
    bool form_wing(int design);
    bool start_focus(int id);
    godot::Array techs_open() const;
    bool start_research(int slot, int tech, bool experimental);
    bool shift_budget(int category, float by);
    godot::String diplomacy(const godot::String& action, const godot::String& tag);
    godot::Array templates() const;
    godot::String recruit(int tmpl);

    // ---- the map
    godot::PackedByteArray province_ids(int step) const;    // two bytes a cell, province + 1 (0 the sea), every step-th cell
    godot::PackedByteArray province_palette() const;        // PALETTE_W x 3 RGBA: owner colour, owner id, occupier colour
    godot::Dictionary province(int id) const;
    godot::Array places() const;                            // nation names and cities to write on the map
    int map_width() const;
    int map_height() const;
    void set_map_mode(int mode);                            // what the wash shows: 0 nations, 1 our relations, 2 terrain, 3 industry, 4 unrest
    godot::Dictionary map_units() const;                    // what stands on the map and may be seen: stacks of divisions, fleets
    godot::Array battles() const;                           // the battles the player may see, as the province card and the map show them
    godot::String declare_war_now(const godot::String& tag);    // tests only: war at once, no claim needed
    godot::String move_divisions(const godot::PackedInt32Array& ids, int province);    // march there; "" or why not
    godot::String sail_fleet(int fleet, int province);      // "" or why not
    godot::Dictionary country(const godot::String& tag) const;    // a foreign government, as the map's card shows it

    // ---- the game on disk
    bool save_game(const godot::String& path) const;
    godot::String load_game(const godot::String& path);     // "" or why not

private:
    int map_mode = 0;
};
