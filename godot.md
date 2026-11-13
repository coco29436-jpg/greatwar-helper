# CLAUDE.md (greatwar-godot): visuals

Guidance for fixing and polishing how the Godot game looks. Display only: never change `greatwar3d/src/sim/` or `native/src/gw_sim.cpp` for a visual reason. The sim is linked as a GDExtension; everything on screen is GDScript `Control` nodes and 3D nodes in `scripts/`. The raylib UI in `greatwar3d/src/ui/` and `src/app/` is NOT part of this game; ignore it.

## Where things live

- Menus: `menu.gd` (title and pause), `nation_pick.gd`.
- Panels: `sheet.gd` is the paper-sheet base class. Politics, ledger, order book, academy and the other desk sheets build on it. A change there reaches every sheet.
- Province card and map overlays: `paper_map.gd`, `map_units.gd`.
- Top clock strip: built in `hq.gd`.
- Scene: Godot's high-quality renderer (real-time bounced light, soft contact shadows, light haze). The "A plain room" setting turns these off.
- Assets: Poly Haven scanned models and materials (CC0), fetched by a script. Everything Poly Haven lacks (desk, lamp, bell, globe, door, bookcases, curtains, papers) is built in code. Custom shaders: the paper map and the smoke. Desk papers (newspaper, programme, census...) are laid out off screen and applied to the desk.
- Fonts: Fira Sans Condensed (text), Playfair Display SC (titles). Don't change.
- No Blender, no hand-drawn art, no image editor. Anything not scanned or downloaded is made in code.

## Target look

Early 20th century, 1900s to 1910s: institutional and utilitarian, **not belle époque**. The world of ledgers, telegrams, ministry files, blueprints, newsprint and war-office maps. Worn, handled, slightly dusty. Restraint over decoration.

**Palette:** ink black, sepia and warm browns, off-white and cream paper, desaturated olive and grey-green, tarnished brass in small doses. Nothing else is saturated. **Nation colours are the only strong colour in the game**, so they must always read clearly against the muted scene. If a change adds colour that isn't a nation's, remove it.

Red is a nation colour too, so it is not an accent: no red headings, no red status text, no red title. Emphasis is ink black or dark sepia. Red appears only as rubber-stamp ink (a warning, a SECRET stamp) or as a nation's colour.

**Avoid:** gilding, scrollwork, ornate borders, Art Nouveau curves, pastels, rich purples and crimsons, glossy or clean surfaces, rounded app-style corners and pill buttons, anything that looks like a modern app or a game HUD.

Scanned Poly Haven assets and code-made shapes must look like they belong to one room. A clean, flat code-made object next to a scanned one is the most common failure.

## Menus and panels

Never a plain or single-solid-colour background, and never a rounded rectangle. Every menu and panel is a physical object on the desk: paper, card, cloth or leather, with a material and wear. Paper has square corners, slightly worn.

- Give each kind of content its own object type and keep it consistent: ledger page (economy, production, academy), cable or telegram form (events, diplomacy), ministry file folder with a tab (country, politics), index card (small tooltips), blueprint sheet (designer), map-table cloth (the map), a worn printed card or poster for the title menu and nation pick.
- Surfaces: paper grain, fold creases, edge wear, faint stains, ruled lines, a soft drop shadow so each sheet sits on the desk. Use Poly Haven paper, cardboard, leather and fabric materials where they exist; otherwise procedural noise in a shader. Generated images (ComfyUI) are for plain seamless material only, never for text or layout.
- Headings look printed: typewriter, letterpress or rubber stamp. Titles in Playfair Display SC, data in Fira Sans Condensed. No decorative borders; use rules, tabs and stamps.
- Buttons are paper slips, label tabs or rubber stamps, not rounded tan rectangles. Hover darkens the ink, pressed shifts the slip a pixel. Disabled is faded ink.
- Nation colour appears as ink: stamps, ribbons, tab edges, flags. It must contrast with the muted paper.
- Legibility first. The texture under text stays low contrast, text is dark ink on light stock (or light on dark stock), and nothing noisy sits behind numbers.
- Clips, stamps, tabs and stains add depth but must never cover information or clickable areas.
- Put shared look-and-feel in `sheet.gd` (paper material, shadow, wear, header style, button style) so every sheet gets it. After changing `sheet.gd`, re-shoot every sheet type, not just one.
- The clock strip, the map's top bar (back button, search field, mode tabs), the province card and the tooltips follow the same rules: they are card, tag or label-holder objects, not flat pills.

