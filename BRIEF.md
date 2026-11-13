# Briefing: the office room (Great War 1906, Godot 4.7 GDScript, Forward+)

You are helping another assistant who has the computer. You cannot run anything: you write code changes, the user carries them back, the other assistant applies them, takes screenshots and reports what they show. Be exact.

## Files you have

- `BRIEF.md`: this file. `godot.md`: the art direction; read it first, it overrides your defaults.
- The room code:
  - `office_set.gd`: builds the room (walls, floor, windows, curtains, door, bookcases, furniture, the desk lamp). Materials are made at the top (`m_dark`, `m_panel`, `m_floor`, `m_paper` (wallpaper), `m_brass`, `m_leather` (desk pad), `m_hide`, `m_cloth` (curtains)).
  - `office.gd`: extends `office_set.gd`. Lights, environment (`Environment`: SDFGI, SSAO, volumetric fog, glow, filmic tonemap), camera, the clickable things on the desk, the cigar and smoke, dust.
  - `room_kit.gd`: shared helpers: `_pbr(texture, tiles, tint, rough)` (a PBR material from `assets/tex/<name>_diff/_nor/_rough.jpg`, triplanar), `_model(parent, name, at, turn, height, anchor, only)` (a scanned Poly Haven model from `assets/models/<name>/<name>.gltf`), `_tint(node, colour)` (sets albedo_color on every surface of a model), `_rbox`, `_slab`, `_lathe`, `_rod`, `_polished`, `_mat`, `_put`.
  - `ambience.gd`: things happening outside (clouds dim the window light, a plane, a carriage). It scales the window lamps (`office.sky_lights`) from the energy they start with.
  - `window_view.gdshader` (the town seen through the windows), `smoke.gdshader`, `cigar.gdshader`.
- `models.txt`: the Poly Haven models (first line) and textures (second line) that are already downloaded. Only these exist on disk; anything else must be fetched first, so name it and ask instead of using it.
- `pictures/`: the room now. `desk_overview.png`, `desk_close.png`, `window_wall.png`, `bookcases.png`.

## Already done today (keep it)

Wallpaper tinted to grey-green, curtains dark brown (their velvet colour map is dropped on purpose, it is magenta), armchairs worn brown, desk pad dark olive-brown, lamp shade dull olive, brass tarnished, desk cases oxblood and slate, rug grey-brown, the windows show `window_view.gdshader` instead of a white glow. The two window lamps (`sky` in office.gd) went from energy 1.5 to 4.5 because the old white glass was lighting the room: the bounced light (SDFGI) takes light only from emissive surfaces the camera can see; a hidden or culled emitter adds nothing.

## Tasks (do them in this order, each as its own set of edits)

1. **Glossy floor.** In `desk_close.png` the herringbone parquet is a mirror. Make it a worn, waxed office floor: soft, broad highlights, no sharp reflections. Change the floor material only (`m_floor`, and if needed the environment's reflection settings, with your reason).
2. **Book spines repeat.** In `bookcases.png` the books on the shelves show an obvious repeating pattern. Find where the bookcase rows are filled (`_bookcase` and the book set `book_encyclopedia_set_01`, with `bindings` tints) and break the repetition: vary which volume, its tint, height, a lean or a gap, deterministically (the code uses hashes such as `_h(k, n)`, never the random generator, so the room is the same every run).
3. **Flat rooftops in the windows.** In `window_wall.png` the town outside is flat grey cut-outs. In `window_view.gdshader`, give it a little depth while keeping it hazy and muted: a third, farthest row fainter still, a few lit or darker windows on the near row, slight variation of roof tone between houses, maybe a church spire or chimney smoke. Keep it a pure function of UV and `seed`, no textures. It must stay dim (it is emissive and lights the room) and low contrast; do not raise `brightness`.

For each task, keep the code style: tabs, short why-comments only, real-world sizes in metres, no new files.

## What to send back

For each task, exact find-and-replace pairs:

    FILE: office_set.gd
    FIND:
    <exact current lines, enough to be unique, tabs as in the file>
    REPLACE:
    <new lines>

Then for each task an EXPECTED list: what the matching picture should show afterwards. If something in the files contradicts this briefing, say so instead of guessing.
