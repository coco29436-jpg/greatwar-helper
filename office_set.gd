class_name OfficeSet
extends RoomKit
## The office as a real room: walls, windows, door, bookcases and furniture, built from rounded and turned shapes and
## lit by the room's own lights. The chair is behind the desk and the desk faces the room: the map hangs on the far
## wall, the windows are on the left, the bookcases on the right. Nothing here can be clicked; office.gd puts the
## things of government on the desk and the walls.
##
## x runs left to right, y up, and the room runs away from the chair toward -z.

const DESK_Y := 0.78
const DESK_AT := Vector3(0, 0, 1.105)
const DESK_SIZE := Vector2(2.9, 1.5)
const HALF_W := 3.6
const FAR_Z := -3.4
const BACK_Z := 3.7
const HEIGHT := 3.7
const DADO := 0.95
const WINDOWS: Array[float] = [-2.33, 1.2]      # where along the left wall (z): far enough apart for the commode between their curtains
const WIN_W := 1.3
const WIN_TOP := 3.05
const DOOR_X := 2.7
const DRAPE_TIE := 1.2                         # how high the curtains are tied back
const SUN := Vector3(1.0, -0.6, 0.14)          # the way the morning light travels

var room: Node3D
var m_wood: StandardMaterial3D
var m_wood_up: StandardMaterial3D
var m_dark: StandardMaterial3D
var m_dark_up: StandardMaterial3D
var m_panel: StandardMaterial3D
var m_floor: StandardMaterial3D
var m_plaster: StandardMaterial3D
var m_paper: StandardMaterial3D
var m_brass: StandardMaterial3D
var m_leather: StandardMaterial3D
var m_hide: StandardMaterial3D
var m_cloth: StandardMaterial3D
var globe_mat: StandardMaterial3D               # office.gd draws the world on it
var clock_at := Vector3.ZERO                    # the middle of the long-case clock, for the hand
var clock_size := Vector2.ONE
var curtains: Array = []                        # one for each window: {wall, x, drapes, ties, open}
var lamp_node: Node3D                           # the desk lamp, its bulb and its shade, for the switch
var lamp_bulb: StandardMaterial3D
var lamp_glass: StandardMaterial3D
const DRAPE_W := 0.5                            # a curtain tied back, across
const DRAPE_OUT := 0.12                         # how far beyond the window it hangs


func _build_set() -> void:
	room = Node3D.new()
	add_child(room)
	m_wood = _pbr("wood_table_001", 1.3, Color(1.0, 0.92, 0.88), 0.75)
	m_wood_up = m_wood
	m_dark = _pbr("lacquered_cherry_wood", 1.1, Color(0.95, 0.8, 0.75), 0.8)
	m_dark_up = m_dark
	m_panel = _pbr("wooden_panels", 0.4, Color(0.62, 0.52, 0.5), 0.85)
	m_floor = _pbr("herringbone_parquet", 0.55, Color(0.62, 0.5, 0.44), 0.7)
	m_plaster = _pbr("painted_plaster_wall", 1.0, Color(1.0, 0.96, 0.88))
	m_paper = _pbr("floral_jacquard", 0.7, Color(0.74, 0.8, 0.7))         # a muted grey-green: the bright green read Victorian
	m_brass = _polished(Color(0.5, 0.4, 0.25), 0.5)          # tarnished: polished gold read gilt
	m_leather = _pbr("leather_white", 3.0, Color(0.2, 0.2, 0.14))        # dark olive-brown: the bright green pad read Victorian
	m_hide = _pbr("leather_red_03", 3.0, Color(0.3, 0.06, 0.05), 0.8)
	m_cloth = _pbr("velour_velvet", 3.0, Color(0.17, 0.13, 0.1), 1.0)      # faded brown: crimson read belle époque
	m_cloth.albedo_texture = null           # the scan's colour is magenta-red; its pile and sheen stay in the normal and roughness maps
	m_cloth.rim_enabled = true              # the nap of velvet catches the light at its edges
	m_cloth.rim = 0.5
	m_cloth.rim_tint = 0.7
	m_cloth.cull_mode = BaseMaterial3D.CULL_DISABLED
	_shell()
	_furniture()


