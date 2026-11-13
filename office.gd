extends "res://scripts/office_set.gd"
## The office: the desk every other screen is reached from. You sit behind the desk and the desk faces the room
## (office_set.gd builds it). The head follows the pointer: low on the screen you look down at the papers, high up you
## look across the room at the map, and toward the sides you turn to the windows and the bookcases. Each thing on the
## desk stands for one part of the government (things, below): the headquarters (hq.gd) opens it when it is clicked.

signal picked(key: String)
signal pointed(caption: String)   # what the hand is on, for the headquarters to name at the foot of the screen

const MAP_SIZE := Vector2(3.56, 2.0)
const EYE := Vector3(0, 1.78, 2.6)
const LOOK_SIDE := 42.0         # degrees the head turns to either side
const LOOK_UP := -1.0           # looking across the room
const LOOK_DOWN := -24.0        # looking at the desk
const LOOK_SPEED := 4.0

var things := {}                # key -> {node, title, hint, size, height, flies, home, mat, wall}
var cam: Camera3D
var desk: Node3D                # the things on the desk
var pointer := Vector2(0.5, 0.5)    # where the hand is on the screen, 0 to 1: the headquarters sets it
var gaze := Vector2.ZERO        # the head: degrees to the left, degrees up
var look_frozen := false        # the head keeps still: a sheet is in the hand, or the picture is closing in on something
var wall_view: SubViewport      # the map on the wall is the real map, drawn small
var wall_map: PaperMap
var prints := {}                # key -> Print: the pages that follow the simulation
var last_news := ""
var tray_pages: Array = []      # the reports lying in the tray: {node, print}, the newest on top
var lamp_on := true
var window_light: Array[float] = [1.0, 1.0]    # how much of the sky each window lets in: its curtains (ambience.gd reads it)
var env_res: Environment
var lamp: SpotLight3D
var sun: DirectionalLight3D
var sky_lights: Array[OmniLight3D] = []
var chandelier: OmniLight3D
var ambience: Node               # what comes in from the capital outside (ambience.gd)
var ember: StandardMaterial3D
var ember_light: OmniLight3D
var flying: Node3D              # the sheet in the hand
var fly_from: Transform3D
var fly_loose := false
var time := 0.0


func _ready() -> void:
	_build_set()
	_build_view()
	_build_walls()
	_build_desk()
	_build_cigar(Vector3(0.56, DESK_Y, 1.66))
	_build_life()
	ambience = load("res://scripts/ambience.gd").new(self)
	add_child(ambience)
	gaze = _look_target()
	_aim(0.0)
	# the lamp and the curtains are worked by hand
	_register("lamp", "The Lamp", "Put it out, or light it", lamp_node, Vector2(0.36, 0.42), 0.48, false, null, false)
	things["lamp"]["fixed"] = true
	for i in curtains.size():
		var hold := Node3D.new()
		hold.position = Vector3(curtains[i].x, (WIN_TOP + 0.3) / 2.0, 0.14)
		(curtains[i].wall as Node3D).add_child(hold)
		_register("curtain%d" % i, "The Curtains", "Draw them, or tie them back", hold, Vector2(WIN_W + 2.0 * (DRAPE_OUT + DRAPE_W / 2.0), WIN_TOP + 0.3), 0.0, false, null, true)
	Game.changed.connect(refresh)
	Game.changed.connect(apply_quality)
	apply_quality()
	refresh()


func refresh() -> void:
	var news: Array = Game.sim.news(1)
	var top: String = news[0].text if not news.is_empty() else "No news of note"
	if top != last_news and last_news != "" and active:
		_deliver_paper()
	last_news = top
	var date := Game.sim.date()
	var focus: Dictionary = Game.sim.focus()
	(prints["news"] as Print).press(top + date)
	(prints["calendar"] as Print).press(date)
	(prints["focus"] as Print).press("%s %d %d" % [focus.get("name", ""), int(focus.get("done", 0)) / 5, (focus.get("open", []) as Array).size()])
	(prints["society"] as Print).press(date.get_slice(" ", 1))
	var n := Game.unread.size()
	for k in tray_pages.size():             # a page for each report waiting, five at most
		var page: Dictionary = tray_pages[k]
		(page.node as Node3D).visible = k < n
		if k < n:
			(page.print as Print).press(String(Game.unread[n - 1 - k].name))
	if wall_view:
		wall_view.render_target_update_mode = SubViewport.UPDATE_ONCE


func _process(delta: float) -> void:
	if not active:
		return
	time += delta
	_aim(delta)
	lamp.light_energy = 3.2 * (1.0 + 0.012 * sin(time * 9.0) + 0.008 * sin(time * 23.0)) if lamp_on else 0.0
	var glow := 0.75 + 0.2 * sin(time * 1.3) + 0.08 * sin(time * 7.1)
	ember.emission_energy_multiplier = 3.5 * glow
	ember_light.light_energy = 0.02 * glow


# ---- the head

func _look_target() -> Vector2:
	var u := clampf(pointer.x * 2.0 - 1.0, -1.0, 1.0)
	return Vector2(-signf(u) * smoothstep(0.3, 1.0, absf(u)) * LOOK_SIDE, lerpf(LOOK_UP, LOOK_DOWN, smoothstep(0.12, 0.6, pointer.y)))


## the head turns, and leans a little with it: over the desk when looking down, to the side when looking aside
func _aim(delta: float) -> void:
	if not look_frozen:
		gaze = gaze.lerp(_look_target(), 1.0 - exp(-delta * LOOK_SPEED))
	cam.rotation_degrees = Vector3(gaze.y, gaze.x, 0)
	var down := clampf(inverse_lerp(LOOK_UP, LOOK_DOWN, gaze.y), 0.0, 1.0)
	cam.position = EYE + Vector3(-gaze.x / LOOK_SIDE * 0.16, -0.03 * down, -0.06 * down)


