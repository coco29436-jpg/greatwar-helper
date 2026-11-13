extends RoomKit
## The drafting office: a fixed camera on the room where the designs are kept. Every design we hold is pinned to the
## board, and the one chosen lies as a blueprint on the drawing table. What the factories build of them is decided from
## the order book on the desk (Sheet._works), not here.
## The room itself is one painting (assets/room/backdrop.jpg, made from this layout with GW_PLATE); the paper, the
## buttons and the drawings over it are live. The board turns into the catalogue of designs to open a new line, and an
## event arrives as a letter that has to be answered before time moves on.

const CARDS_MAX := 6
const CARD_W := 0.86
const CARD_H := 0.64

var sim: GreatWarSim
var selected := 0
var page := 0                   # of the board, six cards each
var catalogue := true           # the board shows the designs; it no longer shows production lines
var cat_page := 0
var confirm_close := -1         # the line whose x was clicked once
var paged_by_hand := false      # the board was turned to a leaf the chosen design is not on
var plaque_label: Label3D
var plaque_ctl: Node3D          # the arrows beside the plaque

var board: Node3D
var drawing: MeshInstance3D
var plate_title: Label3D
var date_label: Label3D
var works_label: Label3D
var money_label: Label3D
var hover_mat := {}             # Area3D -> the material to light up
var plate_mode := false         # GW_PLATE: draw only the empty room, as the layout for the painted backdrop
var live: Array[Node3D] = []    # what stays real 3D over the painting
var tilt: Node3D
var world_env: WorldEnvironment
var plaque_node: Node3D
var note_node: Node3D
var cal_node: Node3D


func _ready() -> void:
	sim = Game.sim
	plate_mode = OS.has_environment("GW_PLATE")
	_build_room()
	_split_baked()
	Game.changed.connect(func(): if active: refresh())
	refresh()


# ---- the simulation's side

func refresh() -> void:
	var n: Dictionary = sim.nation()
	date_label.text = sim.date()
	works_label.text = "%d of %d factories at work" % [n.factories_used, n.factories]
	money_label.text = "Treasury  %d m" % int(n.money)
	_clear(board)
	_clear(plaque_ctl)
	hover_mat.clear()
	var all: Array = sim.designs()
	selected = clampi(selected, 0, maxi(all.size() - 1, 0))
	cat_page = selected / CARDS_MAX if not paged_by_hand else cat_page
	_board_catalogue()
	_show_plate(all[selected] if not all.is_empty() else {})


func _board_lines(all: Array, idle: int) -> void:
	var pages := int(ceil((all.size() + 1) / float(CARDS_MAX)))     # one slot more: the card that opens a line
	page = clampi(page, 0, pages - 1)
	_plaque("PRODUCTION LINES", page, pages)
	for k in CARDS_MAX:
		var i := page * CARDS_MAX + k
		if i < all.size():
			_card(all[i], i, k)
		elif i == all.size():
			_new_card(k, idle)


func _board_catalogue() -> void:
	var list: Array = sim.designs()
	var pages := maxi(1, int(ceil(list.size() / float(CARDS_MAX))))
	cat_page = clampi(cat_page, 0, pages - 1)
	_plaque("THE DESIGNS", cat_page, pages)
	for k in CARDS_MAX:
		var i := cat_page * CARDS_MAX + k
		if i < list.size():
			_design_card(list[i], k, i)


func _plaque(title: String, at: int, pages: int) -> void:
	plaque_label.text = title if pages <= 1 else "%s  ·  %d of %d" % [title, at + 1, pages]
	plaque_label.font_size = 78 if pages <= 1 else 62
	if pages > 1:
		_button(plaque_ctl, "<", Vector3(-0.6, 0, 0.012), _turn.bind(-1), 0.12)
		_button(plaque_ctl, ">", Vector3(0.6, 0, 0.012), _turn.bind(1), 0.12)


func _turn(by: int) -> void:
	confirm_close = -1
	if catalogue:
		var pages := maxi(1, int(ceil(sim.designs().size() / float(CARDS_MAX))))
		cat_page = posmod(cat_page + by, pages)
		paged_by_hand = true
	else:
		var pages := int(ceil((sim.lines().size() + 1) / float(CARDS_MAX)))
		page = posmod(page + by, pages)
	refresh()


func _change(line: int, delta: int) -> void:
	confirm_close = -1
	if sim.change_factories(line, delta):
		refresh()


