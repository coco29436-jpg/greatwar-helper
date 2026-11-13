class_name RoomKit
extends Node3D
## What every room of the headquarters is built from: paper with a shadow, brass buttons, things to click, and the plain
## shapes of the layout a backdrop is painted from. A room is one painting with a few live things over it.

const INK := Color(0.13, 0.09, 0.05)
const SOFT := Color(0.33, 0.25, 0.16)

var active := false             # the room on screen: the headquarters sets it
var font_title: FontFile
var font_text: FontFile


func _init() -> void:
	font_title = Game.font_title
	font_text = Game.font_text


func _clear(node: Node) -> void:
	for c in node.get_children():
		node.remove_child(c)
		c.queue_free()


func _button(parent: Node3D, sign_text: String, at: Vector3, action: Callable, size := 0.14) -> void:
	var quad := QuadMesh.new()
	quad.size = Vector2(size, size)
	var m := _flat(load("res://assets/room/brass.png"), Color.WHITE)
	m.render_priority = 2
	var mesh := _add(parent, quad, at, m)
	var label := _text(parent, sign_text, at + Vector3(0, size * 0.015, 0.006), int(170 * size / 0.14), Color(0.16, 0.1, 0.03), font_text)
	label.render_priority = 3
	_click(parent, Vector3(size * 1.07, size * 1.07, 0.04), at + Vector3(0, 0, 0.02), mesh.material_override, action)


## a sheet of paper lying on something, with a soft shadow under it. Unlit: the painting already has its light.
## With `lit` the room's own light and shadows fall on it (a room built in 3D, not painted).
func _paper(parent: Node3D, size: Vector2, at: Vector3, tint: Color, lit := false) -> StandardMaterial3D:
	var s := QuadMesh.new()
	s.size = size * Vector2(1.2, 1.238)
	var sm := _flat(load("res://assets/room/shadow.png"), Color.WHITE)
	sm.render_priority = 0
	_add(parent, s, at + Vector3(0.012, -0.02, -0.002), sm).cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	var q := QuadMesh.new()
	q.size = size
	var m := _flat(load("res://assets/room/paper.png"), tint)
	m.render_priority = 1
	if lit:                                 # a solid cut-out: it takes shadow and the dark of corners, and throws its own
		m.shading_mode = BaseMaterial3D.SHADING_MODE_PER_PIXEL
		m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA_SCISSOR
		m.alpha_scissor_threshold = 0.5
		m.cull_mode = BaseMaterial3D.CULL_DISABLED
		m.roughness = 0.95
		m.specular_mode = BaseMaterial3D.SPECULAR_DISABLED
	var i := _add(parent, q, at, m)
	if not lit:
		i.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	return i.material_override


func _flat(tex: Texture2D, tint: Color) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	m.albedo_texture = tex
	m.albedo_color = tint
	m.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
	return m


func _click(parent: Node3D, size: Vector3, at: Vector3, mat: BaseMaterial3D, action: Callable, reveal: Node3D = null, pointed := Callable()) -> void:
	var area := Area3D.new()
	area.position = at
	var shape := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = size
	shape.shape = box
	area.add_child(shape)
	parent.add_child(area)
	var base: Color = mat.albedo_color if mat else Color.WHITE
	area.mouse_entered.connect(func():
		if is_instance_valid(mat):
			mat.albedo_color = Color(min(base.r * 1.16, 1.0), min(base.g * 1.14, 1.0), min(base.b * 1.08, 1.0), base.a)
		if is_instance_valid(reveal):
			reveal.visible = true
		if pointed.is_valid():
			pointed.call())
	area.mouse_exited.connect(func():
		if is_instance_valid(mat):
			mat.albedo_color = base
		if is_instance_valid(reveal):
			reveal.visible = false)
	area.input_event.connect(func(_c, event, _p, _n, _s):
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			action.call_deferred())


# ---- small things