## turn to a thing and hold still: the way into the map or the drafting office starts from there
func face_thing(key: String, seconds := 0.3) -> Tween:
	look_frozen = true
	var t: Dictionary = things[key]
	var n: Node3D = t.node
	var to := n.global_position - EYE
	var want := Vector2(clampf(rad_to_deg(atan2(-to.x, -to.z)), -LOOK_SIDE, LOOK_SIDE),
			clampf(rad_to_deg(atan2(to.y, Vector2(to.x, to.z).length())), LOOK_DOWN, LOOK_UP + 6.0))
	var tw := create_tween()
	tw.tween_property(self, "gaze", want, seconds).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	return tw


# ---- the eye and the light

func _build_view() -> void:
	var env := WorldEnvironment.new()
	var e := Environment.new()
	env_res = e
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color(0.02, 0.02, 0.03)
	e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	e.ambient_light_color = Color(0.72, 0.68, 0.62)
	e.ambient_light_energy = 0.3
	e.reflected_light_source = Environment.REFLECTION_SOURCE_DISABLED
	e.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	e.tonemap_white = 4.0
	e.tonemap_exposure = 1.4
	e.ssao_enabled = true
	e.ssao_radius = 0.35
	e.ssao_intensity = 3.0
	e.ssao_power = 1.8
	e.glow_enabled = true
	e.glow_intensity = 0.5
	e.glow_bloom = 0.03
	e.glow_hdr_threshold = 1.1
	e.sdfgi_enabled = true              # light thrown back by the floor, the walls and the furniture
	e.sdfgi_cascades = 3
	e.sdfgi_min_cell_size = 0.06
	e.sdfgi_use_occlusion = true
	e.sdfgi_bounce_feedback = 0.6
	e.sdfgi_energy = 2.2
	e.sdfgi_y_scale = Environment.SDFGI_Y_SCALE_100_PERCENT
	e.volumetric_fog_enabled = true
	e.volumetric_fog_density = 0.012
	e.volumetric_fog_albedo = Color(0.95, 0.9, 0.8)
	e.volumetric_fog_anisotropy = 0.55
	e.volumetric_fog_length = 12.0
	e.volumetric_fog_ambient_inject = 0.0
	env.environment = e
	add_child(env)

	cam = Camera3D.new()
	cam.fov = 62
	cam.near = 0.08
	cam.far = 30.0
	add_child(cam)
	cam.current = true

	# the morning through the windows
	sun = DirectionalLight3D.new()
	sun.basis = Basis.looking_at(SUN.normalized(), Vector3.UP)
	sun.light_color = Color(1.0, 0.9, 0.74)
	sun.light_energy = 4.5
	sun.light_angular_distance = 0.6
	sun.light_volumetric_fog_energy = 1.6
	sun.shadow_enabled = true
	sun.shadow_blur = 1.0
	sun.directional_shadow_mode = DirectionalLight3D.SHADOW_ORTHOGONAL
	sun.directional_shadow_max_distance = 13.0
	add_child(sun)
	# the sky's own light from each window
	for z in WINDOWS:
		var sky := OmniLight3D.new()
		sky_lights.append(sky)
		sky.position = Vector3(-HALF_W + 0.5, 2.1, z)
		sky.light_color = Color(0.8, 0.88, 1.0)
		sky.light_energy = 4.5              # three times what it was: the windows' view no longer glows white, and the room lost that light
		sky.shadow_enabled = true
		sky.shadow_blur = 3.0
		sky.omni_range = 7.5
		sky.omni_attenuation = 1.4
		sky.light_specular = 0.2
		sky.light_volumetric_fog_energy = 0.0
		add_child(sky)
	# the chandelier is lit too: its warmth fills the side of the room the windows do not reach
	chandelier = OmniLight3D.new()
	chandelier.position = Vector3(0, HEIGHT - 1.0, -1.2)
	chandelier.light_color = Color(1.0, 0.8, 0.55)
	chandelier.light_energy = 1.8
	chandelier.omni_range = 7.5
	chandelier.omni_attenuation = 1.2
	chandelier.shadow_enabled = true
	chandelier.shadow_blur = 3.0
	chandelier.light_volumetric_fog_energy = 0.15
	add_child(chandelier)

	# the lamp: a pool of light on the desk under the shade
	lamp = SpotLight3D.new()
	lamp.position = Vector3(-1.22, DESK_Y + 0.4, 0.63)
	lamp.rotation_degrees = Vector3(-90, 0, 0)
	lamp.light_color = Color(1.0, 0.76, 0.48)
	lamp.light_energy = 3.2
	lamp.spot_range = 1.6
	lamp.spot_angle = 68.0
	lamp.spot_angle_attenuation = 0.6
	lamp.shadow_enabled = true
	lamp.shadow_blur = 2.0
	lamp.light_volumetric_fog_energy = 0.4
	add_child(lamp)

	# brass and polish have the room to reflect
	var probe := ReflectionProbe.new()
	probe.position = Vector3(0, HEIGHT / 2.0, (FAR_Z + BACK_Z) / 2.0)
	probe.size = Vector3(HALF_W * 2, HEIGHT, BACK_Z - FAR_Z)
	probe.origin_offset = Vector3(0, -0.4, 1.0)
	probe.box_projection = true
	probe.interior = true
	probe.ambient_mode = ReflectionProbe.AMBIENT_COLOR
	probe.ambient_color = e.ambient_light_color
	probe.ambient_color_energy = e.ambient_light_energy
	probe.update_mode = ReflectionProbe.UPDATE_ONCE
	add_child(probe)


# ---- on the walls: the map, the clock, the globe

