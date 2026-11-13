class_name Sheet
extends PanelContainer
## A sheet of paper with print on it: what a thing on the desk says when it is picked up. `Sheet.make(key)` writes the
## sheet for one thing from the simulation as it stands today. Some sheets act: a button on them gives an order to the
## simulation (act), and the headquarters writes the sheet again.

const INK := Color(0.13, 0.09, 0.05)
const SOFT := Color(0.36, 0.28, 0.19)
const RED := Color(0.56, 0.08, 0.06)       # stamp ink only: red is a nation's colour, never an accent (godot.md)
const BAD := Color(0.3, 0.15, 0.07)        # bad news: dark sepia, a shade warmer than the ink
const GOOD := Color(0.3, 0.34, 0.2)        # good news: a muted olive
const MAX_H := 830.0                    # a sheet longer than this is printed smaller
const PAD := 16.0                       # the shadow round a sheet, beyond its edge
const PAPER := preload("res://assets/shaders/paper_sheet.gdshader")
static var white: ImageTexture

var col: VBoxContainer                  # where the next line goes
var main: VBoxContainer
var over: Control                       # for a stamp across the page

static var lab := 0                     # the laboratory whose subject is being chosen, and whether it is an experimental one
static var lab_experimental := false
static var court := ""                  # the government a letter is being written to
static var post := -1                   # the seat in the cabinet being filled
static var leaf := {}                   # list name -> which leaf of a long list is showing


func _init(tint := Color(1.0, 0.97, 0.88), min_size := Vector2(1080, 0), margin := 46) -> void:
	custom_minimum_size = min_size
	size = min_size                 # so wrapped lines know their width before the first layout
	# the paper itself is drawn by a shader over the stylebox, which reaches PAD beyond the sheet for its shadow
	var box := StyleBoxTexture.new()
	box.texture = _white()
	box.set_content_margin_all(margin)
	box.set_expand_margin_all(PAD)
	add_theme_stylebox_override("panel", box)
	var m := ShaderMaterial.new()
	m.shader = PAPER
	m.set_shader_parameter("tint", tint)
	m.set_shader_parameter("pad", PAD)
	m.set_shader_parameter("seed", float(get_instance_id() % 997) / 997.0)
	material = m
	resized.connect(func(): m.set_shader_parameter("size", size + Vector2.ONE * PAD * 2.0))
	main = VBoxContainer.new()
	main.add_theme_constant_override("separation", 7)
	add_child(main)
	col = main
	over = Control.new()
	over.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(over)


static func _white() -> ImageTexture:
	if white == null:
		var img := Image.create(4, 4, false, Image.FORMAT_RGBA8)
		img.fill(Color.WHITE)
		white = ImageTexture.create_from_image(img)
	return white


# ---- the print

func _line(s: String, font: Font, size: int, color: Color) -> Label:
	var l := RoomKit._label(s, font, size, color)
	col.add_child(l)
	return l


func title(s: String, size := 38, color := INK) -> Label:
	var l := _line(s, Game.font_title, size, color)
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	return l


func note(s: String, size := 17) -> Label:
	var l := _line(s, Game.font_text, size, SOFT)
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	return l


## a printed heading: dark ink, and under it the thick-and-thin rule of a printer's form
func heading(s: String, color := INK) -> void:
	gap(4)
	_line(s, Game.font_title, 22, color)
	var pair := VBoxContainer.new()
	pair.add_theme_constant_override("separation", 2)
	for r: Array in [[2, 0.6], [1, 0.35]]:
		var c := ColorRect.new()
		c.color = Color(INK, r[1])
		c.custom_minimum_size = Vector2(0, r[0])
		pair.add_child(c)
	col.add_child(pair)


func para(s: String, size := 18, color := INK) -> Label:
	return _line(s, Game.font_text, size, color)


func rule(thick := 2, alpha := 0.5) -> void:
	var r := ColorRect.new()
	r.color = Color(INK, alpha)
	r.custom_minimum_size = Vector2(0, thick)
	col.add_child(r)


func gap(h: float) -> void:
	var g := Control.new()
	g.custom_minimum_size = Vector2(0, h)
	col.add_child(g)


## a name on the left, a figure on the right, as in a ledger
func row(label: String, value: String, color := INK, size := 18) -> void:
	var h := HBoxContainer.new()
	var a := RoomKit._label(label, Game.font_text, size, SOFT)
	a.autowrap_mode = TextServer.AUTOWRAP_OFF
	a.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	a.clip_text = true
	var b := RoomKit._label(value, Game.font_text, size, color)
	b.autowrap_mode = TextServer.AUTOWRAP_OFF
	b.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	h.add_child(a)
	h.add_child(b)
	col.add_child(h)


