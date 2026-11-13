class_name PaperMap
extends Control
## The world map as a printed sheet (assets/shaders/paper_map.gdshader): drag to move, wheel to zoom, click a province
## for its card. `wall` makes the small still copy that hangs in the office. On the working map the armies and fleets
## stand as counters (map_units.gd): click ours to take them up, right-click a province to send them there. The strip
## along the top lets days go by, changes what the wash shows and finds a place by name.

const SHEET := Vector2(1600, 900)
const UPP_WORLD := TAU / 1600.0         # the whole world across the sheet
const UPP_CLOSE := 0.00022              # about three pixels to a map cell
const Y_MAX := 2.6                      # Mercator y of the top margin
const MODE_NOTES := ["Every nation in its own colour; colonies paler.",
		"Blue: ours. Teal: allies. Green to red: how each government stands with us. Dark red: at war.",
		"Plains, forest, hills, mountains, desert, jungle and ice.",
		"Darker where the factories and arms works stand.",
		"Red where the people are close to rising."]

signal chosen(tag: String)             # the country to take office in, while `choosing`
signal nation_clicked(tag: String)     # while `choosing`: a country was clicked on the map

var wall := false
var choosing := false
var centre := Vector2(0.17, 0.62)       # longitude in radians, Mercator y: Europe a little above the middle
var upp := UPP_WORLD
var mat: ShaderMaterial
var names: Control
var units: Control                      # the counters (map_units.gd)
var card: PanelContainer
var places: Array = []
var hover := -1
var picked := -1
var focus_owner := 0                    # choosing: the nation that stands out
var hover_owner := 0
var dragging := false
var dragged := 0.0
var drawn_for := Vector3.ZERO           # the view the names were laid out for
var name_items: Array = []              # the names as laid out: {text, at, font, size, colour, outline, edge}
var sea_font: FontVariation             # seas are lettered in a sloping hand
var quiet_left := 0.0                   # no names left of this x: the title page stands there
var flight: Tween
var stacks: Array = []                  # sim.map_units(): divisions by province and nation
var fleets: Array = []
var battles: Array = []                 # sim.battles(): the provinces under assault that we may see
var sel := {}                           # ids of our divisions in hand
var sel_fleet := -1
var boxing := false                     # shift-drag: a box round our counters
var box_from := Vector2.ZERO
var box_to := Vector2.ZERO
var bar: Control                        # the strip along the top
var mode_note: Label
var mode_buttons: Array = []
var find: LineEdit
var tip: Label
var hand: PanelContainer                # what is in hand, at the lower left


func _ready() -> void:
	Game.need_map()
	size = SHEET
	clip_contents = true
	var sheet := ColorRect.new()
	sheet.size = SHEET
	sheet.mouse_filter = Control.MOUSE_FILTER_IGNORE
	mat = ShaderMaterial.new()
	mat.shader = load("res://assets/shaders/paper_map.gdshader")
	mat.set_shader_parameter("ids", Game.map_ids)
	mat.set_shader_parameter("palette", Game.map_palette)
	mat.set_shader_parameter("sheet", SHEET)
	mat.set_shader_parameter("wall", 1.0 if wall else 0.0)
	sheet.material = mat
	add_child(sheet)
	names = Control.new()
	names.mouse_filter = Control.MOUSE_FILTER_IGNORE
	names.draw.connect(_draw_names)
	add_child(names)
	sea_font = FontVariation.new()
	sea_font.base_font = Game.font_title
	sea_font.variation_transform = Transform2D(Vector2(1, 0), Vector2(-0.22, 1), Vector2.ZERO)
	sea_font.spacing_glyph = 3
	places = Game.sim.places()
	places.sort_custom(func(a, b): return a.weight > b.weight)
	Game.changed.connect(_on_changed)
	if wall:
		mouse_filter = Control.MOUSE_FILTER_IGNORE
	else:
		units = load("res://scripts/map_units.gd").new()
		units.set("map", self)
		units.size = SHEET
		units.mouse_filter = Control.MOUSE_FILTER_IGNORE
		add_child(units)
		_build_bar()
		tip = RoomKit._label("", Game.font_text, 15, Sheet.INK)
		tip.autowrap_mode = TextServer.AUTOWRAP_OFF
		var back := StyleBoxFlat.new()
		back.bg_color = Color(0.93, 0.89, 0.78, 0.97)
		back.border_color = Color(0.13, 0.09, 0.05, 0.55)
		back.set_border_width_all(1)
		back.border_width_bottom = 2
		back.shadow_color = Color(0, 0, 0, 0.3)
		back.shadow_size = 6
		back.shadow_offset = Vector2(2, 3)
		back.set_content_margin_all(4)
		back.content_margin_left = 8
		back.content_margin_right = 8
		tip.add_theme_stylebox_override("normal", back)
		tip.mouse_filter = Control.MOUSE_FILTER_IGNORE
		tip.visible = false
		add_child(tip)
		visibility_changed.connect(_on_changed)
	_apply()