func _build_walls() -> void:
	desk = Node3D.new()
	add_child(desk)

	# the map in its frame, on the wall the desk faces. The sheet is the real map
	var map_at := Vector3(0, 2.1, FAR_Z + 0.06)
	for side: float in [-1.0, 1.0]:
		_rbox(room, Vector3(MAP_SIZE.x + 0.26, 0.13, 0.07), 0.025, map_at + Vector3(0, side * (MAP_SIZE.y / 2.0 + 0.065), -0.01), m_dark, true)
		_rbox(room, Vector3(0.13, MAP_SIZE.y + 0.02, 0.07), 0.025, map_at + Vector3(side * (MAP_SIZE.x / 2.0 + 0.065), 0, -0.01), m_dark_up, true)
	wall_view = SubViewport.new()
	wall_view.size = Vector2i(960, 540)
	wall_view.size_2d_override = Vector2i(PaperMap.SHEET)
	wall_view.size_2d_override_stretch = true
	wall_view.disable_3d = true
	wall_view.render_target_update_mode = SubViewport.UPDATE_ONCE
	add_child(wall_view)
	wall_map = PaperMap.new()
	wall_map.wall = true
	wall_view.add_child(wall_map)
	var sheet := MeshInstance3D.new()
	var quad := QuadMesh.new()
	quad.size = MAP_SIZE
	sheet.mesh = quad
	var sm := StandardMaterial3D.new()
	sm.albedo_texture = wall_view.get_texture()
	sm.albedo_color = Color(0.95, 0.93, 0.88)
	sm.roughness = 0.95
	sm.specular_mode = BaseMaterial3D.SPECULAR_DISABLED
	sm.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
	sheet.material_override = sm
	var map_node := Node3D.new()
	map_node.position = map_at
	add_child(map_node)
	map_node.add_child(sheet)
	_register("map", "The Map", "Open the map of the world", map_node, MAP_SIZE, 0.0, false, sm, true)

	# the long-case clock, left of the map: a day goes by
	var clock := Node3D.new()
	clock.position = clock_at
	add_child(clock)
	_register("clock", "The Clock", "Stop or start the hours", clock, clock_size, 0.0, false, null, true)

	# the globe shows the same world as the map
	globe_mat.albedo_texture = wall_view.get_texture()


# ---- the desk: every thing on it is one part of the government

func _build_desk() -> void:
	var y := DESK_Y + 0.012

	# the newspaper: masthead, headline and date are today's
	_page("news", "The Newspaper", "Read the news", Vector3(-0.62, y, 1.19), 7, Vector2(0.42, 0.58), Vector2i(600, 828), DeskPrints.news)
	# the diary: the government's programme, in the head of government's own hand
	_page("focus", "The Diary", "The government's programme", Vector3(-0.13, y, 1.27), -9, Vector2(0.3, 0.4), Vector2i(480, 640), DeskPrints.programme)
	# the census: how the people live
	_page("society", "The Census", "The people and how they live", Vector3(0.0, y, 0.78), -6, Vector2(0.3, 0.42), Vector2i(480, 672), DeskPrints.census)

	# the post: letters from abroad, one sealed, one stamped
	var post := _thing_node("diplomacy", "The Post", "Letters from foreign courts", Vector3(0.27, y, 1.15), 11, Vector2(0.46, 0.34), 0.0, true)
	for k in 3:
		var env := Node3D.new()
		env.position = Vector3(-0.08 + k * 0.08, 0.005 * k, 0.03 - k * 0.03)     # 5 mm apart: closer, and each one's shadow lay in the paper under it and flickered
		env.rotation = Vector3(-PI / 2, deg_to_rad([-14.0, 3.0, 17.0][k]), 0)
		post.add_child(env)
		var em := _leaf(env, Vector2(0.3, 0.19), Vector3.ZERO, Color.WHITE)
		Print.make(self, Vector2i(480, 304), DeskPrints.envelope.bind(k), em)
		if k == 2:
			things["diplomacy"].mat = em

	# the manila folders: what the secret service knows
	var folders := _thing_node("espionage", "The Folders", "The secret service", Vector3(0.68, y, 1.15), -14, Vector2(0.38, 0.5), 0.0, true)
	for k in 3:
		var f := Node3D.new()
		f.position = Vector3(0.012 * k, 0.005 * k, -0.01 * k)
		f.rotation = Vector3(-PI / 2, deg_to_rad([6.0, -3.0, 0.0][k]), 0)
		folders.add_child(f)
		var fm := _leaf(f, Vector2(0.36, 0.48), Vector3.ZERO, Color.WHITE)
		Print.make(self, Vector2i(540, 720), DeskPrints.folder.bind(k == 2), fm)
		if k == 2:
			things["espionage"].mat = fm

	# the tray: what the ministries report
	var tray := _solid_thing("reports", "The Tray", "Reports from the ministries", Vector3(-1.1, DESK_Y, 1.2), 5, Vector3(0.36, 0.05, 0.46), Color.BLACK, "tray")
	for k in 5:                             # the reports themselves: page k from the top lies under the ones before it
		var page := Node3D.new()
		page.position = Vector3([0.0, 0.012, -0.01, 0.006, -0.004][k], 0.016 + 0.005 * (4 - k), [0.0, -0.008, 0.01, 0.004, -0.012][k])
		page.rotation = Vector3(-PI / 2, deg_to_rad([2.0, -7.0, 5.0, -3.0, 9.0][k]), 0)
		page.visible = false
		tray.add_child(page)
		var pm := _leaf(page, Vector2(0.27, 0.37), Vector3.ZERO, Color.WHITE)
		tray_pages.append({"node": page, "print": Print.make(self, Vector2i(378, 518), DeskPrints.report.bind(k), pm)})

	# the dispatch boxes of the services, and the bound books
	_solid_thing("army", "The Red Box", "The army", Vector3(-0.9, DESK_Y, 0.72), 12, Vector3(0.34, 0.12, 0.27), Color(0.78, 0.36, 0.3), "case")      # oxblood, not scarlet
	_solid_thing("navy", "The Blue Box", "The navy", Vector3(-0.5, DESK_Y, 0.63), -7, Vector3(0.34, 0.12, 0.27), Color(0.42, 0.47, 0.6), "case")      # slate, not royal blue
	_solid_thing("air", "The Steel Box", "The air service", Vector3(-0.16, DESK_Y, 0.52), 14, Vector3(0.3, 0.1, 0.17), Color(0.62, 0.68, 0.74), "steel")
	_solid_thing("research", "The Journals", "Science and invention", Vector3(0.36, DESK_Y, 0.68), -13, Vector3(0.25, 0.09, 0.36), Color(1.0, 0.9, 0.75), "stack")
	_solid_thing("budget", "The Ledger", "The treasury and the works", Vector3(0.75, DESK_Y, 0.66), 8, Vector3(0.33, 0.075, 0.46), Color(0.22, 0.2, 0.19), "book")
	_solid_thing("works", "The Order Book", "The works: what the factories build", Vector3(-1.0, DESK_Y, 1.63), 6, Vector3(0.26, 0.05, 0.36), Color(0.5, 0.56, 0.46), "book")
	_solid_thing("politics", "The Statute Book", "Government, cabinet and laws", Vector3(1.12, DESK_Y, 0.74), -19, Vector3(0.27, 0.085, 0.38), Color(0.42, 0.24, 0.18), "book")

	# the blueprints (the drafting office) are off the desk until the user settles how designs should work

	# the bell: a week goes by
	var bell := _thing_node("bell", "The Bell", "Make the hours go faster", Vector3(0.3, DESK_Y, 1.5), 0, Vector2(0.2, 0.2), 0.14, false)
	_lathe(bell, PackedVector2Array([Vector2(0, 0), Vector2(0.09, 0), Vector2(0.093, 0.012), Vector2(0.086, 0.022), Vector2(0, 0.022)]),
			Vector3.ZERO, _mat(Color(0.05, 0.035, 0.03), 0.25))
	_lathe(bell, PackedVector2Array([Vector2(0.078, 0.02), Vector2(0.074, 0.04), Vector2(0.06, 0.066), Vector2(0.035, 0.082), Vector2(0.009, 0.088),
			Vector2(0.008, 0.104), Vector2(0.013, 0.11), Vector2(0.008, 0.117), Vector2(0, 0.118)]), Vector3.ZERO, m_brass)
	_contact(bell, Vector2(0.19, 0.19), true)

	# the calendar, a tear-off block on a brass easel at the far right of the desk: a month goes by
	var cal := Node3D.new()
	cal.position = Vector3(1.22, DESK_Y + 0.165, 0.46)
	cal.rotation_degrees = Vector3(-16, -24, 0)
	desk.add_child(cal)
	_rbox(cal, Vector3(0.3, 0.34, 0.012), 0.004, Vector3(0, 0, -0.022), m_dark_up, true)
	_rbox(cal, Vector3(0.262, 0.302, 0.018), 0.002, Vector3(0, 0, -0.0095), _pbr("painted_plaster_wall", 8.0, Color(0.85, 0.82, 0.74)), true)    # the block of days to come
	var cal_mat := _leaf(cal, Vector2(0.26, 0.3), Vector3.ZERO, Color.WHITE)
	prints["calendar"] = Print.make(self, Vector2i(416, 480), DeskPrints.calendar, cal_mat)
	var prop := _rbox(cal, Vector3(0.03, 0.3, 0.01), 0.003, Vector3(0, -0.02, -0.07), m_brass, true)
	prop.rotation_degrees.x = 34
	_register("calendar", "The Calendar", "What day it is", cal, Vector2(0.3, 0.34), 0.0, false, cal_mat, true)


