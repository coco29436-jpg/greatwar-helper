class_name Print
extends SubViewport
## A printed page: drawn once off screen and kept as a texture with mipmaps, so type and fine lines stay clean on a
## sheet lying at a slant across the desk. The painter is a function that draws on the page; press() prints it again
## when what it says has changed.

const INK := Color(0.13, 0.09, 0.05)
const RED := Color(0.5, 0.07, 0.05)

class Page:
	extends Control
	var painter: Callable
	func _draw() -> void:
		for c in get_children():
			c.queue_free()
		painter.call(self)

var page: Page
var mat: BaseMaterial3D
var said := "\n"                # what the page said when it was last printed


## `host` keeps the press in the tree; the print lands in `material`'s albedo
static func make(host: Node, px: Vector2i, painter: Callable, material: BaseMaterial3D) -> Print:
	var p := Print.new()
	p.size = px
	p.transparent_bg = true
	p.disable_3d = true
	p.render_target_update_mode = SubViewport.UPDATE_DISABLED
	p.mat = material
	p.page = Page.new()
	p.page.size = Vector2(px)
	p.page.painter = painter
	p.add_child(p.page)
	host.add_child(p)
	p.press("")
	return p


## print again if the page now says something else
func press(says: String) -> void:
	if says == said:
		return
	said = says
	page.queue_redraw()
	render_target_update_mode = SubViewport.UPDATE_ONCE
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var img := get_texture().get_image()
	img.generate_mipmaps()
	mat.albedo_texture = ImageTexture.create_from_image(img)


# ---- what pages are made of

## the sheet itself: the torn-edged paper, in a tint
static func paper(c: Control, tint: Color) -> void:
	c.draw_texture_rect(load("res://assets/room/paper.png"), Rect2(Vector2.ZERO, c.size), false, tint)


## lines of type too small to read: words as short strokes, ragged on the right
static func greek(c: Control, box: Rect2, line: float, seed_n: int, color := Color(0.13, 0.09, 0.05, 0.6), weight := 0.42) -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_n
	var y := box.position.y
	while y + line * weight <= box.end.y:
		var x := box.position.x
		var last := y + line * 2.0 > box.end.y
		var stop := box.end.x - (box.size.x * rng.randf_range(0.2, 0.6) if last else rng.randf_range(0.0, line * 0.8))
		while x < stop:
			var w := minf(rng.randf_range(line * 0.5, line * 2.6), stop - x)
			c.draw_rect(Rect2(x, y, w, line * weight), color)
			x += w + line * 0.42
		y += line


static func text(c: Control, s: String, at: Vector2, px: int, color := INK, font: Font = null, width := -1.0,
		align := HORIZONTAL_ALIGNMENT_LEFT) -> void:
	c.draw_string(font if font else Game.font_title, at, s, align, width, px, color)


## text that wraps inside `width`; returns the height it took
static func block(c: Control, s: String, at: Vector2, width: float, px: int, color := INK, font: Font = null,
		align := HORIZONTAL_ALIGNMENT_CENTER, lines := 3) -> float:
	var f: Font = font if font else Game.font_title
	c.draw_multiline_string(f, at + Vector2(0, f.get_ascent(px)), s, align, width, px, lines, color)
	return minf(f.get_multiline_string_size(s, align, width, px, lines).y, f.get_height(px) * lines)


## a drawing in white line on black, laid on the page so that only its lines show
static func tracing(c: Control, tex: Texture2D, box: Rect2, strength := 1.0) -> void:
	var t := TextureRect.new()
	t.texture = tex
	t.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	t.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	t.position = box.position
	t.size = box.size
	t.modulate = Color(strength, strength, strength)
	var m := CanvasItemMaterial.new()
	m.blend_mode = CanvasItemMaterial.BLEND_MODE_ADD
	t.material = m
	c.add_child(t)


## a rubber stamp: a word in a box, askew
static func stamp(c: Control, s: String, at: Vector2, px: int, turn: float, color: Color) -> void:
	var f := Game.font_title
	var w := f.get_string_size(s, HORIZONTAL_ALIGNMENT_LEFT, -1, px)
	c.draw_set_transform(at, turn, Vector2.ONE)
	c.draw_rect(Rect2(-w.x / 2 - px * 0.3, -w.y / 2 - px * 0.1, w.x + px * 0.6, w.y + px * 0.2), color, false, px * 0.09)
	c.draw_string(f, Vector2(-w.x / 2, f.get_ascent(px) - w.y / 2), s, HORIZONTAL_ALIGNMENT_LEFT, -1, px, color)
	c.draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