func _text(parent: Node3D, s: String, at: Vector3, size: int, color: Color, font: Font, align := HORIZONTAL_ALIGNMENT_CENTER) -> Label3D:
	var l := Label3D.new()
	l.text = s
	l.font = font
	l.font_size = size
	l.pixel_size = 0.00065
	l.modulate = color
	l.outline_size = 0
	l.position = at
	l.horizontal_alignment = align
	l.shaded = false
	l.double_sided = false
	l.render_priority = 3
	parent.add_child(l)
	return l


func _fit(s: String, n: int) -> String:
	return s if s.length() <= n else s.substr(0, n - 1) + "…"


func _mat(color: Color, rough: float) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.roughness = rough
	return m


func _metal(color: Color) -> StandardMaterial3D:
	var m := _mat(color.lightened(0.25), 0.42)   # the room has nothing to reflect yet, so full metal would be black
	m.metallic = 0.2
	return m


## wood and plaster for now: a colour with soft noise over it
func _textured(color: Color, rough: float, freq: float, dark: float, light: float, scale_uv: Vector3) -> StandardMaterial3D:
	var n := FastNoiseLite.new()
	n.frequency = freq
	var t := NoiseTexture2D.new()
	t.width = 512
	t.height = 512
	t.seamless = true
	t.noise = n
	var g := Gradient.new()
	g.set_color(0, color.darkened(dark))
	g.set_color(1, color.lightened(light))
	t.color_ramp = g
	var m := _mat(Color.WHITE, rough)
	m.albedo_texture = t
	m.uv1_scale = scale_uv
	return m


func _box(parent: Node3D, size: Vector3, at: Vector3, m: Material) -> MeshInstance3D:
	var b := BoxMesh.new()
	b.size = size
	return _add(parent, b, at, m)


func _sphere(parent: Node3D, r: float, at: Vector3, m: Material) -> MeshInstance3D:
	var s := SphereMesh.new()
	s.radius = r
	s.height = r * 2
	return _add(parent, s, at, m)


func _cylinder(parent: Node3D, r: float, h: float, at: Vector3, m: Material) -> MeshInstance3D:
	var c := CylinderMesh.new()
	c.top_radius = r
	c.bottom_radius = r
	c.height = h
	return _add(parent, c, at, m)


func _add(parent: Node3D, mesh: Mesh, at: Vector3, m: Material) -> MeshInstance3D:
	var i := MeshInstance3D.new()
	i.mesh = mesh
	i.position = at
	i.material_override = m.duplicate()
	parent.add_child(i)
	return i


# ---- shapes with no hard edges, for rooms built in 3D

## a box with rounded edges and corners: a ball cut in eight, its pieces pushed out to the corners
func _rbox(parent: Node3D, size: Vector3, radius: float, at: Vector3, m: Material, shared := false) -> MeshInstance3D:
	var r := minf(radius, minf(size.x, minf(size.y, size.z)) / 2.0 - 0.0001)
	var inner := size / 2.0 - Vector3(r, r, r)
	const Q := 3                                    # steps in a quarter turn
	var rows := 2 * (Q + 1)
	var cols := 4 * (Q + 1)
	var verts := PackedVector3Array()
	var normals := PackedVector3Array()
	for i in rows:
		var up := i <= Q
		var theta := (float(i if up else i - 1) / Q) * PI / 2.0
		for j in cols:
			var quarter := j / (Q + 1)
			var phi := (quarter + float(j % (Q + 1)) / Q) * PI / 2.0
			var n := Vector3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi))
			var corner := Vector3(1.0 if quarter == 0 or quarter == 3 else -1.0, 1.0 if up else -1.0, 1.0 if quarter < 2 else -1.0)
			verts.append(n * r + corner * inner)
			normals.append(n)
	var idx := PackedInt32Array()
	for i in rows - 1:
		for j in cols:
			var a := i * cols + j
			var b := i * cols + (j + 1) % cols
			var c := a + cols
			var d := b + cols
			idx.append_array([a, c, b, b, c, d])
	var last := (rows - 1) * cols
	var step := Q + 1                               # the flat top and bottom, between the four corners
	idx.append_array([0, step, step * 3, step, step * 2, step * 3])
	idx.append_array([last, last + step * 3, last + step, last + step, last + step * 3, last + step * 2])
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = normals
	arrays[Mesh.ARRAY_INDEX] = idx
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	return _put(parent, mesh, at, m) if shared else _add(parent, mesh, at, m)