## The lamp and the curtains answer the hand themselves; true when `key` was one of them.
func switch(key: String) -> bool:
	if key == "lamp":
		lamp_on = not lamp_on
		lamp_bulb.emission_energy_multiplier = 4.0 if lamp_on else 0.0
		lamp_glass.emission_energy_multiplier = 0.18 if lamp_on else 0.0
		return true
	if key.begins_with("curtain"):
		var index := int(key.substr(7))
		var to := 0.0 if float(curtains[index].open) > 0.5 else 1.0
		var tw := create_tween()
		tw.tween_method(func(open: float):
			_set_curtains(index, open)
			window_light[index] = lerpf(0.12, 1.0, open), float(curtains[index].open), to, 1.1).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
		return true
	return false


## What is said when a thing on the desk has nothing in it to read; "" when it has.
func word_if_empty(key: String) -> String:
	match key:
		"reports":
			if Game.unread.is_empty():
				return "The tray is empty: no report is waiting."
		"news":
			if Game.sim.news(1).is_empty():
				return "Yesterday's paper: there is no news of note."
		"air":
			if (Game.sim.forces().wings as Array).is_empty() and Game.sim.air_designs().is_empty():
				return "The air service has no flying machines yet."
	return ""


## Fine or plain (the menu's setting): a plain room does without bounced light, haze in the air and the dark of corners
func apply_quality() -> void:
	var fine: bool = not Game.settings.plain
	env_res.sdfgi_enabled = fine
	env_res.volumetric_fog_enabled = fine
	env_res.ssao_enabled = fine
	env_res.ambient_light_energy = 0.3 if fine else 0.75     # what the bounced light gave, as a flat wash
	for l: OmniLight3D in sky_lights:
		l.shadow_enabled = fine
	chandelier.shadow_enabled = fine


## a printed sheet lying on the desk (desk_prints.gd draws it)
func _page(key: String, title: String, hint: String, at: Vector3, yaw: float, size: Vector2, px: Vector2i, painter: Callable) -> void:
	_flat_thing(key, title, hint, at, yaw, size, Color.WHITE)
	prints[key] = Print.make(self, px, painter, things[key].mat)


## paper in this room takes the room's light
func _leaf(parent: Node3D, size: Vector2, at: Vector3, tint: Color) -> StandardMaterial3D:
	return _paper(parent, size, at, tint, true)


## a thing on the desk: where it is, how big, and what the pointer and the hand do with it
func _thing_node(key: String, title: String, hint: String, at: Vector3, yaw: float, size: Vector2, height: float, flies: bool) -> Node3D:
	var n := Node3D.new()
	n.position = at
	n.rotation.y = deg_to_rad(yaw)
	desk.add_child(n)
	_register(key, title, hint, n, size, height, flies, null, false)
	return n