static func _h(i: int, k: int) -> float:
	return fposmod(sin(i * 127.1 + k * 311.7) * 43758.5453, 1.0)


func _slab(parent: Node3D, size: Vector3, at: Vector3, m: Material) -> MeshInstance3D:
	var b := BoxMesh.new()
	b.size = size
	return _put(parent, b, at, m)


func _rod(parent: Node3D, r: float, length: float, at: Vector3, m: Material) -> MeshInstance3D:
	var c := CylinderMesh.new()
	c.top_radius = r
	c.bottom_radius = r
	c.height = length
	c.radial_segments = 24
	return _put(parent, c, at, m)


# ---- walls. Each is built on a node whose x runs along the wall and whose z points into the room.

func _shell() -> void:
	var depth := BACK_Z - FAR_Z
	var mid := (BACK_Z + FAR_Z) / 2.0
	_slab(room, Vector3(HALF_W * 2 + 0.4, 0.2, depth + 0.4), Vector3(0, -0.1, mid), m_floor)
	_slab(room, Vector3(HALF_W * 2 + 0.4, 0.2, depth + 0.4), Vector3(0, HEIGHT + 0.1, mid), m_plaster)

	var far := _wall(Vector3(0, 0, FAR_Z), 0.0, HALF_W * 2, [[DOOR_X, 1.0, 0.0, 2.2]])
	_door(far, DOOR_X)
	_wall(Vector3(HALF_W, 0, mid), -90.0, depth, [])
	_wall(Vector3(0, 0, BACK_Z), 180.0, HALF_W * 2, [])
	var holes := []
	for z in WINDOWS:
		holes.append([mid - z, WIN_W, DADO, WIN_TOP])
	var left := _wall(Vector3(-HALF_W, 0, mid), 90.0, depth, holes)
	for z in WINDOWS:
		_window(left, mid - z)

	for z: float in [-2.3, -0.5, 1.3]:              # beams across the ceiling
		_rbox(room, Vector3(HALF_W * 2, 0.2, 0.24), 0.03, Vector3(0, HEIGHT - 0.1, z), m_dark, true)

	# the carpet: a border and a field
	_slab(room, Vector3(4.6, 0.012, 3.5), Vector3(0, 0.006, -0.85), _pbr("dirty_carpet", 1.5, Color(1.0, 0.25, 0.2)))
	_slab(room, Vector3(4.0, 0.014, 2.9), Vector3(0, 0.008, -0.85), _pbr("dirty_carpet", 1.5, Color(0.46, 0.4, 0.33)))      # worn grey-brown, not blue
	_slab(room, Vector3(3.4, 0.016, 2.3), Vector3(0, 0.009, -0.85), _pbr("dirty_carpet", 1.5, Color(1.1, 0.3, 0.22)))


## holes: [x along the wall, width, bottom, top]. A hole that reaches the floor is a door.
func _wall(at: Vector3, turn: float, length: float, holes: Array) -> Node3D:
	var w := Node3D.new()
	w.position = at
	w.rotation_degrees.y = turn
	room.add_child(w)
	holes.sort_custom(func(a, b): return a[0] < b[0])
	var x := -length / 2.0
	var runs: Array[Vector2] = []               # where the panelling goes: everywhere but across a door
	var run_from := x
	for h: Array in holes:
		var a: float = h[0] - h[1] / 2.0
		var b: float = h[0] + h[1] / 2.0
		_wall_piece(w, x, a, 0.0, HEIGHT)
		_wall_piece(w, a, b, 0.0, h[2])
		_wall_piece(w, a, b, h[3], HEIGHT)
		if h[2] <= 0.0:
			runs.append(Vector2(run_from, a))
			run_from = b
		x = b
	_wall_piece(w, x, length / 2.0, 0.0, HEIGHT)
	runs.append(Vector2(run_from, length / 2.0))
	for r in runs:
		var n := r.y - r.x
		if n < 0.05:
			continue
		var c := (r.x + r.y) / 2.0
		_slab(w, Vector3(n, DADO, 0.04), Vector3(c, DADO / 2.0, 0.02), m_panel)
		_rbox(w, Vector3(n, 0.07, 0.07), 0.02, Vector3(c, DADO, 0.035), m_wood, true)          # dado rail
		_rbox(w, Vector3(n, 0.16, 0.06), 0.015, Vector3(c, 0.08, 0.035), m_dark, true)         # skirting
	_rbox(w, Vector3(length, 0.2, 0.14), 0.05, Vector3(0, HEIGHT - 0.1, 0.07), m_plaster, true)     # cornice
	_rbox(w, Vector3(length, 0.035, 0.035), 0.012, Vector3(0, HEIGHT - 0.5, 0.017), m_wood, true)   # picture rail
	return w