## a name, a measure filled to `part` (0..1), and what it stands at
func bar(label: String, part: float, value: String, color := INK) -> void:
	var h := HBoxContainer.new()
	h.add_theme_constant_override("separation", 10)
	var a := RoomKit._label(label, Game.font_text, 18, SOFT)
	a.autowrap_mode = TextServer.AUTOWRAP_OFF
	a.clip_text = true
	a.custom_minimum_size = Vector2(150, 0)
	a.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	a.size_flags_stretch_ratio = 1.0
	var p := ProgressBar.new()
	p.min_value = 0.0
	p.max_value = 1.0
	p.value = clampf(part, 0.0, 1.0)
	p.show_percentage = false
	p.custom_minimum_size = Vector2(120, 11)
	p.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	p.size_flags_stretch_ratio = 1.2
	p.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	var back := StyleBoxFlat.new()
	back.bg_color = Color(INK, 0.1)
	back.border_color = Color(INK, 0.45)
	back.set_border_width_all(1)
	var fill := StyleBoxFlat.new()
	fill.bg_color = Color(color, 0.72)
	p.add_theme_stylebox_override("background", back)
	p.add_theme_stylebox_override("fill", fill)
	var b := RoomKit._label(value, Game.font_text, 18, color)
	b.autowrap_mode = TextServer.AUTOWRAP_OFF
	b.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	b.custom_minimum_size = Vector2(110, 0)
	h.add_child(a)
	h.add_child(p)
	h.add_child(b)
	col.add_child(h)


## the page divides into columns; print goes in the first until `into` picks another
func columns(n: int, sep := 34) -> Array:
	var h := HBoxContainer.new()
	h.add_theme_constant_override("separation", sep)
	main.add_child(h)
	var out := []
	for i in n:
		var v := VBoxContainer.new()
		v.add_theme_constant_override("separation", 6)
		v.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		h.add_child(v)
		out.append(v)
	col = out[0]
	return out


func into(c: VBoxContainer) -> void:
	col = c


## a rubber stamp in the top right corner of the page, in the margin and clear of the title and its note
func stamp(s: String, color := RED) -> void:
	var l := Label.new()
	l.text = s
	l.add_theme_font_override("font", Game.font_title)
	l.add_theme_font_size_override("font_size", 34)
	l.add_theme_color_override("font_color", Color(color, 0.62))
	var b := StyleBoxFlat.new()
	b.bg_color = Color(0, 0, 0, 0)
	b.border_color = Color(color, 0.62)
	b.set_border_width_all(3)
	b.set_content_margin_all(4)
	b.content_margin_left = 12
	b.content_margin_right = 12
	l.add_theme_stylebox_override("normal", b)
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	over.add_child(l)
	l.rotation = deg_to_rad(-7)
	var place := func():                    # measured when placed: the stamp's font sets its size only once in the tree
		l.size = l.get_combined_minimum_size()
		l.pivot_offset = l.size / 2.0
		l.position = Vector2(over.size.x - l.size.x - 6.0, -4.0 - l.size.y / 2.0)
	over.resized.connect(place)


## a small stamped button on the page: pressing it gives an order
func button(label: String, order: Callable, enabled := true, tip := "") -> Button:
	var b := Button.new()
	b.text = label
	b.disabled = not enabled
	b.tooltip_text = tip
	b.focus_mode = Control.FOCUS_NONE
	b.add_theme_font_override("font", Game.font_text)
	b.add_theme_font_size_override("font_size", 15)
	for k in ["font_color", "font_focus_color"]:
		b.add_theme_color_override(k, Color(INK, 0.86))
	for k in ["font_hover_color", "font_pressed_color"]:        # the ink darkens under the hand
		b.add_theme_color_override(k, Color(0.03, 0.02, 0.01))
	b.add_theme_color_override("font_disabled_color", Color(INK, 0.32))
	for state in ["normal", "hover", "pressed", "disabled"]:
		b.add_theme_stylebox_override(state, slip(state))
	b.pressed.connect(order)
	return b


## a paper slip, cut square from card a shade darker than the page: a thin ink edge and a heavier one below, where it
## stands off the page. Pressed, the slip goes down a pixel; disabled, its ink is faded.
static func slip(state: String) -> StyleBoxFlat:
	var s := StyleBoxFlat.new()
	s.bg_color = {"normal": Color(0.89, 0.85, 0.74), "hover": Color(0.86, 0.81, 0.69), "pressed": Color(0.82, 0.77, 0.65),
			"disabled": Color(0.89, 0.85, 0.74, 0.4)}[state]
	s.border_color = Color(INK, {"normal": 0.5, "hover": 0.85, "pressed": 0.85, "disabled": 0.18}[state])
	s.set_border_width_all(1)
	s.border_width_bottom = 1 if state in ["pressed", "disabled"] else 2
	s.set_content_margin_all(3)
	s.content_margin_left = 9
	s.content_margin_right = 9
	if state == "pressed":
		s.content_margin_top = 4
		s.content_margin_bottom = 2
	return s