## a single sheet lying on the desk; returns the node its print goes on (x across, y up the page)
func _flat_thing(key: String, title: String, hint: String, at: Vector3, yaw: float, size: Vector2, tint: Color) -> Node3D:
	var n := _thing_node(key, title, hint, at, yaw, size, 0.0, true)
	var face := Node3D.new()
	face.rotation.x = -PI / 2
	n.add_child(face)
	things[key].mat = _leaf(face, size, Vector3.ZERO, tint)
	return face


## a box, a book, a pile or a tray. The cases and the books are scanned ones
func _solid_thing(key: String, title: String, hint: String, at: Vector3, yaw: float, size: Vector3, color: Color, kind: String) -> Node3D:
	var n := _thing_node(key, title, hint, at, yaw, Vector2(size.x, size.z), size.y, false)
	match kind:
		"case":                                 # a leather dispatch case, lying flat with its handle toward the chair
			var c := _model(n, "vintage_suitcase", Vector3(0, size.y / 2, 0), 0.0, 0.0, "centre", "vintage_suitcase_01")
			var own: Vector3 = c.get_meta("size")
			c.basis = Basis.from_euler(Vector3(-PI / 2, PI, 0)).scaled_local(Vector3(size.x / own.x, size.z / own.y, size.y / own.z))
			_tint(c, color)
		"steel":
			var box := _model(n, "metal_toolbox", Vector3.ZERO)       # a scanned steel box, as real as the leather cases beside it
			var scan := box.get_child(0) as Node3D
			var lid := scan.find_child("metal_toolbox_lid", true, false) as Node3D     # it was scanned open, its tray lifted out
			if lid:
				lid.rotation = Vector3.ZERO
			var inner := scan.find_child("metal_toolbox_tray", true, false)
			if inner:
				inner.free()
			scan.transform = Transform3D(Basis().scaled(Vector3.ONE * (size.x / 0.4)), Vector3.ZERO)      # the box itself is 0.4 m long
			_tint(box, color)
		"book":
			_volume(n, size, color, 0.0, 1)
		"stack":                                # journals, none quite square on the next
			for k in 3:
				var one := Node3D.new()
				one.rotation_degrees.y = [-4.0, 7.0, -3.0][k]
				n.add_child(one)
				_volume(one, Vector3(size.x - 0.02 * k, size.y / 3.0, size.z - 0.02 * k), color.darkened(0.15 * k), size.y / 3.0 * k, [5, 12, 19][k])
		"tray":
			var wood := m_wood
			_rbox(n, Vector3(size.x, 0.012, size.z), 0.004, Vector3(0, 0.006, 0), wood, true)
			for side: float in [-1.0, 1.0]:
				_rbox(n, Vector3(0.012, size.y, size.z), 0.004, Vector3(side * (size.x / 2 - 0.006), size.y / 2, 0), wood, true)
				_rbox(n, Vector3(size.x, size.y * (1.0 if side < 0 else 0.55), 0.012), 0.004,
						Vector3(0, size.y * (0.5 if side < 0 else 0.275), side * (size.z / 2 - 0.006)), wood, true)
	_contact(n, Vector2(size.x, size.z))
	return n


## one bound volume out of the scanned set, lying flat: spine to the right, `size` across, thick and deep
func _volume(n: Node3D, size: Vector3, tint: Color, y: float, which: int) -> void:
	var v := _model(n, "book_encyclopedia_set_01", Vector3.ZERO, 0.0, 0.0, "centre", "book_encyclopedia_set_01_book%02d" % which)
	var own: Vector3 = v.get_meta("size")           # as it stands on a shelf: x thick, y tall, z deep
	v.transform = Transform3D(Basis(Vector3(0, size.y / own.x, 0), Vector3(0, 0, size.z / own.y), Vector3(size.x / own.z, 0, 0)),
			Vector3(0, y + size.y / 2, 0))
	_tint(v, tint)


## the dark where a thing meets the desk: without it everything looks set down a hair above the leather
func _contact(n: Node3D, size: Vector2, round_thing := false) -> void:
	var q := PlaneMesh.new()
	q.size = size * (1.9 if round_thing else 1.45)
	var m := _flat(_soft() if round_thing else load("res://assets/room/shadow.png"), Color(0, 0, 0, 0.75) if round_thing else Color(1, 1, 1, 0.9))
	m.render_priority = -1
	_add(n, q, Vector3(0, 0.0045, 0), m).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF


func _register(key: String, title: String, hint: String, n: Node3D, size: Vector2, height: float, flies: bool, mat: StandardMaterial3D, on_wall: bool) -> void:
	things[key] = {"node": n, "title": title, "hint": hint, "size": size, "height": height, "flies": flies, "home": n.transform, "mat": mat,
			"wall": on_wall}
	var area := Area3D.new()
	var shape := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(size.x, size.y, 0.06) if on_wall else Vector3(size.x, height + 0.03, size.y)
	shape.shape = box
	area.position = Vector3.ZERO if on_wall else Vector3(0, (height + 0.03) / 2, 0)
	area.add_child(shape)
	n.add_child(area)
	area.mouse_entered.connect(_point.bind(key, true))
	area.mouse_exited.connect(_point.bind(key, false))
	area.input_event.connect(func(_c, event, _p, _n, _s):
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			picked.emit.call_deferred(key))


## the pointer finds a thing: the headquarters names it, and it lifts a little off the desk
func _point(key: String, on: bool) -> void:
	var t: Dictionary = things[key]
	if t.node == flying:
		return
	pointed.emit("%s  ·  %s" % [t.title, t.hint] if on else "")
	var mat: StandardMaterial3D = t.mat
	if mat and t.wall:
		if not t.has("tint"):
			t["tint"] = mat.albedo_color
		var base: Color = t.tint
		mat.albedo_color = Color(minf(base.r * 1.14, 1.0), minf(base.g * 1.12, 1.0), minf(base.b * 1.08, 1.0), base.a) if on else base
	if not t.wall and not t.get("fixed", false):
		var home: Transform3D = t.home
		var tw := create_tween()
		tw.tween_property(t.node, "position:y", home.origin.y + ((0.035 if t.flies else 0.012) if on else 0.0), 0.14).set_trans(Tween.TRANS_SINE)


