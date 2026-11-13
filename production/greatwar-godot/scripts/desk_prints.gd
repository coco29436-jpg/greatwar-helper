class_name DeskPrints
extends RefCounted
## What is printed on the papers of the desk: each function draws one page (see print.gd). They read the simulation
## when they are drawn, so printing a page again brings it up to date.

const INK := Color(0.13, 0.09, 0.05)
const RED := Color(0.5, 0.07, 0.05)
const PEN := Color(0.1, 0.13, 0.33)         # the head of government writes in blue-black
const LINE := Color(0.92, 0.96, 1.0)        # the white of a blueprint


static func news(c: Control) -> void:
	var w := c.size.x
	var m := 36.0
	Print.paper(c, Color(0.86, 0.83, 0.74))
	c.draw_rect(Rect2(m, 34, w - m * 2, 4), INK)
	var head_px := 46                           # a long masthead is set smaller: a line that does not fit is cut short
	while head_px > 20 and Game.font_title.get_string_size(Game.newspaper(), HORIZONTAL_ALIGNMENT_LEFT, -1, head_px).x > w - m * 2 - 8:
		head_px -= 2
	Print.text(c, Game.newspaper(), Vector2(m, 84), head_px, INK, null, w - m * 2, HORIZONTAL_ALIGNMENT_CENTER)
	c.draw_rect(Rect2(m, 96, w - m * 2, 1.5), INK)
	Print.text(c, "%s   ·   Morning Edition   ·   No. %d" % [Game.sim.date(), 11000 + hash(Game.sim.date()) % 900], Vector2(m, 113), 12,
			Color(INK, 0.8), Game.font_text, w - m * 2, HORIZONTAL_ALIGNMENT_CENTER)
	c.draw_rect(Rect2(m, 120, w - m * 2, 3), INK)
	var items: Array = Game.sim.news(3)
	var head: String = items[0].text if not items.is_empty() else "No News of Note"
	var y := 132.0
	y += Print.block(c, head, Vector2(m, y), w - m * 2, 30, INK, null, HORIZONTAL_ALIGNMENT_CENTER, 3) + 10
	c.draw_rect(Rect2(m, y, w - m * 2, 1), Color(INK, 0.7))
	y += 12
	var cw := (w - m * 2 - 28) / 3.0
	var seed_n := hash(head)
	for k in 3:
		var x := m + k * (cw + 14)
		var top := y
		if k > 0:
			c.draw_rect(Rect2(x - 7.5, y, 1, c.size.y - y - 40), Color(INK, 0.35))
		if k == 0:                              # a drop capital opens the leading article
			Print.text(c, head.substr(0, 1).to_upper(), Vector2(x, top + 44), 56, INK)
			Print.greek(c, Rect2(x + 44, top + 4, cw - 44, 44), 9, seed_n)
			top += 52
		elif k == 1:                            # an engraving, with its caption
			c.draw_rect(Rect2(x, top, cw, 128), Color(INK, 0.78))
			for i in 22:
				c.draw_line(Vector2(x, top + 6 + i * 6), Vector2(x + cw, top + i * 6 - 18), Color(0.86, 0.83, 0.74, 0.16), 2)
			c.draw_rect(Rect2(x + cw * 0.18, top + 62, cw * 0.64, 44), Color(0.86, 0.83, 0.74, 0.34))
			c.draw_circle(Vector2(x + cw * 0.7, top + 34), 16, Color(0.86, 0.83, 0.74, 0.4))
			Print.greek(c, Rect2(x + 8, top + 136, cw - 16, 16), 7, seed_n + 5, Color(INK, 0.5))
			top += 160
		else:
			var second: String = items[1].text if items.size() > 1 else "From Our Correspondents"
			top += Print.block(c, second, Vector2(x, top), cw, 15, INK, null, HORIZONTAL_ALIGNMENT_LEFT, 4) + 8
			c.draw_rect(Rect2(x, top - 4, cw * 0.4, 1), Color(INK, 0.6))
		var mid := top + (c.size.y - 40 - top) * 0.55
		Print.greek(c, Rect2(x, top, cw, mid - top - 16), 9, seed_n + k * 17 + 1)
		c.draw_rect(Rect2(x, mid - 9, cw * 0.55, 3.5), Color(INK, 0.8))        # a cross-head
		Print.greek(c, Rect2(x, mid + 2, cw, c.size.y - 40 - mid), 9, seed_n + k * 31 + 2)
	# it has been folded once across the middle
	c.draw_rect(Rect2(8, c.size.y * 0.5 - 3, w - 16, 6), Color(0, 0, 0, 0.045))
	c.draw_line(Vector2(8, c.size.y * 0.5), Vector2(w - 8, c.size.y * 0.5), Color(0, 0, 0, 0.12), 1.2)