## a turned shape: the outline (x out from the axis, y up) swept round the axis. Bottom to top.
func _lathe(parent: Node3D, outline: PackedVector2Array, at: Vector3, m: Material, segments := 40) -> MeshInstance3D:
	var verts := PackedVector3Array()
	var normals := PackedVector3Array()
	var idx := PackedInt32Array()
	var uvs := PackedVector2Array()                 # once round the axis, and the height in metres: for a shader that wants them
	var n := outline.size()
	for i in n:
		var t := (outline[mini(i + 1, n - 1)] - outline[maxi(i - 1, 0)]).normalized()
		var out := Vector2(t.y, -t.x)               # square to the outline, away from the axis
		for j in segments + 1:
			var a := TAU * j / segments
			verts.append(Vector3(outline[i].x * cos(a), outline[i].y, outline[i].x * sin(a)))
			normals.append(Vector3(out.x * cos(a), out.y, out.x * sin(a)))
			uvs.append(Vector2(float(j) / segments, outline[i].y))
	for i in n - 1:
		for j in segments:
			var a := i * (segments + 1) + j
			var c := a + segments + 1
			idx.append_array([a, a + 1, c, a + 1, c + 1, c])
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = verts
	arrays[Mesh.ARRAY_NORMAL] = normals
	arrays[Mesh.ARRAY_TEX_UV] = uvs
	arrays[Mesh.ARRAY_INDEX] = idx
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	var i := _add(parent, mesh, at, m)
	if i.material_override is BaseMaterial3D:
		(i.material_override as BaseMaterial3D).cull_mode = BaseMaterial3D.CULL_DISABLED     # a bowl shows its inside
	return i


## like _add, with the material shared, not copied: for the many pieces of one piece of furniture
func _put(parent: Node3D, mesh: Mesh, at: Vector3, m: Material) -> MeshInstance3D:
	var i := MeshInstance3D.new()
	i.mesh = mesh
	i.position = at
	i.material_override = m
	parent.add_child(i)
	return i


## wood, cloth, leather or plaster that lies right on any shape, with the grain along one axis
func _surface(color: Color, rough: float, freq: float, dark: float, light: float, grain := Vector3.ONE) -> StandardMaterial3D:
	var m := _textured(color, rough, freq, dark, light, grain)
	m.uv1_triplanar = true
	return m


## a scanned surface from assets/tex (colour, relief, roughness), lying right on any shape. `tiles` to the metre.
func _pbr(tex: String, tiles := 1.0, tint := Color.WHITE, rough := 1.0) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	var base := "res://assets/tex/%s_" % tex
	if ResourceLoader.exists(base + "diff.jpg"):
		m.albedo_texture = load(base + "diff.jpg")
	m.albedo_color = tint
	if ResourceLoader.exists(base + "nor.jpg"):
		m.normal_enabled = true
		m.normal_texture = load(base + "nor.jpg")
	if ResourceLoader.exists(base + "rough.jpg"):
		m.roughness_texture = load(base + "rough.jpg")
	m.roughness = rough
	m.uv1_triplanar = true
	m.uv1_scale = Vector3.ONE * tiles
	m.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
	return m