## a name, a figure, and buttons after them
func act_row(label: String, value: String, buttons: Array, color := INK, size := 18) -> void:
	var h := HBoxContainer.new()
	h.add_theme_constant_override("separation", 8)
	var a := RoomKit._label(label, Game.font_text, size, color)
	a.autowrap_mode = TextServer.AUTOWRAP_OFF
	a.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	a.clip_text = true
	h.add_child(a)
	if value != "":
		var v := RoomKit._label(value, Game.font_text, size - 2, SOFT)
		v.autowrap_mode = TextServer.AUTOWRAP_OFF
		h.add_child(v)
	for b: Button in buttons:
		h.add_child(b)
	col.add_child(h)


## The part of a long list that is showing, `per` to a leaf. more() puts the button that turns the leaf under it.
static func leaf_of(list: Array, key: String, per: int) -> Array:
	var leaves := maxi(1, ceili(list.size() / float(per)))
	var at: int = int(leaf.get(key, 0)) % leaves
	leaf[key] = at
	return list.slice(at * per, at * per + per)


func more(list: Array, key: String, per: int) -> void:
	var leaves := ceili(list.size() / float(per))
	if leaves <= 1:
		return
	act_row("Leaf %d of %d" % [int(leaf.get(key, 0)) + 1, leaves], "%d in all" % list.size(), [button("Turn over", func():
		Sheet.leaf[key] = int(Sheet.leaf.get(key, 0)) + 1
		Game.touch())], SOFT, 15)


## an order to the simulation that either is carried out or comes back with the reason it cannot be
static func order(why_not: String) -> void:
	if why_not != "":
		Game.said.emit(why_not)
	Game.touch()


static func pct(v: float) -> String:
	return "%d%%" % int(round(v))


# ---- the sheets

static func make(key: String) -> Sheet:
	match key:
		"news": return _news()
		"espionage": return _espionage()
		"diplomacy": return _diplomacy()
		"budget": return _budget()
		"politics": return _politics()
		"society": return _society()
		"focus": return _focus()
		"research": return _research()
		"army": return _army()
		"navy": return _navy()
		"air": return _air()
		"reports": return _reports()
		"works": return _works()
	var s := Sheet.new()
	s.title(key)
	return s


static func _news() -> Sheet:
	var s := Sheet.new(Color(0.93, 0.91, 0.84))
	var sim := Game.sim
	var paper := Game.newspaper()
	s.rule(3, 0.8)
	s.title(paper, 54)
	s.rule(1, 0.8)
	s.note("%s        ·        Morning Edition" % sim.date(), 18)
	s.rule(3, 0.8)
	var items: Array = sim.news(19)
	if items.is_empty():
		s.gap(30)
		s.title("No News of Note", 34)
		s.note("The world is quiet. Our correspondents report nothing that would trouble the government.", 20)
		s.gap(30)
		return s
	s.gap(4)
	s.title(items[0].text, 32)
	s.note(items[0].date, 16)
	s.rule(1, 0.5)
	var cols := s.columns(3, 26)
	for i in range(1, items.size()):
		s.into(cols[(i - 1) % 3])
		var it: Dictionary = items[i]
		s.para(it.text, 18, INK if it.ours else INK.lightened(0.12))
		s._line(it.date, Game.font_text, 14, SOFT)
		s.rule(1, 0.18)
	return s


static func _espionage() -> Sheet:
	var s := Sheet.new(Color(0.9, 0.78, 0.55))
	var d: Dictionary = Game.sim.espionage()
	s.title(d.agency, 34)
	s.note("Most secret  ·  For the head of government only")
	s.rule()
	s.stamp("SECRET")
	var cols := s.columns(2)
	s.heading("Our networks abroad", INK)
	var nets: Array = d.networks
	nets.sort_custom(func(a, b): return a.intel > b.intel)
	var send := func(tag: String, by: int) -> Callable:
		return func(): Sheet.order("" if Game.sim.post_agent(tag, by) else ("No agent is waiting for a posting." if by > 0 else "We have no one there."))
	for n in nets.slice(0, 13):
		s.act_row(n.name, "%d%%  ·  %d there  ·  %s" % [int(n.intel), n.spies, n.knows], [
				s.button("−", send.call(n.tag, -1), n.spies > 0, "Recall an agent"), s.button("+", send.call(n.tag, 1), d.waiting > 0, "Send our best waiting agent")], INK, 16)
	if nets.is_empty():
		s.para("We have no one abroad.")
	s.into(cols[1])
	s.heading("The service", INK)
	s.row("Agents", "%d of %d" % [d.agents, d.max_agents])
	s.row("Waiting for a posting", str(d.waiting))
	s.row("Influence to spend", "%d" % int(d.influence))
	s.heading("Our agents", INK)
	var list: Array = d.list
	for a in list.slice(0, 12):
		s.row(a.name, a.post)
	if list.is_empty():
		s.para("No agent is in our pay. Agents are hired with influence, then posted to a foreign capital or kept at home to hunt spies.")
	var home: String = Game.sim.politics().tag
	s.act_row("Hunting spies at home", "", [s.button("−", send.call(home, -1), true, "Take one off the hunt"), s.button("+", send.call(home, 1), d.waiting > 0, "Set a waiting agent to it")])
	s.heading("Men who offer their services", INK)
	for c in Game.sim.candidates():
		var index: int = c.index
		s.act_row(c.name, "infiltration %.1f  ·  hunting %.1f  ·  cover %.1f" % [c.infiltration, c.counter, c.cover], [s.button("Hire, %d" % int(c.cost), func():
			Sheet.order("" if Game.sim.hire_agent(index) else "We cannot take him on."), c.can, "Paid in influence")], INK, 15)
	return s