var disc_tex: GradientTexture2D
var soft_tex: GradientTexture2D

## light that fades from the middle out, with no edge
func _soft() -> GradientTexture2D:
	if soft_tex == null:
		soft_tex = GradientTexture2D.new()
		soft_tex.fill = GradientTexture2D.FILL_RADIAL
		soft_tex.fill_from = Vector2(0.5, 0.5)
		soft_tex.fill_to = Vector2(1.0, 0.5)
		var g := Gradient.new()
		g.interpolation_mode = Gradient.GRADIENT_INTERPOLATE_CUBIC
		g.offsets = PackedFloat32Array([0.0, 0.45, 1.0])
		g.colors = PackedColorArray([Color.WHITE, Color(1, 1, 1, 0.35), Color(1, 1, 1, 0)])
		soft_tex.gradient = g
	return soft_tex


func _disc() -> GradientTexture2D:
	if disc_tex == null:
		disc_tex = GradientTexture2D.new()
		disc_tex.fill = GradientTexture2D.FILL_RADIAL
		disc_tex.fill_from = Vector2(0.5, 0.5)
		disc_tex.fill_to = Vector2(1.0, 0.5)
		var g := Gradient.new()
		g.offsets = PackedFloat32Array([0.0, 0.72, 1.0])
		g.colors = PackedColorArray([Color.WHITE, Color.WHITE, Color(1, 1, 1, 0)])
		disc_tex.gradient = g
	return disc_tex


# ---- the cigar: left burning on its ashtray

func _build_cigar(at: Vector3) -> void:
	var tray := Node3D.new()
	tray.position = at
	desk.add_child(tray)
	# a heavy dish of dark green onyx, with a notch of brass to rest the cigar in
	_lathe(tray, PackedVector2Array([Vector2(0, 0), Vector2(0.078, 0), Vector2(0.088, 0.008), Vector2(0.09, 0.03), Vector2(0.084, 0.036),
			Vector2(0.076, 0.03), Vector2(0.066, 0.014), Vector2(0, 0.012)]), Vector3.ZERO, _pbr("marble_01", 6.0, Color(0.05, 0.16, 0.1), 0.25))
	_lathe(tray, PackedVector2Array([Vector2(0, 0.0125), Vector2(0.03, 0.0135), Vector2(0.05, 0.0145), Vector2(0.06, 0.0165)]), Vector3.ZERO,
			_pbr("painted_plaster_wall", 30.0, Color(0.3, 0.29, 0.28)), 20)                  # ash in the dish
	_rbox(tray, Vector3(0.03, 0.012, 0.022), 0.004, Vector3(0.082, 0.036, 0), m_brass, true)
	_contact(tray, Vector2(0.19, 0.19), true)

	# the cigar lies across the rim, the lit end over the dish
	var cigar := _cigar(tray, true)
	cigar.position = Vector3(0.082, 0.053, 0)
	cigar.rotation_degrees = Vector3(0, 12, 83)              # along x, the mouth end out over the desk and a little higher
	var tip := Node3D.new()                                  # where the smoke leaves
	tip.position = Vector3(0, 0.072, 0)
	cigar.add_child(tip)
	ember_light = OmniLight3D.new()
	ember_light.light_color = Color(1.0, 0.4, 0.1)
	ember_light.omni_range = 0.22
	ember_light.light_specular = 0.0
	ember_light.light_volumetric_fog_energy = 0.0
	tip.add_child(ember_light)
	ember_light.position = Vector3(0.015, 0, 0)

	# the box they came from, open, its lid standing back with the maker's label inside
	var box := Node3D.new()
	box.position = at + Vector3(0.36, 0, 0.02)
	box.rotation_degrees.y = -16
	desk.add_child(box)
	var cedar := _pbr("wood_table_001", 4.0, Color(1.0, 0.8, 0.62), 0.8)
	_rbox(box, Vector3(0.215, 0.008, 0.155), 0.003, Vector3(0, 0.004, 0), cedar, true)
	for side: float in [-1.0, 1.0]:
		_rbox(box, Vector3(0.008, 0.038, 0.155), 0.003, Vector3(side * 0.1035, 0.019, 0), cedar, true)
		_rbox(box, Vector3(0.215, 0.038, 0.008), 0.003, Vector3(0, 0.019, side * 0.0735), cedar, true)
	for k in 5:                                     # one is missing: it is in the ashtray
		var c := _cigar(box, false)
		c.position = Vector3(0.012 * sin(k * 2.3), 0.021, -0.058 + k * 0.0235)
		c.rotation_degrees = Vector3(0, 0, 90)
	var lid := Node3D.new()
	lid.position = Vector3(0, 0.04, -0.0775)
	lid.rotation_degrees.x = -104
	box.add_child(lid)
	_rbox(lid, Vector3(0.215, 0.008, 0.155), 0.003, Vector3(0, 0, 0.0775), cedar, true)
	var label_mat := _mat(Color.WHITE, 0.7)
	var label := QuadMesh.new()
	label.size = Vector2(0.19, 0.13)
	_put(lid, label, Vector3(0, -0.0045, 0.0775), label_mat).rotation_degrees.x = 90
	Print.make(self, Vector2i(300, 204), DeskPrints.cigar_label, label_mat)
	_contact(box, Vector2(0.215, 0.155))

	# the smoke: a thread that wavers, then loose wisps where it breaks up
	var smoke := Node3D.new()
	add_child(smoke)
	smoke.global_position = tip.global_position
	var thread := MeshInstance3D.new()
	var q := QuadMesh.new()
	q.size = Vector2(0.5, 0.9)
	thread.mesh = q
	thread.position = Vector3(0, 0.45, 0)
	var tm := ShaderMaterial.new()
	tm.shader = load("res://assets/shaders/smoke.gdshader")
	tm.render_priority = 5              # over the papers it rises in front of
	thread.material_override = tm
	thread.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	smoke.add_child(thread)
	# what has left the thread hangs over the desk as a haze, and takes the light of the windows (only where the
	# room's air is drawn at all: the fine setting)
	var haze := FogVolume.new()
	haze.shape = RenderingServer.FOG_VOLUME_SHAPE_ELLIPSOID
	haze.size = Vector3(0.7, 0.9, 0.7)
	haze.position = Vector3(0.08, 0.75, 0)
	var hm := FogMaterial.new()
	hm.density = 0.5
	hm.albedo = Color(0.85, 0.87, 0.92)
	hm.edge_fade = 1.0
	haze.material = hm
	smoke.add_child(haze)

	var wisps := GPUParticles3D.new()
	wisps.position = Vector3(0, 0.34, 0)
	wisps.amount = 14
	wisps.lifetime = 6.0
	wisps.preprocess = 6.0
	wisps.visibility_aabb = AABB(Vector3(-1, -0.5, -1), Vector3(2, 2.5, 2))
	var pm := ParticleProcessMaterial.new()
	pm.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_SPHERE
	pm.emission_sphere_radius = 0.03
	pm.direction = Vector3(0.15, 1, 0)
	pm.spread = 12.0
	pm.initial_velocity_min = 0.07
	pm.initial_velocity_max = 0.11
	pm.gravity = Vector3(0.004, 0.006, 0)
	pm.damping_min = 0.012
	pm.damping_max = 0.02
	pm.turbulence_enabled = true
	pm.turbulence_noise_strength = 0.06
	pm.turbulence_noise_scale = 2.2
	pm.turbulence_noise_speed = Vector3(0.02, 0.05, 0.02)
	pm.turbulence_influence_min = 0.03
	pm.turbulence_influence_max = 0.07
	pm.scale_min = 0.7
	pm.scale_max = 1.3
	var grow := CurveTexture.new()
	var gc := Curve.new()
	gc.add_point(Vector2(0, 0.25))
	gc.add_point(Vector2(1, 1.0))
	grow.curve = gc
	pm.scale_curve = grow
	var fade := GradientTexture1D.new()
	var fg := Gradient.new()
	fg.offsets = PackedFloat32Array([0.0, 0.2, 1.0])
	fg.colors = PackedColorArray([Color(1, 1, 1, 0), Color(1, 1, 1, 1), Color(1, 1, 1, 0)])
	fade.gradient = fg
	pm.color_ramp = fade
	wisps.process_material = pm
	var puff := QuadMesh.new()
	puff.size = Vector2(0.2, 0.2)
	var wm := _flat(_soft(), Color(0.8, 0.82, 0.88, 0.03))
	wm.billboard_mode = BaseMaterial3D.BILLBOARD_PARTICLES
	wm.vertex_color_use_as_albedo = true
	wm.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_DISABLED
	wm.render_priority = 5
	puff.material = wm
	wisps.draw_pass_1 = puff
	wisps.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	smoke.add_child(wisps)