func _wall_piece(w: Node3D, x0: float, x1: float, y0: float, y1: float) -> void:
	if x1 - x0 < 0.001 or y1 - y0 < 0.001:
		return
	_slab(w, Vector3(x1 - x0, y1 - y0, 0.24), Vector3((x0 + x1) / 2.0, (y0 + y1) / 2.0, -0.12), m_paper)


func _window(w: Node3D, x: float) -> void:
	var h := WIN_TOP - DADO
	var yc := (WIN_TOP + DADO) / 2.0
	# the capital outside, in the haze (it was a bare sky gradient that burnt out white)
	var glass := ShaderMaterial.new()
	glass.shader = preload("res://assets/shaders/window_view.gdshader")
	glass.set_shader_parameter("seed", x * 0.37)
	var q := QuadMesh.new()
	q.size = Vector2(WIN_W, h)
	_put(w, q, Vector3(x, yc, -0.2), glass).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	# the reveal, the sash bars and the sill
	for side: float in [-1.0, 1.0]:
		_rbox(w, Vector3(0.1, h + 0.16, 0.3), 0.02, Vector3(x + side * (WIN_W / 2.0 + 0.03), yc, -0.1), m_dark_up, true)
	_rbox(w, Vector3(WIN_W + 0.26, 0.1, 0.3), 0.02, Vector3(x, WIN_TOP + 0.04, -0.1), m_dark, true)
	_rbox(w, Vector3(WIN_W + 0.3, 0.05, 0.34), 0.02, Vector3(x, DADO + 0.01, -0.1), m_dark, true)     # short of the curtains, so they can be drawn
	_slab(w, Vector3(0.04, h, 0.04), Vector3(x, yc, -0.16), m_dark_up)
	for k in [1, 2, 3]:
		_slab(w, Vector3(WIN_W, 0.035, 0.04), Vector3(x, DADO + h * k / 4.0, -0.16), m_dark)
	# velvet curtains, drawn back into brass ties under a wooden pelmet
	_rbox(w, Vector3(WIN_W + 0.8, 0.26, 0.2), 0.03, Vector3(x, WIN_TOP + 0.34, 0.11), m_dark, true)
	_rbox(w, Vector3(WIN_W + 0.84, 0.05, 0.23), 0.02, Vector3(x, WIN_TOP + 0.48, 0.115), m_dark, true)
	_slab(w, Vector3(WIN_W + 0.72, 0.012, 0.004), Vector3(x, WIN_TOP + 0.25, 0.212), _polished(Color(0.5, 0.4, 0.25), 0.5))
	var pair := {"wall": w, "x": x, "drapes": [], "ties": [], "open": 1.0}
	for side: float in [-1.0, 1.0]:
		var hang := x + side * (WIN_W / 2.0 + DRAPE_OUT)
		pair.drapes.append(_put(w, _drape(DRAPE_W, WIN_TOP + 0.3, side), Vector3(hang, 0.02, 0.12), m_cloth))
		var tie := TorusMesh.new()
		tie.inner_radius = 0.085
		tie.outer_radius = 0.105
		tie.rings = 32
		var ring := _put(w, tie, Vector3(hang + side * 0.14, DRAPE_TIE, 0.12), m_brass)
		ring.scale = Vector3(1.0, 0.8, 0.5)
		pair.ties.append(ring)
		pair.ties.append(_rbox(w, Vector3(0.03, 0.03, 0.12), 0.01, Vector3(hang + side * 0.24, DRAPE_TIE, 0.06), m_brass, true))
	curtains.append(pair)