static func _diplomacy() -> Sheet:
	var s := Sheet.new(Color(1.0, 0.97, 0.89))
	var sim := Game.sim
	var all: Array = sim.relations()
	s.title("Foreign Correspondence")
	s.note("The courts and governments that matter to us  ·  %d influence to spend" % int(sim.politics().influence))
	s.rule()
	var cols := s.columns(2)
	var wars := all.filter(func(r): return r.war)
	var friends := all.filter(func(r): return not r.war and (r.allied or r.entente or r.guaranteed or r.guarantees_us))
	var rest := all.filter(func(r): return not r.war and not (r.allied or r.entente or r.guaranteed or r.guarantees_us))
	var write := func(r: Dictionary) -> Button:
		return s.button("Write", func():
			Sheet.court = r.tag
			Game.touch(), true, "Write to this government")
	s.heading("At war with")
	if wars.is_empty():
		s.para("No one. The country is at peace.")
	for r in wars.slice(0, 8):
		s.row(r.name, r.faction if r.faction != "" else r.government, BAD)
	s.heading("Bound to us")
	if friends.is_empty():
		s.para("We stand alone: no alliance, no understanding, no guarantee.")
	for r in friends.slice(0, 8):
		var ties := []
		if r.allied: ties.append("ally")
		if r.entente: ties.append("entente")
		if r.guaranteed: ties.append("we guarantee")
		if r.guarantees_us: ties.append("guarantees us")
		s.act_row(r.name, ", ".join(ties), [write.call(r)], GOOD)
	# the letter in hand
	var to: Dictionary = {}
	for r in all:
		if r.tag == Sheet.court:
			to = r
	s.heading("A letter to %s" % to.name if not to.is_empty() else "A letter")
	if to.is_empty():
		s.para("Choose a government with Write, and decide what the letter shall say.", 17, SOFT)
	else:
		s.para("Relations stand at %+d." % int(to.relation), 17, SOFT)
		var send := func(what: String) -> Callable:
			return func(): Sheet.order(sim.diplomacy(what, to.tag))
		var h := HBoxContainer.new()
		h.add_theme_constant_override("separation", 8)
		if to.improve_due > 0.05:
			s.para("Goodwill already on its way: %+.1f." % to.improve_due, 15, GOOD)
		h.add_child(s.button("Improve relations", send.call("improve"), not to.war and sim.politics().influence >= to.improve_cost,
				"%d influence: relations rise by 8 over two months. Each approach to the same government costs 5 more; a year without one brings it down again." % to.improve_cost))
		h.add_child(s.button("Guarantee", send.call("guarantee"), not to.guaranteed and not to.war, "25 influence: we answer for their independence"))
		s.col.add_child(h)
		var h2 := HBoxContainer.new()
		h2.add_theme_constant_override("separation", 8)
		h2.add_child(s.button("Prepare a claim", send.call("justify"), not to.war and not to.allied, "40 influence: in time it gives us cause for war"))
		h2.add_child(s.button("Declare war", send.call("war"), not to.war and not to.allied, "Only with cause, and never lightly"))
		s.col.add_child(h2)
	s.into(cols[1])
	s.heading("The powers and our neighbours")
	for r in Sheet.leaf_of(rest, "courts", 15):
		var v: float = r.relation
		s.act_row(r.name, "%+d" % int(v), [write.call(r)], GOOD if v >= 20 else BAD if v <= -20 else INK)
	s.more(rest, "courts", 15)
	return s