func _advance(days: int) -> void:
	confirm_close = -1
	Game.advance(days)          # stops by itself when an event arrives


func _select(line: int) -> void:
	confirm_close = -1
	selected = line
	paged_by_hand = false
	refresh()


func _open_catalogue() -> void:
	confirm_close = -1
	catalogue = true
	refresh()


func _leave_catalogue() -> void:
	catalogue = false
	refresh()


## a design from the catalogue goes on a line: an idle factory starts on it, or joins the line it already has
func _open(design: int) -> void:
	if not sim.open_line(design):
		return
	catalogue = false
	var all: Array = sim.lines()
	for i in all.size():
		if all[i].design == design:
			selected = i
			page = i / CARDS_MAX
	refresh()


## the first click asks, the second closes
func _close(line: int) -> void:
	if confirm_close != line:
		confirm_close = line
		refresh()
		return
	confirm_close = -1
	if sim.close_line(line):
		refresh()


## Escape: out of the catalogue or a question first; false when there was nothing to leave
func step_back() -> bool:
	return false


func _show_plate(line: Dictionary) -> void:
	var mat: StandardMaterial3D = drawing.material_override
	if line.is_empty():
		plate_title.text = ""
		mat.albedo_texture = null
		mat.albedo_color = Color(0.05, 0.12, 0.27)
		return
	plate_title.text = "%s\n%s" % [line.name, line.kind]
	var path := "res://assets/blueprints/%s.png" % line.chassis
	if ResourceLoader.exists(path):
		var tex: Texture2D = load(path)
		mat.albedo_texture = tex
		mat.albedo_color = Color(0.74, 0.88, 1.0)
		var w := 1.86
		var h := w * float(tex.get_height()) / tex.get_width()
		if h > 0.6:           # tall machines: as high as the sheet allows
			w *= 0.6 / h
			h = 0.6
		(drawing.mesh as PlaneMesh).size = Vector2(w, h)
	else:
		mat.albedo_texture = null
		mat.albedo_color = Color(0.05, 0.12, 0.27)


# ---- the cards on the board

func _pin_card(k: int, tint: Color) -> Array:
	var card := Node3D.new()
	card.position = Vector3(-0.92 + (k % 3) * 0.92, 0.36 - (k / 3) * 0.72, 0.015)
	card.rotation.z = deg_to_rad([-1.2, 0.8, -0.4, 1.3, 0.5, -0.9][k % 6])
	board.add_child(card)
	var paper := _paper(card, Vector2(CARD_W, CARD_H), Vector3.ZERO, tint)
	_sphere(card, 0.018, Vector3(0, CARD_H / 2 - 0.035, 0.016), _mat(Color(0.6, 0.08, 0.06), 0.3))
	return [card, paper]


func _name_size(s: String) -> int:
	return mini(74, int(1780.0 / max(s.length(), 1)))


## line i of the nation, in place k on the board
func _card(line: Dictionary, i: int, k: int) -> void:
	var chosen := i == selected
	var made := _pin_card(k, Color(1.0, 0.97, 0.86) if chosen else Color(0.8, 0.75, 0.63))
	var card: Node3D = made[0]
	var name_text: String = line.name
	_text(card, name_text, Vector3(0, 0.2, 0.004), _name_size(name_text), INK, font_title)
	_text(card, _fit(line.kind, 40), Vector3(0, 0.125, 0.004), 60, SOFT, font_text)
	_text(card, "%.2f a day    %d in stock" % [line.daily, int(line.stock)], Vector3(0, 0.05, 0.004), 70, INK, font_text)
	if confirm_close == i:
		_text(card, "Close this line?  Click × again", Vector3(0, -0.02, 0.004), 54, Color(0.6, 0.08, 0.06), font_text)
	else:
		_text(card, "efficiency %d%%" % int(round(line.efficiency * 100)), Vector3(0, -0.02, 0.004), 60, SOFT, font_text)
	_text(card, str(line.factories), Vector3(0, -0.19, 0.004), 130, INK, font_title)
	_text(card, "factories", Vector3(0, -0.272, 0.004), 50, SOFT, font_text)
	_click(card, Vector3(CARD_W, CARD_H, 0.01), Vector3(0, 0, 0.003), made[1], _select.bind(i))
	_button(card, "-", Vector3(-0.2, -0.19, 0.008), _change.bind(i, -1))
	_button(card, "+", Vector3(0.2, -0.19, 0.008), _change.bind(i, 1))
	_button(card, "×", Vector3(CARD_W / 2 - 0.065, CARD_H / 2 - 0.065, 0.008), _close.bind(i), 0.075)