## a report in the tray: the kth from the newest (office.gd shows as many as wait)
static func report(c: Control, k: int) -> void:
	var w := c.size.x
	Print.paper(c, [Color(0.95, 0.93, 0.86), Color(0.9, 0.88, 0.8), Color(0.93, 0.9, 0.8), Color(0.88, 0.86, 0.79), Color(0.92, 0.9, 0.84)][k])
	var n := Game.unread.size()
	if k >= n:
		return
	var r: Dictionary = Game.unread[n - 1 - k]
	Print.text(c, "MINISTRY OF STATE" if r.focus else "ROYAL ACADEMY", Vector2(0, 50), 17, Color(INK, 0.85), Game.font_text, w, HORIZONTAL_ALIGNMENT_CENTER)
	c.draw_rect(Rect2(34, 60, w - 68, 2), INK)
	c.draw_rect(Rect2(34, 65, w - 68, 1), INK)
	Print.text(c, "Carried through" if r.focus else "Discovered", Vector2(36, 96), 18, RED, Game.font_text)
	Print.text(c, String(r.date), Vector2(36, 96), 14, Color(INK, 0.7), Game.font_text, w - 72, HORIZONTAL_ALIGNMENT_RIGHT)
	var y := 112.0 + Print.block(c, String(r.name), Vector2(36, 112), w - 72, 27, INK, null, HORIZONTAL_ALIGNMENT_LEFT, 3)
	c.draw_rect(Rect2(36, y + 8, (w - 72) * 0.35, 1.5), Color(INK, 0.6))
	Print.greek(c, Rect2(36, y + 24, w - 72, c.size.y - y - 120), 11, hash(String(r.name)), Color(INK, 0.55))
	Print.stamp(c, "SEEN" if k > 0 else "URGENT", Vector2(w * 0.7, c.size.y - 64), 30, -0.2, Color(0.6, 0.08, 0.06, 0.75))


static func programme(c: Control) -> void:
	var w := c.size.x
	Print.paper(c, Color(0.9, 0.86, 0.74))
	Print.text(c, "Programme", Vector2(0, 74), 44, RED, null, w, HORIZONTAL_ALIGNMENT_CENTER)
	c.draw_rect(Rect2(40, 92, w - 80, 2), Color(RED, 0.7))
	var y := 136.0
	while y < c.size.y - 40:
		c.draw_line(Vector2(34, y), Vector2(w - 34, y), Color(0.25, 0.35, 0.55, 0.3), 1)
		y += 38
	c.draw_line(Vector2(78, 100), Vector2(78, c.size.y - 30), Color(RED, 0.4), 1.2)
	var d: Dictionary = Game.sim.focus()
	var row := 0
	if d.has("name"):
		Print.text(c, String(d.name), Vector2(90, 130), 24, PEN, Game.font_text, w - 130)
		Print.text(c, "in hand: %d of %d days" % [int(d.done), int(d.days)], Vector2(90, 166), 18, Color(PEN, 0.75), Game.font_text, w - 130)
		c.draw_line(Vector2(90, 172), Vector2(90 + (w - 150) * clampf(d.done / maxf(d.days, 1.0), 0.0, 1.0), 172), Color(PEN, 0.8), 2.5)
		row = 2
	else:
		Print.text(c, "Nothing in hand.", Vector2(90, 130), 24, RED, Game.font_text, w - 130)
		row = 1
	var open: Array = d.get("open", [])
	for f: Dictionary in open.slice(0, 9):
		var ly := 130.0 + row * 38
		if ly > c.size.y - 50:
			break
		c.draw_rect(Rect2(46, ly - 18, 18, 18), Color(PEN, 0.7), false, 1.5)
		Print.text(c, String(f.name), Vector2(90, ly), 20, Color(PEN, 0.7), Game.font_text, w - 130)
		row += 1