static func _budget() -> Sheet:
	var s := Sheet.new(Color(0.95, 0.94, 0.86))
	var d: Dictionary = Game.sim.budget()
	s.title("The Ledger")
	s.note("Treasury and works of the nation, %s" % Game.sim.date())
	s.rule()
	var cols := s.columns(2)
	s.heading("The treasury")
	s.row("In the treasury", "%d million" % int(d.money), BAD if d.money < 0 else INK)
	s.row("Revenue a day", "%.2f million" % d.income)
	s.row("Railways and forts a day", "%.2f million" % d.upkeep)
	s.heading("Where the revenue goes")
	var k := 0
	for b in d.shares:
		var which := k
		k += 1
		if which == d.shares.size() - 1:        # the reserve is what the others leave
			s.act_row(b.name, pct(b.share * 100.0), [], INK)
			continue
		s.act_row(b.name, pct(b.share * 100.0), [
				s.button("−", func(): Sheet.order("" if Game.sim.shift_budget(which, -0.01) else "Nothing is spent on that."), b.share > 0.004, "A point back to the reserve"),
				s.button("+", func(): Sheet.order("" if Game.sim.shift_budget(which, 0.01) else "The reserve is empty: take from another purpose first."), true, "A point from the reserve")])
	s.into(cols[1])
	s.heading("The works")
	s.row("Civil factories", "%d" % int(d.civil))
	s.row("   making goods for the people", "%d" % int(d.consumer))
	s.row("   working for trade", "%d" % int(d.trade))
	s.row("   free to build", "%d" % int(d.civil_free))
	s.row("Arms works", "%d" % int(d.arms))
	s.row("Works under construction", str(d.building))
	s.heading("Materials, a day")
	for r in Game.sim.market():
		if r.needed < 0.01 and r.made < 0.01:
			continue
		var short: bool = r.made + r.bought - r.sold < r.needed * 0.98
		var offers: Array = []
		var res: int = r.resource
		for seller in (r.sellers as Array).slice(0, 2):
			var tag: String = seller.tag
			offers.append(s.button("Buy 4 from %s" % seller.name, func(): Sheet.order(Game.sim.buy(tag, res, 4.0)), true,
					"They have %.0f to spare. One free civil factory pays for it." % seller.spare))
		s.row(r.name, "%.1f made, %.1f bought, %.1f needed%s" % [r.made, r.bought, r.needed, "  ·  short" if short else ""], BAD if short else INK, 17)
		if not offers.is_empty():
			var h := HFlowContainer.new()
			h.add_theme_constant_override("h_separation", 6)
			for b: Button in offers:
				h.add_child(b)
			s.col.add_child(h)
	var deals: Array = d.deals
	if not deals.is_empty():
		s.heading("Contracts")
		for c in deals.slice(0, 6):
			var index: int = c.index
			s.act_row("%s %s" % ["Buying from" if c.buying else "Selling to", c["with"]], "%.1f %s" % [c.amount, String(c.what).to_lower()],
					[s.button("End", func(): Sheet.order("" if Game.sim.cancel_deal(index) else "That contract is not ours to end."))], INK, 17)
	return s


static func _politics() -> Sheet:
	var s := Sheet.new(Color(0.97, 0.94, 0.85))
	var sim := Game.sim
	var d: Dictionary = sim.politics()
	s.title("%s Government" % d.government)
	var who: String = d.leader
	if d.title != "":
		who = "%s, %s" % [d.leader, d.title]
	s.note("%s%s  ·  %d influence to spend" % [who, "  ·  " + d.party if d.party != "" else "", int(d.influence)], 19)
	s.rule()
	var cols := s.columns(2)
	s.heading("The state of the country")
	s.bar("Stability", d.stability / 100.0, pct(d.stability), BAD if d.stability < 35 else INK)
	s.bar("Support for war", d.war_support / 100.0, pct(d.war_support))
	s.heading("The country's opinion")
	var k := 0
	for p in d.support:
		var ideo := k
		k += 1
		var running: bool = d.campaign == ideo
		s.act_row(p.name + ("  (in power)" if p.rules else ""), pct(p.share), [s.button("Stop" if running else "Campaign", func():
			Sheet.order("" if Game.sim.campaign(-1 if running else ideo) else "A campaign takes 25 influence to open, and support below 60."),
			true, "Speeches, newspapers and meetings: support rises a little every day, for half a point of influence a day")], BAD if p.rules else INK)
	s.heading("Laws")
	for g in sim.laws():
		var now := ""
		var choices: Array = []
		for o in g.options:
			if o.current:
				now = o.name
			elif o.allowed:
				var group: String = g.group
				var option: int = o.option
				choices.append(s.button(o.name, func(): Sheet.order(Game.sim.set_law(group, option)), true, "50 influence"))
		s.act_row(g.name, now, [], INK, 17)
		if not choices.is_empty():
			var h := HFlowContainer.new()
			h.add_theme_constant_override("h_separation", 6)
			for b: Button in choices:
				h.add_child(b)
			s.col.add_child(h)
	s.into(cols[1])
	s.heading("The cabinet")
	var seat := 0
	for m in d.cabinet:
		var which := seat
		seat += 1
		s.act_row(m.post, m.name if m.kind == "" else "%s  ·  %s" % [m.name, m.kind], [s.button("Choosing" if Sheet.post == which else "Change", func():
			Sheet.post = which
			Game.touch(), Sheet.post != which)], INK, 17)
		if Sheet.post == which:
			for c in sim.ministers(which):
				if c.holds:
					continue
				var id: int = c.id
				s.act_row("      " + c.kind, c.name, [s.button("Appoint", func(): Sheet.order(Game.sim.appoint(id)), c.can, c.why if not c.can else c.text)], SOFT, 15)
	var open: Array = sim.decisions()
	if not open.is_empty():
		s.heading("Decisions before the government")
		for x in Sheet.leaf_of(open, "decisions", 9):
			var id: int = x.id
			s.act_row(x.name, "%d infl." % int(x.cost) if x.cost > 0 else "", [s.button("Decide", func():
				Sheet.order("" if Game.sim.take_decision(id) else "The government cannot decide that now."), x.can, x.text)], INK, 17)
		s.more(open, "decisions", 9)
	return s