## a cigar, along its own y with the mouth end down: wrapper leaf, a gilt band, and when lit the ember and its ash
func _cigar(parent: Node3D, lit: bool) -> Node3D:
	var c := Node3D.new()
	parent.add_child(c)
	var leaf := ShaderMaterial.new()                # the wrapper: a leaf wound on in a spiral (cigar.gdshader)
	leaf.shader = load("res://assets/shaders/cigar.gdshader")
	# a parejo: a rounded head with its cap, a body a shade fuller in the middle, a cut foot
	var body := PackedVector2Array([Vector2(0.0, -0.066), Vector2(0.0025, -0.0657), Vector2(0.0048, -0.0648), Vector2(0.0068, -0.0632), Vector2(0.0084, -0.0608),
			Vector2(0.0096, -0.0575), Vector2(0.0103, -0.053), Vector2(0.0107, -0.046), Vector2(0.011, -0.03), Vector2(0.0112, -0.005),
			Vector2(0.0112, 0.025), Vector2(0.011, 0.045), Vector2(0.0108, 0.06)])
	if not lit:
		body.append_array([Vector2(0.0108, 0.0648), Vector2(0.0104, 0.0655), Vector2(0.0085, 0.0657), Vector2(0.0, 0.0652)])
	_lathe(c, body, Vector3.ZERO, leaf, 40)
	_lathe(c, PackedVector2Array([Vector2(0.01125, -0.036), Vector2(0.01125, -0.02)]), Vector3.ZERO, _mat(Color(0.5, 0.06, 0.05), 0.55), 24)
	var gilt := _polished(Color(0.85, 0.66, 0.3), 0.35)
	for y: float in [-0.036, -0.0222]:
		_lathe(c, PackedVector2Array([Vector2(0.0114, y), Vector2(0.0114, y + 0.0022)]), Vector3.ZERO, gilt, 24)
	_rod(c, 0.0052, 0.0006, Vector3(0, -0.028, 0.0114), gilt).rotation_degrees.x = 90     # the maker's medallion
	if lit:
		var glow := _mat(Color(0.25, 0.06, 0.02), 0.9)
		glow.emission_enabled = true
		glow.emission = Color(1.0, 0.32, 0.06)
		ember = _lathe(c, PackedVector2Array([Vector2(0.0108, 0.06), Vector2(0.0109, 0.0625)]), Vector3.ZERO, glow, 24).material_override as StandardMaterial3D
		var ash := ShaderMaterial.new()             # the ash it has grown: flaky rings, the fire in its cracks
		ash.shader = load("res://assets/shaders/cigar.gdshader")
		ash.set_shader_parameter("ash", 1.0)
		ash.set_shader_parameter("fire_at", 0.0625)
		_lathe(c, PackedVector2Array([Vector2(0.0107, 0.0625), Vector2(0.0103, 0.0642), Vector2(0.0101, 0.066), Vector2(0.0106, 0.0678), Vector2(0.0108, 0.0695),
				Vector2(0.0099, 0.0712), Vector2(0.0097, 0.073), Vector2(0.0103, 0.0748), Vector2(0.0102, 0.0765), Vector2(0.0093, 0.0787),
				Vector2(0.0084, 0.0805), Vector2(0.0066, 0.0821), Vector2(0.004, 0.083), Vector2(0, 0.0835)]), Vector3.ZERO, ash, 40)
	return c