## the last place on the board: opens the catalogue
func _new_card(k: int, idle: int) -> void:
	var made := _pin_card(k, Color(0.74, 0.7, 0.6))
	var card: Node3D = made[0]
	_text(card, "New Line", Vector3(0, 0.2, 0.004), 74, INK, font_title)
	if idle > 0:
		_text(card, "%d %s idle" % [idle, "factory" if idle == 1 else "factories"], Vector3(0, 0.125, 0.004), 56, SOFT, font_text)
		_text(card, "+", Vector3(0, -0.06, 0.004), 300, SOFT, font_text)
		_text(card, "choose from the catalogue", Vector3(0, -0.24, 0.004), 50, SOFT, font_text)
		_click(card, Vector3(CARD_W, CARD_H, 0.01), Vector3(0, 0, 0.003), made[1], _open_catalogue)
	else:
		_text(card, "no factory is idle", Vector3(0, 0.125, 0.004), 56, SOFT, font_text)
		_text(card, "Take one off a line with −\nto start another", Vector3(0, -0.1, 0.004), 54, SOFT, font_text)


## a design in the catalogue: pointing at it lays its drawing on the table, a click puts it on a line
func _design_card(d: Dictionary, k: int, index: int) -> void:
	var made := _pin_card(k, Color(1.0, 0.97, 0.86) if index == selected else Color(0.8, 0.75, 0.63))
	var card: Node3D = made[0]
	var name_text: String = d.name
	_text(card, name_text, Vector3(0, 0.2, 0.004), _name_size(name_text), INK, font_title)
	_text(card, _fit(d.kind, 40), Vector3(0, 0.125, 0.004), 60, SOFT, font_text)
	_text(card, "%s  ·  %d" % [d.category, d.year], Vector3(0, 0.03, 0.004), 56, SOFT, font_text)
	_text(card, "cost %.1f    %d in stock" % [d.cost, int(d.stock)], Vector3(0, -0.06, 0.004), 70, INK, font_text)
	_text(card, "the works are building it" if d.on_line else "not being built", Vector3(0, -0.2, 0.004), 58,
			Color(0.46, 0.07, 0.05) if d.on_line else SOFT, font_text)
	_click(card, Vector3(CARD_W, CARD_H, 0.01), Vector3(0, 0, 0.003), made[1], _select.bind(index))


## Over the painting only the live things are drawn; for the layout pass only the room is.
func _split_baked() -> void:
	live.append_array([drawing, plate_title, board])
	# paper on the wall faces the camera, so the fixed view's tilt does not skew it
	for n in [board, plaque_node, note_node, cal_node]:
		n.rotation.x = deg_to_rad(-9.5)
	if plate_mode:
		for n in live:
			n.visible = false
		return
	for parent in [self, tilt]:
		for c in parent.get_children():
			if (c is MeshInstance3D or c is Light3D) and not live.has(c):
				c.visible = false
	world_env.environment.background_mode = Environment.BG_CANVAS
	var layer := CanvasLayer.new()
	layer.layer = -1
	add_child(layer)
	var back := TextureRect.new()
	back.texture = load("res://assets/room/backdrop.jpg")
	back.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	back.stretch_mode = TextureRect.STRETCH_SCALE
	back.set_anchors_preset(Control.PRESET_FULL_RECT)
	layer.add_child(back)


# ---- the room