static func census(c: Control) -> void:
	var w := c.size.x
	Print.paper(c, Color(0.86, 0.84, 0.76))
	Print.text(c, "Census", Vector2(0, 66), 42, INK, null, w, HORIZONTAL_ALIGNMENT_CENTER)
	Print.text(c, "and Social Survey of the Nation", Vector2(0, 92), 16, Color(INK, 0.8), null, w, HORIZONTAL_ALIGNMENT_CENTER)
	c.draw_rect(Rect2(40, 104, w - 80, 2.5), INK)
	c.draw_rect(Rect2(40, 109, w - 80, 1), INK)
	var d: Dictionary = Game.sim.society()
	Print.text(c, "%.1f million souls" % d.people, Vector2(0, 140), 20, INK, Game.font_text, w, HORIZONTAL_ALIGNMENT_CENTER)
	var y := 176.0
	Print.text(c, "CLASS", Vector2(44, y), 12, Color(INK, 0.7), Game.font_text)
	Print.text(c, "SHARE", Vector2(250, y), 12, Color(INK, 0.7), Game.font_text)
	Print.text(c, "CONTENT", Vector2(350, y), 12, Color(INK, 0.7), Game.font_text)
	c.draw_rect(Rect2(40, y + 6, w - 80, 1.5), INK)
	y += 34
	for k: Dictionary in d.classes:
		Print.text(c, String(k.name), Vector2(44, y), 17, INK, Game.font_text, 200)
		Print.text(c, "%d%%" % int(round(k.share * 100.0)), Vector2(250, y), 17, INK, Game.font_text)
		c.draw_rect(Rect2(350, y - 12, 90, 10), Color(INK, 0.8), false, 1)
		c.draw_rect(Rect2(350, y - 12, 90 * clampf(k.mood / 100.0, 0.0, 1.0), 10), Color(INK, 0.6))
		c.draw_line(Vector2(40, y + 10), Vector2(w - 40, y + 10), Color(INK, 0.2), 1)
		y += 34
	for x: float in [240.0, 340.0]:
		c.draw_line(Vector2(x, 162), Vector2(x, y - 22), Color(INK, 0.3), 1)
	Print.greek(c, Rect2(44, y + 6, w - 88, c.size.y - y - 60), 10, 77, Color(INK, 0.45))


## k: 2 the letter on top, seen from the back with its seal; 1 one addressed and stamped; 0 the one underneath
static func envelope(c: Control, k: int) -> void:
	var w := c.size.x
	var h := c.size.y
	Print.paper(c, [Color(0.8, 0.76, 0.64), Color(0.87, 0.83, 0.72), Color(0.84, 0.8, 0.7)][k])
	if k == 2:
		var tip := Vector2(w * 0.5, h * 0.6)
		for from: Vector2 in [Vector2(10, 10), Vector2(w - 10, 10)]:
			c.draw_line(from + Vector2(0, 3), tip + Vector2(0, 3), Color(0, 0, 0, 0.1), 5)
			c.draw_line(from, tip, Color(INK, 0.3), 1.5)
		for from: Vector2 in [Vector2(10, h - 10), Vector2(w - 10, h - 10)]:
			c.draw_line(from, Vector2(w * 0.5, h * 0.52), Color(INK, 0.14), 1.2)
		c.draw_circle(tip + Vector2(2, 4), 33, Color(0, 0, 0, 0.18))           # the seal, pressed into wax that spread unevenly
		c.draw_circle(tip, 31, Color(0.42, 0.04, 0.04))
		c.draw_circle(tip + Vector2(-9, 7), 13, Color(0.42, 0.04, 0.04))
		c.draw_circle(tip + Vector2(11, -8), 12, Color(0.42, 0.04, 0.04))
		c.draw_circle(tip, 22, Color(0.52, 0.07, 0.06))
		c.draw_arc(tip, 22, 0, TAU, 40, Color(0.3, 0.02, 0.02), 2)
		c.draw_arc(tip, 15, 3.4, 5.2, 12, Color(0.75, 0.25, 0.2, 0.7), 2.5)
		Print.text(c, "R", tip + Vector2(-9, 10), 26, Color(0.33, 0.03, 0.03))
		return
	# the address, in a running hand
	for i in 3:
		var pts := PackedVector2Array()
		var x0 := w * 0.22 + i * 14
		var y0 := h * 0.5 + i * 34
		var n := 46 - i * 9
		for j in n:
			pts.append(Vector2(x0 + j * 5.2, y0 + sin(j * 1.9 + i) * 4.5 + sin(j * 0.6) * 2.5))
		c.draw_polyline(pts, Color(PEN, 0.85), 1.6, true)
	if k == 1:
		var s := Rect2(w - 104, 22, 76, 92)
		c.draw_rect(s, Color(0.9, 0.87, 0.78))
		c.draw_rect(s.grow(-6), Color(0.36, 0.14, 0.16))
		c.draw_circle(s.get_center() + Vector2(0, -4), 22, Color(0.78, 0.6, 0.55))
		c.draw_rect(Rect2(s.position.x + 12, s.end.y - 22, 52, 9), Color(0.78, 0.6, 0.55))
		for i in 10:                            # perforations
			c.draw_circle(Vector2(s.position.x + 4 + i * 7.6, s.position.y), 2.2, Color(0.8, 0.76, 0.64))
			c.draw_circle(Vector2(s.position.x + 4 + i * 7.6, s.end.y), 2.2, Color(0.8, 0.76, 0.64))
		var pm := Vector2(w - 150, 78)          # the postmark runs over the stamp
		c.draw_arc(pm, 36, 0, TAU, 40, Color(INK, 0.6), 2)
		c.draw_arc(pm, 25, 0, TAU, 32, Color(INK, 0.5), 1.2)
		Print.text(c, Game.sim.date().get_slice(" ", 0) + " " + Game.sim.date().get_slice(" ", 1).substr(0, 3).to_upper(), pm + Vector2(-22, 5), 12,
				Color(INK, 0.7), Game.font_text)
		for i in 5:
			var wave := PackedVector2Array()
			for j in 20:
				wave.append(Vector2(pm.x + 38 + j * 5, pm.y - 22 + i * 11 + sin(j * 0.9) * 3))
			c.draw_polyline(wave, Color(INK, 0.5), 1.4, true)