func _on_changed() -> void:
	if not is_inside_tree() or (not wall and not visible):
		return                              # a map nobody is looking at is written when it is next shown
	places = Game.sim.places()
	places.sort_custom(func(a, b): return a.weight > b.weight)
	drawn_for = Vector3.ZERO
	if not wall:
		_read_units()
		bar.visible = not choosing
	if picked >= 0:
		_show_card(picked)
	_apply()


## what stands on the map today; what was in hand and is no more is let go
func _read_units() -> void:
	if choosing:
		stacks = []
		fleets = []
		battles = []
		return
	var u: Dictionary = Game.sim.map_units()
	stacks = u.stacks
	fleets = u.fleets
	battles = Game.sim.battles()
	var alive := {}
	for st in stacks:
		if st.ours:
			for id in st.ids:
				if sel.has(id):
					alive[id] = true
	sel = alive
	if sel_fleet >= 0 and not fleets.any(func(f): return f.ours and f.fleet == sel_fleet):
		sel_fleet = -1
	_show_hand()


# ---- the strips along the top: a place by name on the left, what the wash shows on the right (the clock between them
# is the headquarters')

func _build_bar() -> void:
	bar = Control.new()
	bar.size = SHEET
	bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(bar)
	var right := _strip_box()
	for i in Game.MAP_MODES.size():
		var b := _bar_button(Game.MAP_MODES[i], set_mode.bind(i), MODE_NOTES[i])
		b.toggle_mode = true
		b.button_pressed = i == Game.map_mode
		mode_buttons.append(b)
		right.add_child(b)
	_place(right, 1.0, 14)
	mode_note = RoomKit._label("", Game.font_text, 14, Color(0.16, 0.1, 0.06, 0.85))
	mode_note.autowrap_mode = TextServer.AUTOWRAP_OFF
	mode_note.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	mode_note.position = Vector2(SHEET.x - 720, 62)
	mode_note.size = Vector2(700, 20)
	mode_note.mouse_filter = Control.MOUSE_FILTER_IGNORE
	mode_note.add_theme_constant_override("outline_size", 6)
	mode_note.add_theme_color_override("font_outline_color", Color(0.95, 0.9, 0.78, 0.95))
	bar.add_child(mode_note)
	var left := _strip_box()
	find = LineEdit.new()
	find.placeholder_text = "Find a country or a town"
	find.custom_minimum_size = Vector2(230, 0)
	find.add_theme_font_override("font", Game.font_text)
	find.add_theme_font_size_override("font_size", 17)
	find.add_theme_color_override("font_color", Sheet.INK)
	find.add_theme_color_override("font_placeholder_color", Color(Sheet.INK, 0.45))
	find.add_theme_color_override("caret_color", Sheet.INK)
	var plain := StyleBoxFlat.new()
	plain.bg_color = Color(1, 1, 1, 0.35)
	plain.set_content_margin_all(5)
	find.add_theme_stylebox_override("normal", plain)
	find.add_theme_stylebox_override("focus", plain)
	find.text_submitted.connect(_find)
	left.add_child(find)
	left.add_child(_bar_button("The world", func(): fly_to_view(Vector2(0.17, 0.62), UPP_WORLD), "The whole world on the sheet  (Home)"))
	left.get_parent().position = Vector2(210, 14)


func _strip_box() -> HBoxContainer:
	var p := PanelContainer.new()
	var back := StyleBoxFlat.new()
	back.bg_color = Color(0.93, 0.89, 0.78, 0.97)
	back.border_color = Color(0.13, 0.09, 0.05, 0.55)
	back.set_border_width_all(1)
	back.border_width_bottom = 2
	back.set_content_margin_all(6)
	back.shadow_color = Color(0, 0, 0, 0.3)
	back.shadow_size = 6
	back.shadow_offset = Vector2(2, 3)
	p.add_theme_stylebox_override("panel", back)
	var h := HBoxContainer.new()
	h.add_theme_constant_override("separation", 6)
	p.add_child(h)
	bar.add_child(p)
	return h


## a strip against the top of the sheet: `along` 0.5 centres it, 1 puts it at the right edge
func _place(h: HBoxContainer, along: float, y: float) -> void:
	var p := h.get_parent() as Control
	p.reset_size()
	await get_tree().process_frame
	if is_instance_valid(p):
		p.reset_size()
		p.position = Vector2((SHEET.x - p.size.x) * along - (20.0 if along > 0.9 else 0.0), y)


