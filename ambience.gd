extends Node
## What reaches the office from the capital outside: a cloud over the sun, an aeroplane past the windows, a carriage in
## the street, the clock striking, rain. Each happening has a condition, a weight, something seen and the name of a
## sound. There are no sounds yet: play() looks for res://assets/sounds/<name>.ogg and is silent without it, so a
## sound is added by putting the file there. Nothing here touches the simulation; it only reads it.
##
## To add a happening: a row in HAPPENINGS, a branch in _can() if it has a condition, and a branch in play() if there
## is something to see.

const SOUNDS := "res://assets/sounds/%s.ogg"
const EVERY := Vector2(20.0, 60.0)      # seconds between two happenings
const HAPPENINGS := {                   # key -> [weight, sound]
	"cloud": [3.0, ""],
	"plane": [1.0, "plane_overhead"],
	"carriage": [3.0, "street_carriage"],
	"chime": [0.0, "clock_chime"],      # weight 0: never by chance, only when asked for (a day goes by on the clock)
}
const BAD_WEATHER_SOUND := {1: "rain_window", 2: "storm_window", 5: "wind_window"}     # by the simulation's weather: rain, storm, blizzard

var office                              # office.gd: its lights and its windows
var rng := RandomNumberGenerator.new()
var wait := 0.0
var player: AudioStreamPlayer           # one-off sounds
var weather_loop: AudioStreamPlayer     # the rain, for as long as it rains
var plane: MeshInstance3D
var facts := {}                         # the simulation's word on the capital today
var sun_full := 0.0
var sky_full := 0.0
var cloud := 0.0                        # 0 clear .. 1 the sun is behind a cloud
var overcast := 0.0                     # the same from the day's weather, until the weather changes
var tremble := 0.0
var quiet := false                      # tests: nothing happens by chance


func _init(room) -> void:
	office = room


func _ready() -> void:
	rng.randomize()
	quiet = OS.has_environment("GW_SHOT")
	wait = rng.randf_range(EVERY.x, EVERY.y)
	player = AudioStreamPlayer.new()
	add_child(player)
	weather_loop = AudioStreamPlayer.new()
	add_child(weather_loop)
	sun_full = office.sun.light_energy
	sky_full = office.sky_lights[0].light_energy
	_build_plane()
	Game.changed.connect(_read_capital)
	_read_capital()


func _process(delta: float) -> void:
	if not office.active:
		return
	# the light of the day: the weather sets it, a passing cloud takes more for a while
	var shade := maxf(cloud, overcast)
	office.sun.light_energy = sun_full * (1.0 - 0.85 * shade)
	for i in office.sky_lights.size():      # each window gives what its curtains let through
		(office.sky_lights[i] as OmniLight3D).light_energy = sky_full * (1.0 - 0.3 * shade) * float(office.window_light[i])
	if tremble > 0.0:                       # the chandelier's crystals, when something heavy goes over
		tremble = maxf(tremble - delta * 0.25, 0.0)
		office.chandelier.light_energy = 1.8 * (1.0 + 0.06 * tremble * sin(Time.get_ticks_msec() * 0.045))
	if quiet:
		return
	wait -= delta
	if wait <= 0.0:
		wait = rng.randf_range(EVERY.x, EVERY.y)
		_by_chance()


func _read_capital() -> void:
	facts = Game.sim.capital()
	var weather := int(facts.get("weather", 0))
	overcast = 0.0 if weather == 0 or weather == 6 else 0.8        # clear and heat are the only days with sun
	var sound: String = BAD_WEATHER_SOUND.get(weather, "")
	var stream := _stream(sound)
	if stream != weather_loop.stream:
		weather_loop.stream = stream
		if stream:
			weather_loop.play()
		else:
			weather_loop.stop()


func _can(key: String) -> bool:
	match key:
		"plane": return int(facts.get("wings", 0)) > 0 or int(facts.get("year", 1906)) >= 1910
		"cloud": return overcast < 0.5
	return true


func _by_chance() -> void:
	var total := 0.0
	for key: String in HAPPENINGS:
		if _can(key):
			total += HAPPENINGS[key][0]
	var pick := rng.randf() * total
	for key: String in HAPPENINGS:
		if not _can(key):
			continue
		pick -= HAPPENINGS[key][0]
		if pick <= 0.0:
			play(key)
			return


## a happening, now: what is seen of it, and its sound if there is one
func play(key: String) -> void:
	if not HAPPENINGS.has(key):
		push_warning("Ambience: no happening called " + key)
		return
	if OS.has_environment("GW_AMBIENT"):
		print("ambience: ", key)
	_sound(HAPPENINGS[key][1])
	match key:
		"cloud":
			var tw := create_tween()
			tw.tween_property(self, "cloud", 1.0, 3.5).set_trans(Tween.TRANS_SINE)
			tw.tween_interval(rng.randf_range(3.0, 9.0))
			tw.tween_property(self, "cloud", 0.0, 4.5).set_trans(Tween.TRANS_SINE)
		"plane":
			# past the far window, then the near one, climbing a little; the wall hides it between them
			var x: float = -office.HALF_W - 0.19
			var from := Vector3(x, 2.35, office.WINDOWS[0] - 1.0)
			var to := Vector3(x, 2.75, office.WINDOWS[1] + 1.0)
			plane.position = from
			plane.visible = true
			tremble = 1.0
			var tw := create_tween()
			tw.tween_property(plane, "position", to, 7.0)
			tw.tween_callback(func(): plane.visible = false)


func _stream(sound: String) -> AudioStream:
	if sound == "" or not ResourceLoader.exists(SOUNDS % sound):
		return null
	return load(SOUNDS % sound)


func _sound(sound: String) -> void:
	var stream := _stream(sound)
	if stream:
		player.stream = stream
		player.play()


## a biplane far off, seen from the side against the sky: a dark shape just inside the glass
func _build_plane() -> void:
	var img := Image.create(96, 40, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))
	var ink := Color(0.16, 0.17, 0.2, 0.8)
	img.fill_rect(Rect2i(14, 18, 62, 5), ink)       # fuselage, nose to the right
	img.fill_rect(Rect2i(6, 17, 12, 3), ink)        # the tail boom thins
	img.fill_rect(Rect2i(4, 9, 5, 11), ink)         # fin
	img.fill_rect(Rect2i(2, 17, 12, 2), ink)        # tailplane
	img.fill_rect(Rect2i(44, 8, 30, 2), ink)        # upper wing, seen edge on
	img.fill_rect(Rect2i(46, 23, 26, 2), ink)       # lower wing
	for sx in [48, 68]:
		img.fill_rect(Rect2i(sx, 10, 1, 13), ink)   # struts
	img.fill_rect(Rect2i(76, 17, 4, 7), ink)        # engine
	img.fill_rect(Rect2i(81, 10, 1, 21), Color(ink, 0.35))      # the propeller's blur
	img.fill_rect(Rect2i(56, 25, 1, 7), ink)        # undercarriage
	img.fill_rect(Rect2i(54, 31, 6, 4), ink)
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	m.albedo_texture = ImageTexture.create_from_image(img)
	m.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR
	var q := QuadMesh.new()
	q.size = Vector2(0.42, 0.175)
	plane = MeshInstance3D.new()
	plane.mesh = q
	plane.material_override = m
	plane.rotation_degrees.y = 90               # its face to the room, its nose toward the near window
	plane.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	plane.visible = false
	office.add_child(plane)