## a scanned model from assets/models, standing on `at` (anchor "floor"), hanging from it ("top") or centred on it
## ("centre"). `height` scales it to that height; 0 keeps its own size. Its size is in the meta "size".
## `only` keeps the parts whose name begins so: one book out of a set, one case out of a pair.
func _model(parent: Node3D, model: String, at: Vector3, turn := 0.0, height := 0.0, anchor := "floor", only := "") -> Node3D:
	var holder := Node3D.new()
	holder.position = at
	holder.rotation_degrees.y = turn
	parent.add_child(holder)
	var scene: PackedScene = load("res://assets/models/%s/%s.gltf" % [model, model])
	var inst: Node3D = scene.instantiate()
	if only != "":
		_keep(inst, only)
	var box: Array = [null]
	_bounds(inst, Transform3D.IDENTITY, box)
	var b: AABB = box[0]
	var s := height / b.size.y if height > 0.0 else 1.0
	var c := b.get_center()
	var foot := Vector3(c.x, b.position.y if anchor == "floor" else (b.end.y if anchor == "top" else c.y), c.z)
	inst.transform = Transform3D(Basis().scaled(Vector3.ONE * s), -foot * s)
	holder.add_child(inst)
	holder.set_meta("size", b.size * s)
	_glaze(inst)
	if OS.has_environment("GW_DIMS"):
		print("MODEL ", model, " size ", b.size, " centre ", c)
	return holder


func _keep(n: Node, only: String) -> void:
	for c in n.get_children():
		if c is MeshInstance3D and not String(c.name).begins_with(only):
			n.remove_child(c)
			c.free()
		else:
			_keep(c, only)


func _bounds(n: Node, xf: Transform3D, box: Array) -> void:
	if n is MeshInstance3D and n.mesh:
		var a: AABB = xf * (n.mesh as Mesh).get_aabb()
		box[0] = a if box[0] == null else (box[0] as AABB).merge(a)
	for c in n.get_children():
		if c is Node3D:
			_bounds(c, xf * (c as Node3D).transform, box)


## glass comes in from the scans as a solid sheet, and lamp shades unlit: clear the one and light the other
func _glaze(n: Node) -> void:
	if n is MeshInstance3D and n.mesh:
		var mi := n as MeshInstance3D
		for i in mi.mesh.get_surface_count():
			var m := mi.mesh.surface_get_material(i)
			if m == null:
				continue
			var called := m.resource_name.to_lower()
			if called.contains("glass") or called.contains("lense"):
				var g := StandardMaterial3D.new()
				g.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
				g.albedo_color = Color(0.95, 0.98, 1.0, 0.04)
				g.roughness = 0.03
				g.metallic_specular = 0.6
				if called.contains("lense"):          # a lens bends what lies under it
					g.refraction_enabled = true
					g.refraction_scale = 0.14
				mi.set_surface_override_material(i, g)
			elif called.begins_with("lamps"):
				var l := StandardMaterial3D.new()
				l.albedo_color = Color(0.95, 0.9, 0.8)
				l.emission_enabled = true
				l.emission = Color(1.0, 0.82, 0.55)
				l.emission_energy_multiplier = 1.6
				mi.set_surface_override_material(i, l)
	for c in n.get_children():
		_glaze(c)


## every surface of a model in another tint: the same books in another binding
func _tint(n: Node, tint: Color) -> void:
	if n is MeshInstance3D and n.mesh:
		var mi := n as MeshInstance3D
		for i in mi.mesh.get_surface_count():
			var m := mi.mesh.surface_get_material(i)
			if m is BaseMaterial3D:
				var copy: BaseMaterial3D = m.duplicate()
				copy.albedo_color = tint
				mi.set_surface_override_material(i, copy)
	for c in n.get_children():
		_tint(c, tint)


## polished metal, for a room with something to reflect
func _polished(color: Color, rough := 0.3) -> StandardMaterial3D:
	var m := _mat(color, rough)
	m.metallic = 0.85
	return m


# ---- paper for the flat interface

static func _label(s: String, font: Font, size: int, color: Color) -> Label:
	var l := Label.new()
	l.text = s
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.add_theme_font_override("font", font)
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", color)
	return l


static func _strip(color: Color) -> StyleBoxFlat:
	var s := StyleBoxFlat.new()
	s.bg_color = color
	s.border_color = Color(0.13, 0.09, 0.05, 0.55)
	s.set_border_width_all(1)
	s.set_corner_radius_all(3)
	s.set_content_margin_all(11)
	return s