# ---- what moves by itself

func _build_life() -> void:
	# dust in the light from the windows
	var dust := CPUParticles3D.new()
	dust.position = Vector3(-1.7, 1.5, 0.4)
	dust.amount = 90
	dust.lifetime = 9.0
	dust.preprocess = 9.0
	dust.emission_shape = CPUParticles3D.EMISSION_SHAPE_BOX
	dust.emission_box_extents = Vector3(1.6, 1.0, 1.6)
	dust.direction = Vector3(0.3, -1, 0.1)
	dust.spread = 40.0
	dust.gravity = Vector3.ZERO
	dust.initial_velocity_min = 0.008
	dust.initial_velocity_max = 0.03
	dust.scale_amount_min = 0.5
	dust.scale_amount_max = 1.3
	var fade := Gradient.new()
	fade.offsets = PackedFloat32Array([0.0, 0.25, 0.75, 1.0])
	fade.colors = PackedColorArray([Color(1, 1, 1, 0), Color.WHITE, Color.WHITE, Color(1, 1, 1, 0)])
	dust.color_ramp = fade
	var mote := QuadMesh.new()
	mote.size = Vector2(0.007, 0.007)
	var mm := _flat(_soft(), Color(1.0, 0.95, 0.8, 0.5))
	mm.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
	mm.billboard_mode = BaseMaterial3D.BILLBOARD_PARTICLES
	mm.vertex_color_use_as_albedo = true
	mote.material = mm
	dust.mesh = mote
	dust.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(dust)


## a fresh paper comes in from the left and settles on the old one
func _deliver_paper() -> void:
	var t: Dictionary = things["news"]
	var n: Node3D = t.node
	if n == flying:
		return
	var home: Transform3D = t.home
	n.position = home.origin + Vector3(-1.5, 0.12, 0.25)
	n.rotation.y = home.basis.get_euler().y + 0.7
	var tw := create_tween().set_parallel()
	tw.tween_property(n, "position", home.origin, 0.6).set_trans(Tween.TRANS_QUART).set_ease(Tween.EASE_OUT)
	tw.tween_property(n, "rotation:y", home.basis.get_euler().y, 0.6).set_trans(Tween.TRANS_QUART).set_ease(Tween.EASE_OUT)


# ---- for the headquarters: where a thing is on the screen, and the sheet that flies to the hand

## the rectangle a thing covers, in the 1600 x 900 of the interface
func screen_rect(key: String) -> Rect2:
	var t: Dictionary = things[key]
	var n: Node3D = t.node
	var s: Vector2 = t.size
	var h: float = t.height
	var corners: Array[Vector3] = []
	if t.wall:
		for c in [Vector3(-1, -1, 0), Vector3(1, -1, 0), Vector3(-1, 1, 0), Vector3(1, 1, 0)]:
			corners.append(Vector3(c.x * s.x / 2, c.y * s.y / 2, 0))
	else:
		for c in [Vector3(-1, 0, -1), Vector3(1, 0, -1), Vector3(-1, 0, 1), Vector3(1, 0, 1)]:
			corners.append(Vector3(c.x * s.x / 2, 0, c.z * s.y / 2))
			corners.append(Vector3(c.x * s.x / 2, h, c.z * s.y / 2))
	_aim(0.0)
	var k := 900.0 / float(get_viewport().get_visible_rect().size.y)
	var r := Rect2(cam.unproject_position(n.to_global(corners[0])) * k, Vector2.ZERO)
	for c in corners:
		r = r.expand(cam.unproject_position(n.to_global(c)) * k)
	return r


## the sheet leaves the desk for the hand (on) or goes back (off). A bound thing gives up one loose page.
func lift(key: String, on: bool, seconds := 0.5) -> Tween:
	var t: Dictionary = things[key]
	if on:
		look_frozen = true
		pointed.emit("")
		fly_loose = not t.flies
		if fly_loose:
			flying = Node3D.new()
			var at: Node3D = t.node
			desk.add_child(flying)
			flying.global_transform = at.global_transform.translated_local(Vector3(0, float(t.height) + 0.01, 0))
			var face := Node3D.new()
			face.rotation.x = -PI / 2
			flying.add_child(face)
			_leaf(face, Vector2(0.3, 0.4), Vector3.ZERO, Color(0.97, 0.94, 0.84))
		else:
			flying = t.node
			flying.transform = t.home
		fly_from = flying.global_transform
		_raise(flying, 20)
	if flying == null:
		look_frozen = false
		return null
	var page_h: float = 0.4 if fly_loose else maxf(float(t.size.y), 0.34)
	var d := page_h / (2.0 * 0.84 * tan(deg_to_rad(cam.fov / 2.0)))
	var cb := cam.global_basis
	var held := Transform3D(Basis(cb.x, cb.z, -cb.y), cam.global_position - cb.z * d)
	var node := flying
	var from := fly_from
	var tw := create_tween()
	tw.tween_method(func(k: float):
		var x := from.interpolate_with(held, k)
		x.origin.y += sin(k * PI) * 0.1
		node.global_transform = x, 0.0 if on else 1.0, 1.0 if on else 0.0, seconds).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_IN_OUT)
	if not on:
		var loose := fly_loose
		node.visible = true
		tw.tween_callback(func():
			if loose:
				node.queue_free()
			else:
				_raise(node, -20)
			flying = null
			look_frozen = false)
	return tw


## the sheet in the hand is drawn over everything left on the desk, print and all
func _raise(n: Node, by: int) -> void:
	if n is MeshInstance3D and n.material_override:
		n.material_override.render_priority += by
	elif n is Label3D:
		n.render_priority += by
		n.outline_render_priority += by
	for c in n.get_children():
		_raise(c, by)


## once the sheet is read on the screen, the one in the hand is out of sight
func hide_held() -> void:
	if flying:
		flying.visible = false


## something from outside, by name (ambience.gd): the headquarters and the tests ask for it
func happen(key: String) -> void:
	ambience.call("play", key)