func _build_room() -> void:
	var env := WorldEnvironment.new()
	world_env = env
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color(0.02, 0.02, 0.03)
	e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	e.ambient_light_color = Color(0.78, 0.72, 0.64)
	e.ambient_light_energy = 0.62
	e.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	e.glow_enabled = true
	e.glow_intensity = 0.3
	env.environment = e
	add_child(env)

	var cam := Camera3D.new()
	cam.position = Vector3(0, 2.05, 2.7)
	cam.rotation_degrees = Vector3(-9.5, 0, 0)
	cam.fov = 50
	add_child(cam)
	cam.current = true

	var wood := _textured(Color(0.36, 0.22, 0.13), 0.55, 0.03, 0.34, 0.16, Vector3(1, 5, 1))
	var plaster := _textured(Color(0.66, 0.61, 0.5), 0.92, 0.09, 0.06, 0.03, Vector3.ONE)
	var panel := _textured(Color(0.27, 0.16, 0.09), 0.5, 0.03, 0.3, 0.14, Vector3(1, 5, 1))
	var wall_z := -0.75
	_box(self, Vector3(9, 0.1, 8), Vector3(0, -0.05, 2), wood)                 # floor
	_box(self, Vector3(9, 4.4, 0.1), Vector3(0, 2.2, wall_z - 0.05), plaster)  # back wall
	_box(self, Vector3(0.1, 4.4, 8), Vector3(-3.4, 2.2, 2), plaster)           # left wall
	_box(self, Vector3(0.1, 4.4, 8), Vector3(3.4, 2.2, 2), plaster)            # right wall
	_box(self, Vector3(9, 0.1, 8), Vector3(0, 4.0, 2), plaster)                # ceiling
	_box(self, Vector3(9, 1.0, 0.04), Vector3(0, 0.5, wall_z + 0.02), panel)   # wainscot
	_box(self, Vector3(9, 0.06, 0.07), Vector3(0, 1.02, wall_z + 0.03), wood)  # dado rail

	# the window, with daylight coming through it
	var glass := StandardMaterial3D.new()
	glass.emission_enabled = true
	glass.emission = Color(0.82, 0.88, 1.0)
	glass.emission_energy_multiplier = 2.4
	glass.albedo_color = Color.BLACK
	_box(self, Vector3(0.02, 2.1, 1.7), Vector3(-3.33, 2.3, 0.9), glass)
	for z in [0.35, 0.9, 1.45]:
		_box(self, Vector3(0.05, 2.15, 0.05), Vector3(-3.3, 2.3, z), panel)
	for y in [1.6, 2.3, 3.0]:
		_box(self, Vector3(0.05, 0.05, 1.75), Vector3(-3.3, y, 0.9), panel)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-28, -72, 0)
	sun.light_color = Color(1.0, 0.93, 0.82)
	sun.light_energy = 0.7
	sun.shadow_enabled = true
	add_child(sun)

	# the board, where the production lines are pinned up
	var cork := _textured(Color(0.56, 0.4, 0.24), 0.95, 0.35, 0.16, 0.08, Vector3.ONE)
	board = Node3D.new()
	var frame_at := Vector3(0, 2.08, wall_z + 0.04)
	_box(self, Vector3(3.0, 1.62, 0.05), frame_at, panel)
	_box(self, Vector3(2.88, 1.5, 0.05), frame_at + Vector3(0, 0, 0.012), cork)
	board.position = frame_at + Vector3(0, 0, 0.04)
	add_child(board)
	var brass := _metal(Color(0.62, 0.48, 0.22))
	var plaque := Node3D.new()
	plaque.position = frame_at + Vector3(0, 0.735, 0.06)
	add_child(plaque)
	live.append(plaque)
	plaque_node = plaque
	_paper(plaque, Vector2(0.98, 0.17), Vector3.ZERO, Color(0.92, 0.87, 0.74))
	plaque_label = _text(plaque, "PRODUCTION LINES", Vector3(0, 0, 0.018), 78, Color(0.1, 0.07, 0.03), font_title)
	plaque_ctl = Node3D.new()
	plaque.add_child(plaque_ctl)

	# the state of the works, on the left of the board
	var note := Node3D.new()
	note.position = Vector3(-2.15, 2.2, wall_z + 0.03)
	add_child(note)
	live.append(note)
	note_node = note
	_paper(note, Vector2(0.76, 0.62), Vector3.ZERO, Color(0.92, 0.87, 0.74))
	works_label = _text(note, "", Vector3(0, 0.1, 0.01), 60, Color(0.13, 0.09, 0.05), font_title)
	works_label.width = 1000
	works_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	money_label = _text(note, "", Vector3(0, -0.16, 0.01), 58, Color(0.3, 0.2, 0.1), font_text)

	# the calendar
	var cal := Node3D.new()
	cal.position = Vector3(2.15, 2.2, wall_z + 0.03)
	add_child(cal)
	live.append(cal)
	cal_node = cal
	_paper(cal, Vector2(0.76, 0.62), Vector3.ZERO, Color(0.96, 0.92, 0.82))
	date_label = _text(cal, "", Vector3(0, 0, 0.012), 84, Color(0.46, 0.07, 0.05), font_title)
	date_label.width = 1000
	date_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART

	# the drawing table, tipped toward the draughtsman
	_box(self, Vector3(3.0, 0.07, 1.3), Vector3(0, 0.95, 0.85), wood)
	for x in [-1.42, 1.42]:
		for z in [0.5, 1.6]:
			_box(self, Vector3(0.08, 0.92, 0.08), Vector3(x, 0.46, z), wood)
	tilt = Node3D.new()
	tilt.position = Vector3(0, 1.18, 0.8)
	tilt.rotation_degrees = Vector3(26, 0, 0)
	add_child(tilt)
	_box(tilt, Vector3(2.2, 0.04, 1.1), Vector3.ZERO, _textured(Color(0.74, 0.59, 0.39), 0.7, 0.03, 0.14, 0.06, Vector3(1, 3, 1)))
	_box(tilt, Vector3(2.0, 0.006, 0.96), Vector3(0, 0.022, 0.0), _mat(Color(0.05, 0.12, 0.27), 0.85))
	drawing = MeshInstance3D.new()
	var quad := PlaneMesh.new()
	quad.size = Vector2(1.86, 0.6)
	drawing.mesh = quad
	drawing.position = Vector3(0, 0.027, 0.02)
	var sheet := StandardMaterial3D.new()
	sheet.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	sheet.blend_mode = BaseMaterial3D.BLEND_MODE_ADD      # the drawings are white on black: only the lines show on the blue sheet
	drawing.material_override = sheet
	tilt.add_child(drawing)
	plate_title = Label3D.new()
	plate_title.font = font_title
	plate_title.font_size = 76
	plate_title.pixel_size = 0.00065
	plate_title.modulate = Color(0.86, 0.92, 1.0)
	plate_title.outline_size = 0
	plate_title.rotation_degrees = Vector3(-90, 0, 0)
	plate_title.position = Vector3(0, 0.03, -0.35)
	plate_title.shaded = false
	plate_title.double_sided = false
	tilt.add_child(plate_title)

	# the lamp on the table
	_cylinder(self, 0.14, 0.03, Vector3(-1.28, 1.0, 0.6), brass)
	_cylinder(self, 0.014, 0.6, Vector3(-1.28, 1.3, 0.6), brass)
	var shade := _cylinder(self, 0.17, 0.12, Vector3(-1.12, 1.6, 0.68), _mat(Color(0.07, 0.3, 0.17), 0.3))
	(shade.mesh as CylinderMesh).top_radius = 0.07
	(shade.mesh as CylinderMesh).radial_segments = 96
	shade.rotation_degrees = Vector3(0, 0, -18)
	var lamp := SpotLight3D.new()
	lamp.position = Vector3(-1.1, 1.58, 0.68)
	lamp.look_at_from_position(lamp.position, Vector3(0.1, 1.2, 1.1), Vector3.UP)
	lamp.light_color = Color(1.0, 0.82, 0.58)
	lamp.light_energy = 1.3
	lamp.spot_range = 3.5
	lamp.spot_angle = 62
	lamp.spot_attenuation = 0.8
	lamp.shadow_enabled = true
	add_child(lamp)

	# the desk bell: one week on
	var bell := _sphere(self, 0.085, Vector3(1.34, 1.08, 0.62), _metal(Color(0.78, 0.62, 0.3)))
	_cylinder(self, 0.11, 0.03, Vector3(1.34, 1.0, 0.62), _mat(Color(0.12, 0.08, 0.05), 0.5))
	var hint := _text(self, "The clock", Vector3(1.34, 1.24, 0.62), 56, Color(0.95, 0.88, 0.7), font_text)
	hint.visible = false
	hint.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	_click(self, Vector3(0.28, 0.28, 0.28), Vector3(1.34, 1.08, 0.62), null, func(): Game.paused = not Game.paused, hint)


# ---- small things

# ---- for testing without a person: the headquarters passes on the steps of GW_ACT it does not know itself

func act(p: PackedStringArray) -> void:
	match p[0]:
		"change": _change(int(p[1]), int(p[2]))
		"select": _select(int(p[1]))
		"turn": _turn(int(p[1]))
		"catalogue": _open_catalogue()
		"open": _open(int(p[1]))
		"close": _close(int(p[1]))