## a window's curtains at `open`: 1 tied back, 0 drawn across the glass
func _set_curtains(index: int, open: float) -> void:
	var pair: Dictionary = curtains[index]
	pair.open = open
	for k in 2:
		(pair.drapes[k] as MeshInstance3D).mesh = _drape(DRAPE_W, WIN_TOP + 0.3, -1.0 if k == 0 else 1.0, open)
	for tie: MeshInstance3D in pair.ties:
		tie.visible = open > 0.6


## A curtain of heavy cloth, `width` across where it hangs from the rod and `height` up from the floor, drawn aside
## (`pull` -1 or 1) into a tie at DRAPE_TIE: gathered there, full above, flaring again toward the floor. As `open`
## falls to 0 it is let out of the tie and drawn across to the middle of the window.
func _drape(width: float, height: float, pull: float, open := 1.0) -> ArrayMesh:
	const COLS := 56
	const ROWS := 18
	const FOLDS := 6.0
	var at: Array[Vector3] = []
	var drawn := WIN_W / 2.0 + DRAPE_OUT + width / 2.0 + 0.03      # from its outer edge to just past the middle of the window
	var wide := lerpf(drawn, width, open)
	var shift := -pull * (wide - width) * 0.5
	for r in ROWS + 1:
		var y := height * r / ROWS
		var g := lerpf(0.42, 1.0, pow(smoothstep(DRAPE_TIE, height, y), 0.8)) if y >= DRAPE_TIE else lerpf(0.42, 0.78, smoothstep(DRAPE_TIE, 0.0, y))
		g = lerpf(1.0, g, open)
		for i in COLS + 1:
			var t := float(i) / COLS
			var depth := (0.026 + 0.05 * (1.0 - g)) * (1.0 + 0.3 * sin(i * 1.7))
			var a := t * FOLDS * TAU + 0.5 * sin(y * 2.1 + i * 0.31)
			at.append(Vector3((t - 0.5) * wide * g + pull * (1.0 - g) * wide * 0.5 + shift, y, sin(a) * depth + 0.018 * sin(t * 5.0 + y * 3.0)))
	var verts := PackedVector3Array()
	var normals := PackedVector3Array()
	var idx := PackedInt32Array()
	for r in ROWS + 1:
		for i in COLS + 1:
			var here := r * (COLS + 1) + i
			var across := at[r * (COLS + 1) + mini(i + 1, COLS)] - at[r * (COLS + 1) + maxi(i - 1, 0)]
			var up := at[mini(r + 1, ROWS) * (COLS + 1) + i] - at[maxi(r - 1, 0) * (COLS + 1) + i]
			verts.append(at[here])
			normals.append(across.cross(up).normalized())
	for r in ROWS:
		for i in COLS:
			var a := r * (COLS + 1) + i
			var b := a + COLS + 1
			idx.append_array([a, b, a + 1, a + 1, b, b + 1])
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = normals
	arrays[Mesh.ARRAY_INDEX] = idx
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return mesh


func _door(w: Node3D, x: float) -> void:
	# The leaf fills its opening square to the edge, with a board behind it, and the casing laps over it all round:
	# with rounded edges and a head set above the opening, slits of wall and of the dark behind showed round the door.
	_slab(w, Vector3(1.0, 2.2, 0.04), Vector3(x, 1.1, -0.06), m_dark)
	_slab(w, Vector3(1.0, 2.2, 0.05), Vector3(x, 1.1, -0.005), m_wood_up)
	_rbox(w, Vector3(1.0, 0.025, 0.12), 0.008, Vector3(x, 0.0125, 0.02), m_dark, true)             # the threshold
	for y: float in [0.32, 1.1, 1.88]:                                                             # hinges
		_rod(w, 0.011, 0.11, Vector3(x + 0.488, y, 0.03), m_brass)
	for y: float in [0.64, 1.62]:
		for side: float in [-1.0, 1.0]:
			_rbox(w, Vector3(0.34, 0.8 if y > 1.0 else 0.72, 0.025), 0.012, Vector3(x + side * 0.22, y, 0.022), m_dark, true)
			_rbox(w, Vector3(0.26, 0.72 if y > 1.0 else 0.64, 0.02), 0.01, Vector3(x + side * 0.22, y, 0.032), m_wood_up, true)
	for side: float in [-1.0, 1.0]:
		_rbox(w, Vector3(0.16, 2.2, 0.08), 0.02, Vector3(x + side * 0.565, 1.1, 0.035), m_dark_up, true)
	_rbox(w, Vector3(1.4, 0.18, 0.1), 0.03, Vector3(x, 2.27, 0.04), m_dark, true)
	_rbox(w, Vector3(1.5, 0.05, 0.13), 0.02, Vector3(x, 2.38, 0.05), m_dark, true)
	var knob := SphereMesh.new()
	knob.radius = 0.035
	knob.height = 0.07
	_put(w, knob, Vector3(x - 0.39, 1.05, 0.085), m_brass)
	_rod(w, 0.012, 0.05, Vector3(x - 0.39, 1.05, 0.05), m_brass).rotation_degrees.x = 90
	_rbox(w, Vector3(0.055, 0.22, 0.008), 0.004, Vector3(x - 0.39, 1.02, 0.03), m_brass, true)