func _bar_button(label: String, act: Callable, about: String) -> Button:
	var b := Button.new()
	b.text = label
	b.tooltip_text = about
	b.focus_mode = Control.FOCUS_NONE
	b.add_theme_font_override("font", Game.font_text)
	b.add_theme_font_size_override("font_size", 17)
	for k in ["font_color", "font_focus_color"]:
		b.add_theme_color_override(k, Color(Sheet.INK, 0.86))
	for k in ["font_hover_color", "font_pressed_color", "font_hover_pressed_color"]:
		b.add_theme_color_override(k, Color(0.03, 0.02, 0.01))
	for state: Array in [["normal", "normal"], ["hover", "hover"], ["pressed", "pressed"], ["hover_pressed", "pressed"]]:
		var box := Sheet.slip(state[1])
		box.set_content_margin_all(5)
		box.content_margin_left = 11
		box.content_margin_right = 11
		if state[1] == "pressed":
			box.content_margin_top = 6
			box.content_margin_bottom = 4
		b.add_theme_stylebox_override(state[0], box)
	b.pressed.connect(act)
	return b


func set_mode(mode: int) -> void:
	Game.set_map_mode(mode)
	for i in mode_buttons.size():
		(mode_buttons[i] as Button).button_pressed = i == mode
	mode_note.text = MODE_NOTES[mode] if mode > 0 else ""


## the country or town whose name starts with (or holds) what was typed comes to the middle of the sheet
func _find(text: String) -> void:
	var want := text.strip_edges().to_lower()
	find.release_focus()
	if want == "":
		return
	var best := {}
	for pass_no in 2:
		for pl in places:
			var n := String(pl.name).to_lower()
			if (pass_no == 0 and n.begins_with(want)) or (pass_no == 1 and n.contains(want)):
				best = pl
				break
		if not best.is_empty():
			break
	if best.is_empty():
		Game.said.emit("No country or town on the map is called \"%s\"." % text.strip_edges())
		return
	find.text = ""
	fly_to(best.lon, best.lat, UPP_WORLD / (5.0 if best.nation else 12.0))
	if not best.nation:
		await flight.finished
		pick(Game.province_at(best.lon, best.lat))


## the view as the whole world, as it hangs on the wall
func world_view() -> void:
	centre = Vector2(0.17, 0.62)
	upp = UPP_WORLD
	_apply()


func look_at_place(lon: float, lat: float, units_per_px: float) -> void:
	centre = Vector2(deg_to_rad(lon), _merc(deg_to_rad(lat)))
	upp = units_per_px
	_apply()


## the sheet slides and closes in until that place is in the middle
func fly_to(lon: float, lat: float, units_per_px: float, time := 0.7) -> void:
	fly_to_view(Vector2(deg_to_rad(lon), _merc(deg_to_rad(lat))), units_per_px, time)


func fly_to_view(to: Vector2, units_per_px: float, time := 0.7) -> void:
	if flight and flight.is_running():
		flight.kill()
	var from := centre
	var from_upp := upp
	var to_upp := clampf(units_per_px, UPP_CLOSE, UPP_WORLD)
	to.x = from.x + wrapf(to.x - from.x, -PI, PI)          # the short way round
	flight = create_tween()
	flight.tween_method(func(t: float):
		centre = from.lerp(to, t)
		upp = exp(lerpf(log(from_upp), log(to_upp), t))     # the zoom changes evenly to the eye
		_apply(), 0.0, 1.0, time).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_IN_OUT)


## a box on the globe (degrees) fills the part of the sheet between `left` and `right` pixels
func fly_to_box(lon: float, half_lon: float, south: float, north: float, left := 0.0, right := SHEET.x) -> void:
	var y0 := _merc(deg_to_rad(clampf(south, -80.0, 84.0)))
	var y1 := _merc(deg_to_rad(clampf(north, -80.0, 84.0)))
	var room := Vector2(right - left, SHEET.y) * 0.72
	var want := maxf(deg_to_rad(maxf(half_lon, 1.5)) * 2.0 / room.x, maxf(y1 - y0, 0.03) / room.y)
	want = clampf(want, UPP_CLOSE * 3.0, UPP_WORLD)
	# the middle of the box goes to the middle of the free part of the sheet
	var off := ((left + right) * 0.5 - SHEET.x * 0.5) * want
	fly_to_view(Vector2(deg_to_rad(lon) - off, (y0 + y1) * 0.5), want)


func _merc(lat: float) -> float:
	return log(tan(PI / 4.0 + lat / 2.0))


func _lat(y: float) -> float:
	return 2.0 * atan(exp(y)) - PI / 2.0


func _apply() -> void:
	upp = clampf(upp, UPP_CLOSE, UPP_WORLD)
	var half := SHEET.y * 0.5 * upp
	centre.y = clampf(centre.y, -Y_MAX + half, Y_MAX - half) if half < Y_MAX else 0.0
	centre.x = wrapf(centre.x, -PI, PI)
	mat.set_shader_parameter("centre", centre)
	mat.set_shader_parameter("upp", upp)
	mat.set_shader_parameter("hover", float(hover))
	mat.set_shader_parameter("picked", float(picked))
	mat.set_shader_parameter("strong", 1.0 if Game.map_mode > 0 and not wall and not choosing else 0.0)
	mat.set_shader_parameter("focus_owner", float(focus_owner))
	mat.set_shader_parameter("hover_owner", float(hover_owner))
	if units:
		units.queue_redraw()
	# the names are laid out for one view and slide with the sheet as a block until it has moved some way
	var slid := Vector2(wrapf(centre.x - drawn_for.x, -PI, PI), centre.y - drawn_for.y) / upp
	if upp != drawn_for.z or absf(slid.x) > 40.0 or absf(slid.y) > 40.0:
		drawn_for = Vector3(centre.x, centre.y, upp)
		_lay_names()
		names.position = Vector2.ZERO
		names.queue_redraw()
	else:
		names.position = Vector2(-slid.x, slid.y).round()