static func _society() -> Sheet:
	var s := Sheet.new(Color(0.94, 0.92, 0.85))
	var d: Dictionary = Game.sim.society()
	s.title("Census and Social Survey")
	s.note("%.1f million souls, %.1f million of them in the home country" % [d.people, d.core_people])
	s.rule()
	var cols := s.columns(2)
	s.heading("The classes")
	for c in d.classes:
		s.bar(c.name, c.share, pct(c.share * 100.0))
		s.bar("      content", c.mood / 100.0, pct(c.mood), BAD if c.mood < 35 else GOOD if c.mood > 65 else INK)
	s.heading("Bread and means")
	s.bar("Food on the table", d.food, pct(d.food * 100.0), BAD if d.food < 0.95 else INK)
	s.row("Wealth a head, against the great powers", "%.2f" % d.wealth)
	s.row("Men fit to serve", "%.2f million" % (d.manpower / 1000.0) if d.manpower > 1000 else "%d thousand" % int(d.manpower))
	s.into(cols[1])
	s.heading("The condition of the country")
	for v in d.development:
		var rising: bool = v.target > v.value + 0.5
		var falling: bool = v.target < v.value - 0.5
		s.bar(v.name, v.value / 100.0, "%d  %s" % [int(v.value), "rising" if rising else "falling" if falling else "steady"], GOOD if rising else BAD if falling else INK)
	var sp: Array = d.spirits
	if not sp.is_empty():
		s.heading("The mood of the times")
		for x in sp.slice(0, 5):
			s.para(x.name, 18, GOOD if x.good else BAD)
			s.para(x.text, 15, SOFT)
	return s


static func _focus() -> Sheet:
	var s := Sheet.new(Color(0.98, 0.95, 0.86))
	var d: Dictionary = Game.sim.focus()
	s.title("The Government's Programme")
	s.note("%d undertakings carried through so far" % d.finished)
	s.rule()
	var cols := s.columns(2)
	s.heading("In hand")
	if d.has("name"):
		s.para(d.name, 26)
		s.bar("Progress", d.done / maxf(d.days, 1.0), "%d of %d days" % [int(d.done), int(d.days)])
		s.para(d.text, 17, SOFT)
	else:
		s.para("Nothing. The government has set itself no task.", 20, BAD)
	s.heading("Campaigns beside it")
	if d.has("programme"):
		s.bar(d.programme, d.programme_done, pct(d.programme_done * 100.0))
	for p in (Game.sim.programmes() as Array).slice(0, 8):
		if p.running:
			continue
		var id: int = p.id
		s.act_row(p.name, "%d days" % int(p.days), [s.button("Begin", func():
			Sheet.order("" if Game.sim.start_programme(id) else "That campaign cannot begin now."), p.can and not d.has("programme"), p.text)], INK, 17)
	s.into(cols[1])
	s.heading("Open to us")
	var open: Array = d.open
	for f in Sheet.leaf_of(open, "focus", 14):
		var id: int = f.id
		s.act_row(f.name, "%d days" % int(f.days), [s.button("Take up", func():
			Sheet.order("" if Game.sim.start_focus(id) else "The government cannot take that up now."), true, f.text)])
	s.more(open, "focus", 14)
	return s