# ---- furniture

func _furniture() -> void:
	_writing_desk()
	_bookcase(Vector3(HALF_W, 0, -1.55), 1.9, 0)
	_bookcase(Vector3(HALF_W, 0, 0.45), 1.9, 1)
	for side: float in [-1.0, 1.0]:             # the visitors' chairs, in oxblood, turned a little toward each other
		_tint(_model(room, "ArmChair_01", Vector3(side * 0.88, 0, -0.95), -22.0 * side), Color(0.55, 0.42, 0.33))
	_globe(Vector3(-2.3, 0, 1.25))
	_banker_lamp(Vector3(-1.27, DESK_Y, 0.52))

	# the far wall: the clock left of the map, a console under it, a landscape over the door
	var clock := _model(room, "vintage_grandfather_clock_01", Vector3(-2.42, 0, FAR_Z + 0.27))
	var cs: Vector3 = clock.get_meta("size")
	clock_at = clock.position + Vector3(0, cs.y / 2.0, cs.z / 2.0)
	clock_size = Vector2(cs.x, cs.y)
	var console := _model(room, "ClassicConsole_01", Vector3(0, 0, FAR_Z + 0.4), 0.0, 0.88)
	var shelf: float = (console.get_meta("size") as Vector3).y
	for side: float in [-1.0, 1.0]:
		_model(room, "brass_candleholders", Vector3(side * 0.48, shelf, FAR_Z + 0.36), 20.0 * side, 0.0, "floor", "brass_candleholder_02")
	_model(room, "fancy_picture_frame_01", Vector3(DOOR_X, 2.8, FAR_Z + 0.02), 0.0, 0.6, "centre")
	_model(room, "potted_plant_02", Vector3(-3.1, 0, -2.25), 0.0, 0.72)        # under the far window: the corner beside the clock is too narrow for it

	# the pier between the windows: a commode with a bust on it, and a portrait above
	var pier := (WINDOWS[0] + WINDOWS[1]) / 2.0
	var commode := _model(room, "GothicCommode_01", Vector3(-HALF_W + 0.36, 0, pier), 90.0)
	var top: float = (commode.get_meta("size") as Vector3).y
	_model(room, "marble_bust_01", Vector3(-HALF_W + 0.38, top, pier), 78.0)
	_model(room, "fancy_picture_frame_02", Vector3(-HALF_W + 0.065, 2.45, pier), 90.0, 1.0, "centre")

	_model(room, "Chandelier_01", Vector3(0, HEIGHT - 0.2, -1.2), 0.0, 1.05, "top")
	_rod(room, 0.012, 0.2, Vector3(0, HEIGHT - 0.1, -1.2), m_brass)

	# small things left lying on the desk
	var lens := _model(room, "magnifying_glass_01", Vector3(-0.38, DESK_Y + 0.02, 1.6), 0.0, 0.0, "centre")
	lens.rotation_degrees = Vector3(-90, 55, 0)