## where a place on the globe falls on the sheet
func to_sheet(lon: float, lat: float) -> Vector2:
	var dx := wrapf(deg_to_rad(lon) - centre.x, -PI, PI)
	return SHEET * 0.5 + Vector2(dx, -(_merc(deg_to_rad(lat)) - centre.y)) / upp


## the province under a point of the sheet, -1 at sea
func province_under(at: Vector2) -> int:
	var m := centre + Vector2(at.x - SHEET.x * 0.5, -(at.y - SHEET.y * 0.5)) * upp
	return Game.province_at(rad_to_deg(wrapf(m.x, -PI, PI)), rad_to_deg(_lat(m.y)))


# ---- the names: nations in spaced capitals, towns in small print, the weightiest first and none over another

func _lay_names() -> void:
	name_items.clear()
	var zoom := UPP_WORLD / upp             # 1 the whole world, 18 closest
	var taken: Array[Rect2] = []
	# where the counters stand (map_units.gd sizes) and the strips along the top: a name keeps out from under them
	var counters: Array[Rect2] = []
	if not wall:
		counters.append(Rect2(0.0, 0.0, SHEET.x, 66.0))     # the strips along the top
		var cs := Vector2(36.0, 23.0)
		for st in stacks:
			counters.append(Rect2(to_sheet(st.lon, st.lat) - cs * 0.5, cs))
		for f in fleets:
			counters.append(Rect2(to_sheet(f.lon, f.lat) + Vector2(-cs.x * 0.5, cs.y * 0.75), Vector2(cs.x, cs.y * 0.8)))
	# every nation, in letters as large as its home country allows: a great empire across half the sheet, a
	# principality in print almost too small to read
	for pl in places:
		if not pl.nation:
			continue
		var at := to_sheet(pl.lon, pl.lat)
		if at.x < -200 or at.x > SHEET.x + 200 or at.y < -60 or at.y > SHEET.y + 60:
			continue
		var word := String(pl.name).to_upper()
		var across := sqrt(maxf(float(pl.get("area", 1.0)), 1.0)) * (TAU / Game.map_w) / upp / cos(deg_to_rad(clampf(pl.lat, -70.0, 70.0)))
		var fs := clampi(int(across * 1.25 / maxf(word.length() * 0.66, 1.0)), 4 if wall else 8, 60)
		if not wall and across * 1.25 / maxf(word.length() * 0.66, 1.0) < 7.0:
			continue                            # on the working map a name waits until it can be read
		var w := Game.font_title.get_string_size(word, HORIZONTAL_ALIGNMENT_LEFT, -1, fs)
		var box := Rect2(at - w * 0.5, w)
		for k: float in [0.0, -0.7, 0.7, -1.4, 1.4]:     # a little up or down to clear the counters; if nothing clears, its own place
			var moved := Rect2(box.position + Vector2(0.0, k * w.y), w)
			if not _clashes(counters, moved.grow(-2.0)):
				box = moved
				break
		if box.position.x < quiet_left or _clashes(taken, box.grow(-2.0)):
			continue                            # the weightier nation was written first and keeps the place
		taken.append(box)
		name_items.append({"text": word, "at": box.position, "font": Game.font_title, "size": fs, "colour": Color(0.15, 0.09, 0.05, 0.86),
				"outline": clampi(fs / 4, 0, 5), "edge": Color(0.93, 0.88, 0.76, 0.55)})
	# the seas, in a sloping blue hand: the oceans always, the smaller waters as the sheet closes in
	for sea: Dictionary in Game.seas:
		var rank: int = sea.rank
		if zoom < [1.0, 2.2, 4.0, 7.0, 11.0][mini(rank, 4)] or (wall and rank > 0):
			continue
		var at := to_sheet(sea.lon, sea.lat)
		if at.x < -200 or at.x > SHEET.x + 200 or at.y < -40 or at.y > SHEET.y + 40:
			continue
		var word := String(sea.name).to_upper() if rank == 0 else String(sea.name)
		var fs: int = [30, 20, 16, 15, 14][mini(rank, 4)]
		var box := Rect2()
		var fits := false
		for attempt in 3:                       # a name that runs ashore is set smaller, and left out if it still does
			var w := sea_font.get_string_size(word, HORIZONTAL_ALIGNMENT_LEFT, -1, fs)
			box = Rect2(at - w * 0.5, w)
			fits = true
			for k in 7:
				if province_under(Vector2(box.position.x + box.size.x * k / 6.0, at.y)) >= 0:
					fits = false
					break
			if fits or fs <= 11:
				break
			fs = int(fs * 0.76)
		if not fits or box.position.x < quiet_left or _clashes(taken, box.grow(4.0)):
			continue
		taken.append(box)
		name_items.append({"text": word, "at": box.position, "font": sea_font, "size": fs, "colour": Color(0.16, 0.27, 0.4, 0.62 if rank == 0 else 0.74),
				"outline": 0, "edge": Color.WHITE})
	if wall or zoom < 3.0:
		return
	var shown := 0
	for pl in places:                           # towns, the weightiest first and none over another
		if pl.nation or (zoom < 6.0 and not pl.capital) or (zoom < 11.0 and pl.weight < 2.0):
			continue
		var at := to_sheet(pl.lon, pl.lat)
		if at.x < -80 or at.x > SHEET.x + 80 or at.y < -30 or at.y > SHEET.y + 30:
			continue
		var s := "•  " + String(pl.name)
		var w := Game.font_text.get_string_size(s, HORIZONTAL_ALIGNMENT_LEFT, -1, 15)
		var box := Rect2(at - Vector2(4.0, w.y * 0.5), w)
		if _clashes(taken, box.grow(3.0)) or _clashes(counters, box):
			continue                            # never under another name or a counter
		taken.append(box)
		name_items.append({"text": s, "at": box.position, "font": Game.font_text, "size": 15, "colour": Color(0.12, 0.08, 0.05, 0.9), "outline": 0,
				"edge": Color.WHITE})
		shown += 1
		if shown > 160:
			break