static func _research() -> Sheet:
	var s := Sheet.new(Color(0.93, 0.92, 0.87))
	var d: Dictionary = Game.sim.research()
	s.title("Proceedings of the Academy")
	s.note("Our science stands at the year %d  ·  %d discoveries known, %d within reach" % [d.era, d.known, d.open])
	s.rule()
	var cols := s.columns(2)
	var k := 0
	var found := false
	for slot in d.slots:
		k += 1
		var mine: bool = slot.slot == Sheet.lab and slot.experimental == Sheet.lab_experimental
		found = found or mine
		s.heading(("Experimental laboratory %d" % (slot.slot + 1)) if slot.experimental else "Laboratory %d" % (slot.slot + 1), INK)
		var number: int = slot.slot
		var kind: bool = slot.experimental
		var choose := s.button("Choosing" if mine else "Choose a subject", func():
			Sheet.lab = number
			Sheet.lab_experimental = kind
			Game.touch(), not mine)
		if slot.has("name"):
			s.act_row(slot.name, "", [choose], INK, 20)
			s.bar("of %d" % slot.year, slot.done / maxf(slot.days, 1.0), "%d of %d days" % [int(slot.done), int(slot.days)])
		else:
			s.act_row("Idle. Nothing is being studied here.", "", [choose], BAD)
	if not found and not (d.slots as Array).is_empty():
		Sheet.lab = d.slots[0].slot
		Sheet.lab_experimental = d.slots[0].experimental
	s.into(cols[1])
	s.heading("Subjects within reach")        # the laboratory on a line of its own: in the heading it wrapped a lone "1"
	s.para("For %s" % ("the experimental laboratory" if Sheet.lab_experimental else "laboratory %d" % (Sheet.lab + 1)), 16, SOFT)
	var open: Array = Game.sim.techs_open().filter(func(t): return t.experimental == Sheet.lab_experimental)
	open.sort_custom(func(a, b): return a.year < b.year if a.year != b.year else a.days < b.days)
	if open.is_empty():
		s.para("Nothing more is within reach of this laboratory.", 18, SOFT)
	var bench := "tech%s%d" % ["x" if Sheet.lab_experimental else "", Sheet.lab]
	for t in Sheet.leaf_of(open, bench, 15):
		var id: int = t.id
		s.act_row(t.name, "%s  ·  %d  ·  %d days" % [t.field, t.year, int(t.days)], [s.button("Study", func():
			Sheet.order("" if Game.sim.start_research(Sheet.lab, id, Sheet.lab_experimental) else "That laboratory cannot take it up."), true, t.text)], INK, 17)
	s.more(open, bench, 15)
	return s


static func _army() -> Sheet:
	var s := Sheet.new(Color(0.96, 0.92, 0.82))
	var d: Dictionary = Game.sim.forces()
	s.title("Order of Battle")
	s.note("%d divisions, %d of them still training  ·  %d thousand men in reserve" % [d.divisions, d.training, int(d.manpower)])
	s.rule()
	s.stamp("WAR OFFICE")
	var cols := s.columns(2)
	s.heading("The armies")
	var armies: Array = d.armies
	if armies.is_empty():
		s.para("No army has been formed.")
	for a in armies.slice(0, 9):
		s.row(a.name, "%d divisions" % a.divisions, INK, 20)
		s.row("      " + a.general, a.skill if a.plan == "" else "%s  ·  %s" % [a.skill, a.plan], SOFT, 15)
	s.into(cols[1])
	s.heading("Under no general")
	var loose: Array = d.reserve
	if loose.is_empty():
		s.para("Every division has its army.")
	for r in loose.slice(0, 7):
		s.row(r.name, str(r.divisions))
	s.heading("Raise a division")
	for t in (Game.sim.templates() as Array).slice(0, 8):
		var id: int = t.id
		s.act_row(t.name, t.kind, [s.button("Raise", func(): Sheet.order(Game.sim.recruit(id)), t.can, t.why if not t.can else "Men are called up and begin training")], INK, 17)
	return s


static func _navy() -> Sheet:
	var s := Sheet.new(Color(0.9, 0.92, 0.9))
	var d: Dictionary = Game.sim.forces()
	s.title("The Fleet")
	s.note("Strength %.1f, in battleships of 1906" % d.navy)
	s.rule()
	s.stamp("ADMIRALTY", Color(0.1, 0.16, 0.4))
	var fleets: Array = d.fleets
	if fleets.is_empty():
		s.gap(20)
		s.para("The nation has no fleet.", 22)
		return s
	var cols := s.columns(2)
	var k := 0
	for f in fleets.slice(0, 6):
		s.into(cols[k % 2])
		k += 1
		s.heading(f.name, Color(0.1, 0.16, 0.4))
		s.row(f.admiral, "%d ships" % f.ships)
		s.row(f.mission, f.at, SOFT, 16)
		var which := k - 1
		var orders := HBoxContainer.new()
		orders.add_theme_constant_override("separation", 6)
		for m in 3:
			var mission := m
			orders.add_child(s.button(["Home waters", "Protect trade", "Blockade"][m], func():
				Sheet.order("" if Game.sim.fleet_mission(which, mission) else "The fleet has that order already."), String(f.mission).to_lower() != ["home waters", "protect trade", "blockade"][m]))
		s.col.add_child(orders)
		for c in (f.classes as Array).slice(0, 7):
			s.row("      %d  %s" % [c.count, c.name], c.kind, INK, 16)
	return s