func _writing_desk() -> void:
	var d := Node3D.new()
	d.position = DESK_AT
	room.add_child(d)
	var turned := Node3D.new()                  # so the grain of the top runs along the desk
	turned.rotation_degrees.y = 90
	d.add_child(turned)
	_rbox(turned, Vector3(DESK_SIZE.y, 0.06, DESK_SIZE.x), 0.022, Vector3(0, DESK_Y - 0.03, 0), m_wood, true)
	_rbox(d, Vector3(DESK_SIZE.x - 0.1, 0.03, DESK_SIZE.y - 0.1), 0.01, Vector3(0, DESK_Y - 0.07, 0), m_dark, true)
	# the leather, with a tooled gold line round it
	_slab(d, Vector3(DESK_SIZE.x - 0.5, 0.004, DESK_SIZE.y - 0.4), Vector3(0, DESK_Y + 0.001, 0), m_leather)
	var gold := _polished(Color(0.5, 0.4, 0.25), 0.55)
	var ped := DESK_SIZE.x / 2.0 - 0.42
	for side: float in [-1.0, 1.0]:
		_slab(d, Vector3(DESK_SIZE.x - 0.6, 0.0045, 0.006), Vector3(0, DESK_Y + 0.001, side * (DESK_SIZE.y / 2.0 - 0.25)), gold)
		_slab(d, Vector3(0.006, 0.0045, DESK_SIZE.y - 0.5), Vector3(side * (DESK_SIZE.x / 2.0 - 0.3), DESK_Y + 0.001, 0), gold)
		_rbox(d, Vector3(0.74, 0.72, DESK_SIZE.y - 0.16), 0.015, Vector3(side * ped, 0.36, 0), m_dark, true)
		for k in 3:                             # the fronts toward the room
			_rbox(d, Vector3(0.6, 0.17, 0.03), 0.01, Vector3(side * ped, 0.15 + k * 0.21, -DESK_SIZE.y / 2.0 + 0.075), m_wood, true)
	_slab(d, Vector3(DESK_SIZE.x - 1.5, 0.5, 0.03), Vector3(0, 0.45, -DESK_SIZE.y / 2.0 + 0.2), m_dark)


## the back of the case is against the right-hand wall. The shelves hold bound sets, each shelf in its own binding
func _bookcase(at: Vector3, width: float, which: int) -> void:
	var b := Node3D.new()
	b.position = at
	b.rotation_degrees.y = -90
	room.add_child(b)
	const DEPTH := 0.36
	const TOP := 2.75
	const SHELVES := 6
	_slab(b, Vector3(width, TOP, 0.03), Vector3(0, TOP / 2.0, 0.015), m_dark)
	for side: float in [-1.0, 1.0]:
		_rbox(b, Vector3(0.06, TOP, DEPTH), 0.012, Vector3(side * (width / 2.0 - 0.03), TOP / 2.0, DEPTH / 2.0), m_dark, true)
	_rbox(b, Vector3(width + 0.1, 0.12, DEPTH + 0.07), 0.035, Vector3(0, TOP + 0.03, DEPTH / 2.0), m_dark, true)
	_rbox(b, Vector3(width + 0.04, 0.3, DEPTH + 0.03), 0.012, Vector3(0, 0.15, DEPTH / 2.0), m_dark, true)
	var bindings: Array[Color] = [Color(0.9, 0.85, 0.8), Color(0.5, 0.58, 0.48), Color(0.75, 0.42, 0.36), Color(0.48, 0.52, 0.68), Color(0.55, 0.5, 0.45),
			Color(0.8, 0.66, 0.5)]
	var step := (TOP - 0.3) / SHELVES
	var inner := width - 0.14
	for s in SHELVES:
		var y := 0.3 + s * step
		_rbox(b, Vector3(inner + 0.02, 0.03, DEPTH - 0.03), 0.008, Vector3(0, y - 0.015, DEPTH / 2.0), m_dark, true)
		var k := which * 7 + s
		var tall := (step - 0.07) * (0.86 + 0.14 * _h(k, 1))
		var sets := 3
		var room_for := inner - (0.16 if _h(k, 2) < 0.4 else 0.0)       # some shelves are not quite full
		for i in sets:
			var set_of := _model(b, "book_encyclopedia_set_01", Vector3.ZERO, 0.0, tall)
			var size: Vector3 = set_of.get_meta("size")
			var stretch := room_for / sets / size.x
			set_of.scale = Vector3(stretch, 1, 1)
			set_of.position = Vector3(-inner / 2.0 + (i + 0.5) * room_for / sets, y, DEPTH - 0.06 - size.z / 2.0)
			_tint(set_of, bindings[(k * 3 + (i if _h(k, 3) < 0.35 else 0)) % bindings.size()])