func _clashes(taken: Array[Rect2], box: Rect2) -> bool:
	for t in taken:
		if t.intersects(box):
			return true
	return false


## the names are drawn in one pass, straight on the sheet: no node for each
func _draw_names() -> void:
	for n: Dictionary in name_items:
		var f: Font = n.font
		var base: Vector2 = n.at + Vector2(0.0, f.get_ascent(n.size))
		if n.outline > 0:
			names.draw_string_outline(f, base, n.text, HORIZONTAL_ALIGNMENT_LEFT, -1, n.size, n.outline, n.edge)
		names.draw_string(f, base, n.text, HORIZONTAL_ALIGNMENT_LEFT, -1, n.size, n.colour)


# ---- the hand on the map

func _gui_input(event: InputEvent) -> void:
	if wall:
		return
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP and event.pressed:
			_zoom_at(event.position, 1.0 / 1.18)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN and event.pressed:
			_zoom_at(event.position, 1.18)
		elif event.button_index == MOUSE_BUTTON_LEFT:
			if find and find.has_focus():
				find.release_focus()
			if event.pressed:
				if event.double_click and not choosing:
					fly_to_view(_under(event.position), upp / 2.2, 0.35)
					dragging = false
					return
				dragged = 0.0
				if event.shift_pressed and not choosing:
					boxing = true
					box_from = event.position
					box_to = event.position
				else:
					dragging = true
			elif boxing:
				boxing = false
				_take_box(Rect2(box_from, box_to - box_from).abs())
				units.queue_redraw()
			elif dragging:
				dragging = false
				if dragged < 6.0:
					_click(event.position, event.shift_pressed or event.ctrl_pressed)
		elif event.button_index == MOUSE_BUTTON_RIGHT and event.pressed:
			if choosing or (sel.is_empty() and sel_fleet < 0):
				pick(-1)
			else:
				_send(province_under(event.position), event.position)
	elif event is InputEventMouseMotion:
		if boxing:
			box_to = event.position
			units.queue_redraw()
			return
		if dragging:
			dragged += event.relative.length()
			if flight and flight.is_running():
				flight.kill()
			centre += Vector2(-event.relative.x, event.relative.y) * upp
		var h := province_under(event.position)
		var ho := Game.owner_id(h) if choosing else 0
		_tell(event.position, h)
		if h != hover or dragging or ho != hover_owner:
			hover = h
			hover_owner = ho
			_apply()


func _unhandled_key_input(event: InputEvent) -> void:
	if wall or choosing or not is_visible_in_tree() or not event.pressed or (find and find.has_focus()):
		return
	var step := 140.0 * upp
	match (event as InputEventKey).keycode:
		KEY_LEFT, KEY_A: centre.x -= step
		KEY_RIGHT, KEY_D: centre.x += step
		KEY_UP, KEY_W: centre.y += step
		KEY_DOWN, KEY_S: centre.y -= step
		KEY_EQUAL, KEY_PLUS, KEY_KP_ADD: upp /= 1.3
		KEY_MINUS, KEY_KP_SUBTRACT: upp *= 1.3
		KEY_HOME: fly_to_view(Vector2(0.17, 0.62), UPP_WORLD)
		_: return
	accept_event()
	_apply()