static func _air() -> Sheet:
	var s := Sheet.new(Color(0.93, 0.93, 0.9))
	var d: Dictionary = Game.sim.forces()
	s.title("The Air Service")
	s.rule()
	var wings: Array = d.wings
	var store: Array = Game.sim.air_designs()
	if wings.is_empty() and store.is_empty():
		s.gap(20)
		s.para("The nation has no flying machines. A few balloons, some enthusiasts, and a file of proposals from inventors.", 22)
		s.para("Wings are formed once the academy has given us aircraft and the works have built them.", 18, SOFT)
		s.gap(20)
		return s
	for w in wings.slice(0, 16):
		s.row("%s  ·  %d %s" % [w.name, w.planes, w.aircraft], "%s, %s" % [w.base, String(w.mission).to_lower()])
	if not store.is_empty():
		s.heading("Machines in store")
		for a in store:
			var design: int = a.design
			s.act_row(a.name, "%d in store" % a.stock, [s.button("Form a wing", func():
				Sheet.order("" if Game.sim.form_wing(design) else "Too few machines for a wing, or no airfield with room for it."), a.can)])
	return s


## The order book of the works: what each line builds and with how many factories, and the designs a line can be
## opened for. The drawings themselves are in the drafting office.
static func _works() -> Sheet:
	var s := Sheet.new(Color(0.93, 0.93, 0.87))
	var sim := Game.sim
	var n: Dictionary = sim.nation()
	var idle: int = n.factories_free
	s.title("The Works Order Book")
	s.note("%d of %d arms works on a line  ·  %d idle" % [n.factories_used, n.factories, idle])
	s.rule()
	var cols := s.columns(2)
	s.heading("On the lines")
	var lines: Array = sim.lines()
	if lines.is_empty():
		s.para("Nothing is being built. Open a line from the list of designs.", 18, BAD)
	var shown := Sheet.leaf_of(lines, "lines", 8)
	var first: int = int(Sheet.leaf.get("lines", 0)) * 8
	for k in shown.size():
		var l: Dictionary = shown[k]
		var index := first + k
		s.act_row(l.name, "%d works" % l.factories, [
				s.button("−", func(): Sheet.order("" if Game.sim.change_factories(index, -1) else "That line has no works to give up."), l.factories > 0, "Take a factory off this line"),
				s.button("+", func(): Sheet.order("" if Game.sim.change_factories(index, 1) else "No factory is idle: take one off another line first."), idle > 0, "Put an idle factory on this line"),
				s.button("Close", func(): Sheet.order("" if Game.sim.close_line(index) else "That line cannot be closed."), true, "End this line: its factories fall idle")], INK, 18)
		s.row("      " + String(l.kind), "%.2f a day  ·  %d in stock  ·  efficiency %d%%" % [l.daily, int(l.stock), int(round(l.efficiency * 100.0))], SOFT, 15)
	s.more(lines, "lines", 8)
	s.into(cols[1])
	s.heading("Designs a line can build")
	if idle <= 0:
		s.para("No factory is idle: take one off a line with − to start another.", 15, SOFT)
	var designs: Array = sim.designs()
	for d in Sheet.leaf_of(designs, "designs", 14):
		var id: int = d.design
		s.act_row(d.name, "cost %.1f  ·  %d in stock" % [d.cost, int(d.stock)], [s.button("Add works" if d.on_line else "Open a line", func():
			Sheet.order("" if Game.sim.open_line(id) else "No factory is idle."), idle > 0, d.kind)], INK, 16)
	s.more(designs, "designs", 14)
	s.into(cols[0])
	var queue: Array = sim.build_queue()
	s.heading("Construction  ·  %d project%s" % [queue.size(), "" if queue.size() == 1 else "s"])
	if queue.is_empty():
		s.para("Nothing is going up. Click one of our provinces on the map to build there.", 15, SOFT)
	var qfirst: int = int(Sheet.leaf.get("queue", 0)) * 8
	var qshown := Sheet.leaf_of(queue, "queue", 8)
	for k in qshown.size():
		var b: Dictionary = qshown[k]
		var index := qfirst + k
		var when := "waiting for idle civil factories" if b.days < 0 else "%d works  ·  about %d days" % [int(b.factories), b.days]
		s.act_row("%s  ·  %s" % [b.name, b.province], "%d%%" % int(b.done * 100.0), [
				s.button("Up", func(): Sheet.order("" if Game.sim.raise_build(index) else "It is first already."), index > 0, "Build this one sooner"),
				s.button("Cancel", func(): Sheet.order("" if Game.sim.cancel_build(index) else "That work is gone."), true, "Stop this work: what was done is lost")], INK if b.days >= 0 else SOFT, 16)
		s.row("      " + when, "", SOFT, 14)
	s.more(queue, "queue", 8)
	return s


static func _reports() -> Sheet:
	var s := Sheet.new(Color(0.98, 0.95, 0.86), Vector2(760, 0))
	s.title("From the Ministries")
	s.rule()
	if Game.unread.is_empty():
		s.gap(12)
		s.para("The tray is empty. Reports land here when the academy finishes a study or the government carries a measure through.", 20)
		s.gap(12)
		return s
	for r in Game.unread.slice(-7):
		s.heading("Carried through" if r.focus else "Discovered")
		s.para(r.name, 24)
		s.note(r.date).horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT
		s.para(r.text, 17, SOFT)
	Game.unread.clear()
	return s