static func folder(c: Control, top: bool) -> void:
	var w := c.size.x
	var h := c.size.y
	Print.paper(c, Color(0.78, 0.64, 0.4) if top else Color(0.7, 0.57, 0.36))
	c.draw_rect(Rect2(14, 14, w - 28, h - 28), Color(0.3, 0.2, 0.08, 0.16), false, 2)
	if not top:
		return
	var label := Rect2(44, 70, 300, 104)
	c.draw_rect(label.grow(2), Color(0, 0, 0, 0.12))
	c.draw_rect(label, Color(0.93, 0.9, 0.82))
	c.draw_rect(label, Color(INK, 0.5), false, 1.2)
	Print.text(c, "DOSSIER  No. %d" % (300 + hash(Game.sim.nation().tag) % 600), label.position + Vector2(12, 28), 18, INK, Game.font_text)
	Print.greek(c, Rect2(label.position + Vector2(12, 42), Vector2(270, 50)), 13, 9, Color(INK, 0.6), 0.3)
	Print.stamp(c, "SECRET", Vector2(w * 0.56, h * 0.42), 74, -0.24, Color(0.62, 0.08, 0.06, 0.82))
	Print.text(c, String(Game.sim.espionage().agency), Vector2(0, h * 0.7), 24, Color(INK, 0.8), Game.font_text, w, HORIZONTAL_ALIGNMENT_CENTER)
	c.draw_rect(Rect2(w * 0.3, h * 0.72, w * 0.4, 1.2), Color(INK, 0.5))
	Print.text(c, "For the head of government only", Vector2(0, h * 0.76), 15, Color(INK, 0.65), Game.font_text, w, HORIZONTAL_ALIGNMENT_CENTER)
	# a paperclip over the top edge
	var steel := Color(0.56, 0.58, 0.6)
	for inset: float in [0.0, 6.0]:
		c.draw_rect(Rect2(w - 120 + inset, 2 + inset * 0.4, 22 - inset * 2, 78 - inset * 5), steel, false, 2.6)
	c.draw_line(Vector2(w - 119, 60), Vector2(w - 98, 60), Color(0, 0, 0, 0.18), 3)


static func calendar(c: Control) -> void:
	var w := c.size.x
	var parts := Game.sim.date().split(" ")
	Print.paper(c, Color(0.93, 0.9, 0.82))
	c.draw_rect(Rect2(10, 10, w - 20, 104), Color(0.46, 0.07, 0.05))
	Print.text(c, parts[1].to_upper() if parts.size() > 1 else "", Vector2(0, 66), 42, Color(0.96, 0.92, 0.82), null, w, HORIZONTAL_ALIGNMENT_CENTER)
	Print.text(c, parts[2] if parts.size() > 2 else "", Vector2(0, 98), 22, Color(0.96, 0.92, 0.82, 0.85), Game.font_text, w, HORIZONTAL_ALIGNMENT_CENTER)
	Print.text(c, parts[0], Vector2(0, 330), 220, INK, null, w, HORIZONTAL_ALIGNMENT_CENTER)
	if parts.size() > 2:
		Print.text(c, weekday(int(parts[0]), parts[1], int(parts[2])), Vector2(0, 392), 36, RED, null, w, HORIZONTAL_ALIGNMENT_CENTER)
	c.draw_rect(Rect2(40, 412, w - 80, 1.5), Color(INK, 0.5))
	Print.greek(c, Rect2(60, 424, w - 120, 30), 11, int(parts[0]) * 7, Color(INK, 0.5), 0.3)
	for x: float in [w * 0.3, w * 0.7]:        # where the block hangs on its rings
		c.draw_circle(Vector2(x, 26), 9, Color(0.1, 0.07, 0.05))
		c.draw_arc(Vector2(x, 20), 12, PI, TAU, 14, Color(0.7, 0.6, 0.35), 4)