## the place on the map under a point of the sheet
func _under(at: Vector2) -> Vector2:
	return centre + Vector2(at.x - SHEET.x * 0.5, -(at.y - SHEET.y * 0.5)) * upp


## a word beside the pointer: what is under it
func _tell(at: Vector2, province: int) -> void:
	if tip == null:
		return
	var text := ""
	if not dragging and not boxing:
		var c: Dictionary = units.call("counter_at", at)
		if not c.is_empty():
			text = c.text
		elif province >= 0 and upp < UPP_WORLD / 2.5 and not choosing:
			var d: Dictionary = Game.sim.province(province)
			text = "%s  ·  %s" % [d.name, d.owner]
			if not sel.is_empty() or sel_fleet >= 0:
				text += "\nRight-click: send them here"
		elif choosing and province >= 0:
			text = Game.sim.province(province).owner
	tip.visible = text != ""
	if text != "":
		tip.text = text
		tip.reset_size()
		tip.position = Vector2(minf(at.x + 18.0, SHEET.x - tip.size.x - 8.0), minf(at.y + 20.0, SHEET.y - tip.size.y - 8.0))


func _click(at: Vector2, add: bool) -> void:
	if choosing:
		var p := province_under(at)
		if p >= 0:
			var tag := String(Game.sim.province(p).get("owner_tag", ""))
			if tag != "":
				nation_clicked.emit(tag)
		return
	var c: Dictionary = units.call("counter_at", at)
	if not c.is_empty() and c.ours:
		if c.has("fleet"):
			sel.clear()
			sel_fleet = -1 if sel_fleet == c.fleet else int(c.fleet)
		else:
			sel_fleet = -1
			var all_in := true
			for id in c.ids:
				if not sel.has(id):
					all_in = false
			if not add:
				sel.clear()
			for id in c.ids:
				if add and all_in:
					sel.erase(id)
				else:
					sel[id] = true
		pick(-1)
		_show_hand()
		units.queue_redraw()
		return
	if not add:
		sel.clear()
		sel_fleet = -1
		_show_hand()
	pick(int(c.province) if not c.is_empty() else province_under(at))


## every counter of ours inside the box is taken up
func _take_box(r: Rect2) -> void:
	sel.clear()
	sel_fleet = -1
	for st in stacks:
		if st.ours and r.has_point(to_sheet(st.lon, st.lat)):
			for id in st.ids:
				sel[id] = true
	pick(-1)
	_show_hand()


## what is in hand marches (or sails) to that province
func _send(province: int, at: Vector2) -> void:
	if sel_fleet >= 0:
		if province < 0:              # a click on the water: the nearest coast will do
			for r: float in [14.0, 30.0, 50.0]:
				for k in 8:
					var q := province_under(at + Vector2.from_angle(k * PI / 4.0) * r)
					if q >= 0:
						province = q
						break
				if province >= 0:
					break
		if province < 0:
			Game.said.emit("A fleet is sent to a coast: click the land it should lie off.")
			return
		Sheet.order(Game.sim.sail_fleet(sel_fleet, province))
		return
	if province < 0:
		Game.said.emit("Divisions are sent to a province: click the land, and the ships will be found if a sea lies between.")
		return
	Sheet.order(Game.sim.move_divisions(PackedInt32Array(sel.keys()), province))
	units.call("flash", at)


# ---- what is in hand, at the lower left

func _show_hand() -> void:
	if hand:
		hand.queue_free()
		hand = null
	if sel.is_empty() and sel_fleet < 0:
		return
	var s := Sheet.new(Color(1.0, 0.97, 0.88), Vector2(330, 0), 22)
	hand = s
	if sel_fleet >= 0:
		for f in fleets:
			if f.ours and f.fleet == sel_fleet:
				s.title(f.name, 22)
				s.note("%d ships  ·  %s" % [f.ships, f.mission], 15)
				s.rule(1, 0.3)
				s.row("Where", ("sailing to %s, %d days" % [f.dest_name, ceili(f.days)]) if f.sailing else "off " + String(f.place), Sheet.INK, 16)
		s.para("Right-click a coast to send the fleet there.", 14, Sheet.SOFT)
	else:
		var kinds := {}
		var where := {}
		var marching := 0
		var training := 0
		for st in stacks:
			if not st.ours:
				continue
			for k in st.ids.size():
				if sel.has(st.ids[k]):
					kinds[st.names[k]] = int(kinds.get(st.names[k], 0)) + 1
					where[st.place] = true
			var mine := 0
			for id in st.ids:
				if sel.has(id):
					mine += 1
			if mine > 0:
				marching += mini(mine, st.moving)
				training += mini(mine, st.training)
		s.title("%d division%s in hand" % [sel.size(), "" if sel.size() == 1 else "s"], 22)
		var spots := where.keys()
		s.note(spots[0] if spots.size() == 1 else "in %d provinces" % spots.size(), 15)
		s.rule(1, 0.3)
		for k in kinds:
			s.row(k, str(kinds[k]), Sheet.INK, 16)
		if marching > 0:
			s.row("On the march", str(marching), Sheet.SOFT, 15)
		if training > 0:
			s.row("Still training", str(training), Sheet.BAD, 15)
		s.para("Right-click a province to send them there.", 14, Sheet.SOFT)
		s.act_row("", "", [s.button("Halt", _halt, marching > 0, "Stop where they stand"), s.button("Put down", func():
			sel.clear()
			_show_hand()
			units.queue_redraw())], Sheet.INK, 15)
	s.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(s)
	_settle(s, 24.0, 46.0)