func _globe(at: Vector3) -> void:
	var g := Node3D.new()
	g.position = at
	room.add_child(g)
	_lathe(g, PackedVector2Array([Vector2(0, 0), Vector2(0.24, 0), Vector2(0.25, 0.03), Vector2(0.1, 0.07), Vector2(0.035, 0.14),
			Vector2(0.03, 0.42), Vector2(0.055, 0.5), Vector2(0.03, 0.58), Vector2(0.028, 0.74), Vector2(0, 0.75)]), Vector3.ZERO, m_dark_up)
	var ball := SphereMesh.new()
	ball.radius = 0.27
	ball.height = 0.54
	globe_mat = _mat(Color(0.8, 0.74, 0.6), 0.5)
	var tilt := Node3D.new()
	tilt.position = Vector3(0, 1.06, 0)
	tilt.rotation_degrees = Vector3(0, 200, 23.4)
	g.add_child(tilt)
	_put(tilt, ball, Vector3.ZERO, globe_mat)
	var ring := TorusMesh.new()
	ring.inner_radius = 0.285
	ring.outer_radius = 0.3
	ring.rings = 64
	_put(tilt, ring, Vector3.ZERO, m_brass).rotation_degrees.x = 90
	_rod(tilt, 0.008, 0.64, Vector3.ZERO, m_brass)





## a brass lamp with an olive glass shade
func _banker_lamp(at: Vector3) -> void:
	var l := Node3D.new()
	l.position = at
	l.rotation_degrees.y = 25
	room.add_child(l)
	lamp_node = l
	var brass := _polished(Color(0.34, 0.26, 0.14), 0.5)          # aged: it stands in its own light
	_lathe(l, PackedVector2Array([Vector2(0, 0), Vector2(0.1, 0), Vector2(0.105, 0.012), Vector2(0.075, 0.028), Vector2(0.024, 0.042),
			Vector2(0.013, 0.07), Vector2(0.012, 0.36), Vector2(0.02, 0.375), Vector2(0.012, 0.39), Vector2(0, 0.395)]), Vector3.ZERO, brass)
	var arm := _rod(l, 0.008, 0.2, Vector3(0, 0.375, 0.09), brass)
	arm.rotation_degrees.x = 90
	var glass := _mat(Color(0.06, 0.08, 0.05), 0.05)           # a dull olive: the emerald shade was the brightest thing in the room
	glass.clearcoat_enabled = true
	glass.emission_enabled = true
	glass.emission = Color(0.13, 0.14, 0.07)
	glass.emission_energy_multiplier = 0.18
	var shade := _lathe(l, PackedVector2Array([Vector2(0.125, 0), Vector2(0.12, 0.03), Vector2(0.1, 0.065), Vector2(0.06, 0.09), Vector2(0, 0.1)]),
			Vector3(0, 0.37, 0.13), glass)
	shade.scale = Vector3(1.4, 1.25, 0.85)
	lamp_glass = shade.material_override
	var bulb := SphereMesh.new()
	bulb.radius = 0.03
	bulb.height = 0.06
	var lit := _mat(Color.BLACK, 1.0)
	lit.emission_enabled = true
	lit.emission = Color(1.0, 0.8, 0.5)
	lit.emission_energy_multiplier = 4.0
	lamp_bulb = lit
	_put(l, bulb, Vector3(0, 0.41, 0.13), lit).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_rod(l, 0.002, 0.14, Vector3(0.1, 0.3, 0.16), brass)
	var dark := PlaneMesh.new()                 # where the foot meets the desk
	dark.size = Vector2(0.4, 0.4)
	var dm := _flat(load("res://assets/room/shadow.png"), Color(1, 1, 1, 0.9))
	_add(l, dark, Vector3(0, 0.0045, 0), dm).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	var knob := SphereMesh.new()
	knob.radius = 0.008
	knob.height = 0.016
	_put(l, knob, Vector3(0.1, 0.225, 0.16), brass)