static func weekday(day: int, month_name: String, year: int) -> String:
	const MONTHS := ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"]
	var m := MONTHS.find(month_name) + 1
	if m <= 0:
		return ""
	var y := year
	if m < 3:
		m += 12
		y -= 1
	var k := (day + (13 * (m + 1)) / 5 + y % 100 + (y % 100) / 4 + (y / 100) / 4 + 5 * (y / 100)) % 7     # Zeller: 0 is Saturday
	return ["Saturday", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"][k]


# ---- the blueprint: a master drawing of what the works are building

const PARTS := {
	"ship": ["MAIN BATTERY", "FIRE CONTROL TOP", "BOILER ROOMS", "ENGINE ROOM", "MAGAZINES", "ARMOUR BELT", "CONNING TOWER", "STEERING GEAR",
			"TORPEDO FLAT", "COAL BUNKERS"],
	"gun": ["BARREL", "RECOIL CYLINDER", "BREECH BLOCK", "ELEVATING GEAR", "TRAIL", "SHIELD", "SIGHT BRACKET", "AXLE TREE", "TRAVERSING SCREW",
			"LIMBER HOOK"],
	"air": ["AIRSCREW", "ENGINE BEARERS", "MAIN SPAR", "WARPING WIRES", "RUDDER POST", "UNDERCARRIAGE", "FUEL TANK", "PILOT'S SEAT", "KING POST",
			"TAIL SKID"],
	"car": ["ENGINE", "GEAR BOX", "DRIVING SPROCKET", "ARMOUR PLATE", "FIGHTING COMPARTMENT", "FUEL TANK", "RADIATOR", "SPRINGS", "STEERING BOX",
			"TOW HOOK"],
	"arm": ["RECEIVER", "BOLT", "MAGAZINE", "FORE-END", "SIGHT LEAF", "BARREL BAND", "BUTT PLATE", "TRIGGER GUARD", "SLING SWIVEL", "CLEANING ROD"],
}


static func _family(line: Dictionary) -> String:
	var s := (String(line.get("category", "")) + " " + String(line.get("kind", "")) + " " + String(line.get("chassis", ""))).to_lower()
	for pair: Array in [["ship", "ship"], ["cruiser", "ship"], ["destroyer", "ship"], ["submarine", "ship"], ["carrier", "ship"], ["hull", "ship"],
			["naval", "ship"], ["plane", "air"], ["air", "air"], ["bomber", "air"], ["fighter", "air"], ["gun", "gun"], ["artil", "gun"],
			["howitzer", "gun"], ["mortar", "gun"], ["tank", "car"], ["car", "car"], ["truck", "car"], ["tractor", "car"], ["loco", "car"]]:
		if s.contains(pair[0]):
			return pair[1]
	return "arm"


## the lines of the works that have a drawing, the largest machine first
static func drawn_lines() -> Array:
	var out := []
	for l: Dictionary in Game.sim.lines():
		if ResourceLoader.exists("res://assets/blueprints/%s.png" % l.chassis):
			out.append(l)
	var rank := {"ship": 0, "air": 1, "car": 2, "gun": 3, "arm": 4}
	out.sort_custom(func(a, b): return rank[_family(a)] < rank[_family(b)])
	return out


static func blueprint(c: Control) -> void:
	var w := c.size.x
	var h := c.size.y
	var f := Game.font_text
	Print.paper(c, Color(0.1, 0.2, 0.4))
	var x := 40.0
	while x < w - 30:                           # the squared ground
		c.draw_line(Vector2(x, 30), Vector2(x, h - 30), Color(LINE, 0.1 if int(x - 40) % 100 == 0 else 0.045), 1)
		x += 20
	var y := 40.0
	while y < h - 30:
		c.draw_line(Vector2(30, y), Vector2(w - 30, y), Color(LINE, 0.1 if int(y - 40) % 100 == 0 else 0.045), 1)
		y += 20
	# the border, zoned in numbers and letters like any master sheet
	c.draw_rect(Rect2(22, 22, w - 44, h - 44), Color(LINE, 0.55), false, 1)
	c.draw_rect(Rect2(36, 36, w - 72, h - 72), Color(LINE, 0.9), false, 2)
	for i in 10:
		var zx := 36 + (w - 72) * (i + 0.5) / 10.0
		for zy: float in [33.0, h - 25.0]:
			Print.text(c, str(10 - i), Vector2(zx - 4, zy), 10, Color(LINE, 0.8), f)
		c.draw_line(Vector2(36 + (w - 72) * i / 10.0, 22), Vector2(36 + (w - 72) * i / 10.0, 36), Color(LINE, 0.6), 1)
		c.draw_line(Vector2(36 + (w - 72) * i / 10.0, h - 36), Vector2(36 + (w - 72) * i / 10.0, h - 22), Color(LINE, 0.6), 1)
	for i in 8:
		var zy := 36 + (h - 72) * (i + 0.5) / 8.0
		for zx: float in [26.0, w - 33.0]:
			Print.text(c, char(65 + i), Vector2(zx, zy + 4), 10, Color(LINE, 0.8), f)
		c.draw_line(Vector2(22, 36 + (h - 72) * i / 8.0), Vector2(36, 36 + (h - 72) * i / 8.0), Color(LINE, 0.6), 1)
		c.draw_line(Vector2(w - 36, 36 + (h - 72) * i / 8.0), Vector2(w - 22, 36 + (h - 72) * i / 8.0), Color(LINE, 0.6), 1)

	var lines := drawn_lines()
	var nation: Dictionary = Game.sim.nation()
	if lines.is_empty():
		Print.text(c, "NO DRAWING ON THE BOARD", Vector2(0, h * 0.5), 26, Color(LINE, 0.8), f, w, HORIZONTAL_ALIGNMENT_CENTER)
		return
	var main: Dictionary = lines[0]
	var rng := RandomNumberGenerator.new()
	rng.seed = hash(String(main.chassis))

	# the principal view, with its parts called out
	var box := Rect2(210, 96, 440, 300)
	var tex: Texture2D = load("res://assets/blueprints/%s.png" % main.chassis)
	var shown := Vector2(box.size.x, box.size.x * tex.get_height() / tex.get_width())
	if shown.y > box.size.y:
		shown = Vector2(box.size.y * tex.get_width() / tex.get_height(), box.size.y)
	var art := Rect2(box.get_center() - shown / 2.0, shown)
	Print.tracing(c, tex, art, 1.25)
	Print.text(c, "FIG. 1", Vector2(60, 76), 12, Color(LINE, 0.9), f)
	Print.text(c, String(main.name).to_upper(), Vector2(104, 76), 16, LINE, f)
	c.draw_rect(Rect2(60, 82, 340, 1.2), Color(LINE, 0.8))
	var names: Array = PARTS[_family(main)]
	for i in 8:
		var left := i % 2 == 0
		var row := i / 2
		var label := Vector2(60.0 if left else 668.0, 122.0 + row * 70.0 + (0.0 if left else 26.0))
		var target := art.position + Vector2(art.size.x * (0.14 + 0.72 * rng.randf() * (0.5 if left else 1.0) + (0.0 if left else 0.14)),
				art.size.y * rng.randf_range(0.3, 0.72))
		var code := "S-SECTOR %d%s" % [rng.randi_range(10, 99), char(65 + rng.randi_range(0, 5))]
		Print.text(c, names[i], label, 12, LINE, f, 132, HORIZONTAL_ALIGNMENT_LEFT if left else HORIZONTAL_ALIGNMENT_RIGHT)
		Print.text(c, code, label + Vector2(0, 12), 8, Color(LINE, 0.7), f, 132, HORIZONTAL_ALIGNMENT_LEFT if left else HORIZONTAL_ALIGNMENT_RIGHT)
		var shelf_from := label + Vector2(0.0 if left else 132.0, 3)
		var shelf_to := label + Vector2(140.0 if left else -8.0, 3)
		c.draw_line(shelf_from, shelf_to, Color(LINE, 0.85), 1, true)
		c.draw_line(shelf_to, target, Color(LINE, 0.85), 1, true)
		c.draw_circle(target, 2.4, LINE)
	# its length, between witness lines
	var dim_y := art.end.y + 26
	for dx: float in [art.position.x, art.end.x]:
		c.draw_line(Vector2(dx, art.end.y + 4), Vector2(dx, dim_y + 6), Color(LINE, 0.7), 1)
	c.draw_line(Vector2(art.position.x, dim_y), Vector2(art.end.x, dim_y), Color(LINE, 0.8), 1)
	for end: Array in [[art.position.x, 1.0], [art.end.x, -1.0]]:
		c.draw_colored_polygon(PackedVector2Array([Vector2(end[0], dim_y), Vector2(end[0] + 9 * end[1], dim_y - 3), Vector2(end[0] + 9 * end[1], dim_y + 3)]), LINE)
	Print.text(c, "LENGTH OVER ALL", Vector2(art.position.x, dim_y - 4), 9, Color(LINE, 0.85), f, art.size.x, HORIZONTAL_ALIGNMENT_CENTER)
	# trial circles round it, as the draughtsman swung them
	for k in 3:
		c.draw_arc(art.get_center() + Vector2(0, 16 * k), art.size.x * (0.56 + 0.07 * k), 2.5 + k * 0.4, 3.9 + k * 0.3, 40, Color(LINE, 0.25), 1, true)
		c.draw_arc(art.get_center() - Vector2(0, 16 * k), art.size.x * (0.56 + 0.07 * k), -0.6 + k * 0.3, 0.7 - k * 0.1, 40, Color(LINE, 0.25), 1, true)

	# the other views, in their own frame
	var frame := Rect2(w - 236, 52, 184, 330)
	c.draw_rect(frame, Color(LINE, 0.07))
	c.draw_rect(frame, Color(LINE, 0.85), false, 1.5)
	var k := 0
	for other: Dictionary in lines.slice(1, 3):
		var cell := Rect2(frame.position + Vector2(10, 14 + k * 160), Vector2(frame.size.x - 20, 120))
		Print.tracing(c, load("res://assets/blueprints/%s.png" % other.chassis), cell, 1.1)
		Print.text(c, "FIG. %d   %s" % [k + 2, String(other.name).to_upper()], Vector2(cell.position.x, cell.end.y + 14), 8, Color(LINE, 0.9), f,
				cell.size.x, HORIZONTAL_ALIGNMENT_CENTER)
		c.draw_line(Vector2(cell.position.x, cell.end.y + 2), Vector2(cell.end.x, cell.end.y + 2), Color(LINE, 0.5), 1)
		k += 1
	if lines.size() < 2:                        # nothing else building: the same machine from above and ahead
		Print.tracing(c, tex, Rect2(frame.position + Vector2(10, 20), Vector2(frame.size.x - 20, 130)), 0.9)
		Print.text(c, "PROFILE", Vector2(frame.position.x, frame.position.y + 170), 9, Color(LINE, 0.9), f, frame.size.x, HORIZONTAL_ALIGNMENT_CENTER)

	# details and the general notes, lower left
	Print.text(c, "ASSEMBLING", Vector2(60, 452), 12, LINE, f)
	Print.text(c, "sheets 21 through 43", Vector2(60, 465), 8, Color(LINE, 0.75), f)
	for i in 3:
		var centre := Vector2(110 + i * 118, 540)
		c.draw_arc(centre, 46, 0, TAU, 48, Color(LINE, 0.85), 1.2, true)
		c.draw_arc(centre, 50, 0.3 + i, 1.6 + i, 16, Color(LINE, 0.5), 1, true)
		var part := Rect2(art.position + art.size * Vector2(rng.randf_range(0.1, 0.7), rng.randf_range(0.2, 0.5)), art.size * 0.2)
		var at := AtlasTexture.new()            # a part of the principal view, enlarged
		at.atlas = tex
		at.region = Rect2((part.position - art.position) / art.size * Vector2(tex.get_size()), part.size / art.size * Vector2(tex.get_size()))
		Print.tracing(c, at, Rect2(centre - Vector2(34, 34), Vector2(68, 68)), 1.2)
		Print.text(c, "DETAIL %s" % char(65 + i), Vector2(centre.x - 50, centre.y + 64), 9, Color(LINE, 0.9), f, 100, HORIZONTAL_ALIGNMENT_CENTER)
	Print.text(c, "NOTES", Vector2(60, 640), 11, LINE, f)
	for i in 4:
		Print.text(c, "%d." % (i + 1), Vector2(60, 660 + i * 22), 9, Color(LINE, 0.85), f)
		Print.greek(c, Rect2(78, 652 + i * 22, 330, 18), 9, 40 + i, Color(LINE, 0.6), 0.3)

	# the title block
	var t := Rect2(w - 420, h - 196, 368, 144)
	c.draw_rect(t, Color(0.1, 0.2, 0.4))
	c.draw_rect(t, LINE, false, 2)
	c.draw_rect(t.grow(4), Color(LINE, 0.7), false, 1)
	c.draw_line(t.position + Vector2(210, 0), t.position + Vector2(210, 144), LINE, 1)
	c.draw_line(t.position + Vector2(0, 86), t.position + Vector2(210, 86), LINE, 1)
	c.draw_line(t.position + Vector2(0, 115), t.position + Vector2(210, 115), LINE, 1)
	c.draw_line(t.position + Vector2(210, 60), t.end - Vector2(0, 84), LINE, 1)
	c.draw_line(t.position + Vector2(290, 60), t.position + Vector2(290, 144), LINE, 1)
	Print.text(c, String(nation.name).to_upper(), t.position + Vector2(10, 22), 13, LINE, f, 196)
	Print.text(c, "MASTER DRAWING", t.position + Vector2(10, 46), 20, LINE, f, 196)
	Print.text(c, String(main.name), t.position + Vector2(10, 64), 11, Color(LINE, 0.9), f, 196)
	Print.text(c, "SHEET 2 OUT OF 51 SHEETS", t.position + Vector2(10, 78), 8, Color(LINE, 0.8), f)
	Print.text(c, "no.", t.position + Vector2(6, 96), 7, Color(LINE, 0.7), f)
	Print.text(c, "ARS-%06d" % (hash(String(main.chassis)) % 100000), t.position + Vector2(40, 106), 13, LINE, f)
	Print.text(c, "date", t.position + Vector2(6, 125), 7, Color(LINE, 0.7), f)
	Print.text(c, Game.sim.date().get_slice(" ", 1).substr(0, 3) + ". " + Game.sim.date().get_slice(" ", 2), t.position + Vector2(40, 135), 13, LINE, f)
	Print.text(c, "all units in metres", t.position + Vector2(218, 20), 11, LINE, f)
	Print.text(c, "ref %s" % String.num_int64(hash(String(main.name)) % 4096, 16).pad_zeros(4), t.position + Vector2(218, 48), 20, LINE, f)
	Print.block(c, "THIS DRAWING IS THE PROPERTY OF THE STATE AND MUST NOT BE ALTERED WITHOUT ORDER", t.position + Vector2(216, 64), 70, 7, Color(LINE, 0.9), f,
			HORIZONTAL_ALIGNMENT_LEFT, 8)
	Print.text(c, "DO NOT", t.position + Vector2(296, 96), 12, LINE, f, 68, HORIZONTAL_ALIGNMENT_CENTER)
	Print.text(c, "SCALE", t.position + Vector2(296, 112), 12, LINE, f, 68, HORIZONTAL_ALIGNMENT_CENTER)
	c.draw_line(t.position + Vector2(302, 118), t.position + Vector2(358, 118), LINE, 1)


## the lithographed label inside the lid of the cigar box
static func cigar_label(c: Control) -> void:
	var w := c.size.x
	var h := c.size.y
	c.draw_rect(Rect2(0, 0, w, h), Color(0.9, 0.84, 0.66))
	c.draw_rect(Rect2(8, 8, w - 16, h - 16), Color(0.62, 0.45, 0.14), false, 5)
	c.draw_rect(Rect2(18, 18, w - 36, h - 36), Color(0.5, 0.07, 0.05), false, 2)
	c.draw_set_transform(Vector2(w / 2, h / 2), 0, Vector2(1.7, 1.0))
	c.draw_circle(Vector2.ZERO, h * 0.3, Color(0.5, 0.07, 0.05))
	c.draw_arc(Vector2.ZERO, h * 0.3, 0, TAU, 48, Color(0.72, 0.55, 0.18), 3)
	c.draw_set_transform(Vector2.ZERO, 0, Vector2.ONE)
	Print.text(c, "FLOR FINA", Vector2(0, h / 2 + 2), 30, Color(0.93, 0.82, 0.5), null, w, HORIZONTAL_ALIGNMENT_CENTER)
	Print.text(c, "HABANA", Vector2(0, h / 2 + 26), 15, Color(0.93, 0.82, 0.5), Game.font_text, w, HORIZONTAL_ALIGNMENT_CENTER)