## every division in hand stops where it stands
func _halt() -> void:
	for st in stacks:
		if not st.ours:
			continue
		var ids := PackedInt32Array()
		for id in st.ids:
			if sel.has(id):
				ids.append(id)
		if not ids.is_empty():
			Game.sim.move_divisions(ids, st.province)
	Game.touch()


func _zoom_at(at: Vector2, factor: float) -> void:
	if flight and flight.is_running():
		flight.kill()
	var off := Vector2(at.x - SHEET.x * 0.5, -(at.y - SHEET.y * 0.5))
	var under := centre + off * upp
	upp = clampf(upp * factor, UPP_CLOSE, UPP_WORLD)
	centre = under - off * upp              # what was under the pointer stays under it
	_apply()


func pick(p: int) -> void:
	picked = -1 if choosing else p
	if p < 0 or choosing:
		if card:
			card.queue_free()
			card = null
	else:
		_show_card(p)
	_apply()


## The battle for a province, on its card: the phase, what each general is trying, how the two exchanges stand
## (the attack's fire against the defence's cover, the defence's fire against the attack's drive), and the cost.
func _battle_on_card(s: Sheet, b: Dictionary) -> void:
	s.heading("Battle  ·  day %d  ·  %s" % [b.day, b.phase_name])
	s.row(b.attacker_adj + " attack", b.attack_tactic, Sheet.INK, 16)
	s.row(b.defender_adj + " defence", b.defence_tactic + ("  (read it)" if b.read else ""), Sheet.BAD if b.read else Sheet.INK, 16)
	var ours_attack: bool = b.we_attack or not b.we_defend
	s.note(b.attack_about if ours_attack else b.defence_about, 14)
	if b.phase > 0:
		var a: float = b.attack_odds
		var d: float = b.defence_odds
		_odds(s, "Attack on the cover", a, Sheet.BAD)
		_odds(s, "Defence on the attack", d, Sheet.INK)
	s.row("In the line", "%d (+%d) against %d (+%d)" % [b.line_a, b.reserve_a, b.line_d, b.reserve_d], Sheet.INK, 16)
	var works := "worth x%.2f" % b.works
	if b.cut > 0.005:
		works += ", shelled down %d%%" % int(round(b.cut * 100.0))
	s.row("The works", works, Sheet.INK, 16)
	var flags := PackedStringArray()
	if b.outflanked:
		flags.append("outflanked from %d sides" % b.sides)
	if b.encircled:
		flags.append("encircled, no way out")
	if not flags.is_empty():
		s.row("The defence is", ", ".join(flags), Sheet.BAD, 16)
	s.row("Lost so far", "%.1f and %.1f thousand" % [b.lost_a, b.lost_d], Sheet.SOFT, 16)


## one exchange of a battle, narrow enough for the card: a name, a measure (half full is even), and the odds
func _odds(s: Sheet, label: String, odds: float, colour: Color) -> void:
	var h := HBoxContainer.new()
	h.add_theme_constant_override("separation", 8)
	var a := RoomKit._label(label, Game.font_text, 16, Sheet.SOFT)
	a.autowrap_mode = TextServer.AUTOWRAP_OFF
	a.custom_minimum_size = Vector2(150, 0)
	var p := ProgressBar.new()
	p.max_value = 1.0
	p.value = odds / (odds + 1.0)
	p.show_percentage = false
	p.custom_minimum_size = Vector2(70, 10)
	p.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	p.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	var back := StyleBoxFlat.new()
	back.bg_color = Color(Sheet.INK, 0.1)
	back.border_color = Color(Sheet.INK, 0.45)
	back.set_border_width_all(1)
	var fill := StyleBoxFlat.new()
	fill.bg_color = Color(colour, 0.72)
	p.add_theme_stylebox_override("background", back)
	p.add_theme_stylebox_override("fill", fill)
	var b := RoomKit._label("%.1f to 1" % odds, Game.font_text, 16, colour)
	b.autowrap_mode = TextServer.AUTOWRAP_OFF
	b.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	b.custom_minimum_size = Vector2(64, 0)
	h.add_child(a)
	h.add_child(p)
	h.add_child(b)
	s.col.add_child(h)