## Capture workflow

The game screenshots itself; no screen-capture tool is needed. It needs a display and a GPU (it shows a real window).

1. `.tools/shot.sh NAME "steps"` starts Godot (the Flatpak) with `GW_SHOT=.tools/shots/NAME.png` and `GW_ACT="steps"`. It kills the game after 60 s if it hangs.
2. `hq.gd` runs the steps in order, like a player: `pick:map`, `click:x:y`, `wait:s`, `advance:n`, `press:ButtonText`, and more (see `hq.gd` for the full list).
3. At the end `hq.gd` (around line 797) saves the rendered window frame and the game quits.
4. Open the PNG with the Read tool and look at it. Don't judge a visual change from the code.

Variations: `GW_PRE` runs steps before the desk (title page, nation choice) and can take shots there with `shot:file`. `GW_TITLE=1` starts on the title page.

The Flatpak can't write to `/tmp`; pictures go in `.tools/shots/`.

To judge small text or a texture, crop and enlarge the area of the PNG (ImageMagick or PIL) and look at that crop too.

### Standard shot set

Keep one script, `.tools/shots_all.sh`, that takes these (create it if it doesn't exist) so before and after are always comparable:
desk overview, desk close-up (papers and lamp), window wall, bookcases corner, title page, settings, nation pick, each desk sheet (Academy, Works Order Book, Politics, Ledger, and the others), map with a province card, map with a battle card, map zoomed to a front.
Run it with "A plain room" off, and a second time with it on.

## Verify every visual change

1. Shoot the affected scenes, before touching anything.
2. Describe what you see and list the problems.
3. Change one thing at a time, re-shoot, compare with the previous shot. Don't stack changes.
4. If a problem stays with "A plain room" on, it is geometry or material, not lighting.
5. After a change to `sheet.gd` or a shared style, run the whole shot set.
6. Keep approved shots in `.tools/shots/approved/` and compare against them.

## Known problems, fix these first

From the current screenshots, in priority order:

1. **Panels are flat cream rounded rectangles.** Every sheet, the province card, the clock strip, the map's tab strip and search box, the settings and title menus. This is the main thing to replace (see Menus and panels).
2. **Red and orange off-palette.** Sheet headings and the "Idle. Nothing is being studied here." lines are dark red; the "1906" on the title page is bright orange-red.
3. **Room palette reads Victorian or belle époque.** Crimson curtains and armchairs, green walls, lamp and desk pad, a blue case, a gilt picture frame, an ornate black carved console, a gilt chandelier. Pull saturation down (tint materials) and prefer plainer models. Show a before and after, and ask before replacing a Poly Haven model.
4. **Windows are blown-out pure white.** No exterior at all. Give them a muted, hazy outside (fog, building silhouettes) and lower the exposure there.
5. **Layout bugs.**
   - "Subjects within reach, for Laboratory 1" wraps and leaves a lone "1" on its own line.
   - The Settings sheet covers the "1906" title and the footer line.
   - The province card runs off the top of the screen on the battle view and covers the tab strip.
   - In the battle card, "Austro-Hungarian defence" collides with "Defence in depth (read it)".
   - The fps counter overlaps the "Unrest" tab (debug only; hide it in shots).
6. **Map legibility.** The help line at the bottom is light cream text straight on the map, hard to read. Nation names sit under unit counters.
7. **Nation colours on the map are pale.** Check that neighbouring nations separate clearly while the rest of the map stays muted.

## Rules

- Poly Haven files come from the download script. Don't edit the downloaded files; change import settings or the script.
- Code-made shapes: real-world scale in metres, bevelled edges, PBR materials with roughness variation, no pure flat colours, texel density matched to the scanned assets.
- Desk papers are laid out off screen and applied to the desk. Fix the layout, not the resulting texture.
- Custom shaders (paper map, smoke): edit, shoot, compare. Don't rewrite one without a before shot.
- No new always-on full-screen effect without checking the frame rate.

## Common problems to check each time

- Light leaking through walls or under furniture.
- Over-bright emissive surfaces; bloom washing out highlights.
- Haze too thick or too flat.
- Dark halos from ambient occlusion.
- Shadow acne or floating shadows; objects without contact shadows.
- Visible texture tiling on wood and plaster.
- Scale mismatches between scanned and code-made objects.
- Text clipped, wrapped badly or overlapping at any panel size.
- Panels that read as flat colour, or colour that isn't a nation's.