func _show_card(p: int) -> void:
	var old := card
	var d: Dictionary = Game.sim.province(p)
	if d.is_empty():
		if old:
			old.queue_free()
		card = null
		return
	var s := Sheet.new(Color(1.0, 0.97, 0.88), Vector2(390, 0), 26)
	card = s
	s.title(d.name, 26)
	if String(d.town) != "":
		s.note(("Chief town: %s" if d.city else "Round %s") % d.town, 16)
	s.note("%s  ·  %s%s" % [d.terrain, d.status, "  ·  the capital" if d.capital else ""])
	s.rule()
	s.row("People", "%.2f million" % d.people if d.people >= 0.01 else "few")
	if d.city:
		s.row("Factories", "%d civil, %d arms" % [d.factories, d.arms_works])
	s.row("Railways", "level %d" % d.railways)
	if d.fort > 0:
		s.row("Fortress", "level %d" % d.fort)
	if d.unrest >= 20.0:
		s.row("Unrest", "%d of 100" % int(d.unrest), Color(0.6, 0.08, 0.06))
	if d.held_by != "":
		s.row("Occupied by", d.held_by, Color(0.6, 0.08, 0.06))
	for b in battles:
		if b.province == p:
			_battle_on_card(s, b)
	s.gap(6)
	s.heading(d.owner)
	if d.has("government"):
		s.row("Government", d.government)
		if d.leader != "":
			s.row("Head", d.leader)
		s.row("People", "%.1f million" % d.nation_people)
		if d.at_war_with_us:
			s.row("Relations", "at war with us", Color(0.6, 0.08, 0.06))
		elif not d.ours:
			s.row("Relations", "%+d" % int(d.relation))
	if d.ours and not choosing:                 # our own ground: what the works could put up here
		s.heading("Build here")
		for b in Game.sim.builds(p):
			if not b.can and b.level <= 0:
				continue
			var type: int = b.type
			s.act_row("%s  ·  %d" % [b.name, b.level], "%.0f" % b.cost, [s.button("Build", func(): Sheet.order(Game.sim.build(p, type)), b.can, b.why)], Sheet.INK, 15)
	if not d.ours and not choosing and String(d.get("owner_tag", "")) != "":      # a foreign government: what may be said to it
		var tag: String = d.owner_tag
		var c: Dictionary = Game.sim.country(tag)
		if c.faction != "":
			s.row("Belongs to", c.faction)
		if c.allied:
			s.row("Standing", "our ally", Sheet.GOOD)
		elif c.guarantees_us:
			s.row("Standing", "guarantees our independence", Sheet.GOOD)
		if c.wars.size() > 0:
			s.row("At war with", ", ".join(PackedStringArray(c.wars.slice(0, 3))) + ("…" if c.wars.size() > 3 else ""), Sheet.BAD, 15)
		s.row("Their forces", ("%d divisions, %d ships" % [c.divisions, int(c.navy)]) if c.known else "not known to us", Sheet.INK if c.known else Sheet.SOFT, 16)
		s.heading("To their government")
		var say := func(what: String): return func(): Sheet.order(Game.sim.diplomacy(what, tag))
		s.act_row("Influence in hand", "%d" % int(c.influence), [], Sheet.SOFT, 15)
		var first := HBoxContainer.new()
		first.add_theme_constant_override("separation", 6)
		if c.improve_due > 0.05:
			s.act_row("Goodwill on its way", "%+.1f" % c.improve_due, [], Sheet.GOOD, 15)
		first.add_child(s.button("Improve relations", say.call("improve"), not c.war and c.influence >= c.improve_cost,
				"%d influence: relations rise by 8 over two months. Each approach to the same government costs 5 more; a year without one brings it down again." % c.improve_cost))
		first.add_child(s.button("Guarantee", say.call("guarantee"), not c.war and not c.guaranteed, "25 influence: we go to war for them if they are attacked"))
		s.col.add_child(first)
		var second := HBoxContainer.new()
		second.add_theme_constant_override("separation", 6)
		second.add_child(s.button("Prepare a claim", say.call("justify"), not c.war, "40 influence: a cause for war, in time"))
		second.add_child(s.button("Declare war", say.call("war"), c.can_war, c.why_no_war if not c.can_war else "There is no taking it back"))
		s.col.add_child(second)
	s.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(s)
	if hand:
		move_child(hand, -1)
	await _settle(s, SHEET.x - 420.0, 30.0)
	if is_instance_valid(old):
		old.queue_free()


## A card comes to rest at the foot of the sheet. Wrapped lines only know their height after two layouts, and a click
## arrives before the first of them: placed any sooner, a card stood far too tall and showed nothing but blank paper.
## One longer than the sheet is printed smaller.
func _settle(s: Sheet, x: float, foot: float) -> void:
	s.modulate.a = 0.0
	s.position = Vector2(x, 0.0)
	for i in 3:
		await get_tree().process_frame
		if not is_instance_valid(s):
			return
		s.reset_size()
	var k := minf(1.0, (SHEET.y - foot - 70.0) / s.size.y)
	s.scale = Vector2(k, k)
	s.position = Vector2(x + s.size.x * (1.0 - k), SHEET.y - foot - s.size.y * k)
	s.modulate.a = 1.0
