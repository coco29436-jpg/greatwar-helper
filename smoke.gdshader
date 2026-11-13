shader_type spatial;
render_mode unshaded, blend_mix, cull_disabled, depth_draw_never, shadows_disabled;
// Smoke from a cigar left burning. It leaves the ash as one fine thread, widens into a ribbon that folds over on
// itself as it wavers, and breaks into curls that thin away. The ribbon is the space between two edges that wander
// together and cross: bright along the edges, where the sheet is seen end on, faint between them. All of it drifts
// up through slow noise, so no two seconds look alike.
// One upright quad, turned to the eye about its own height.

uniform vec3 tint : source_color = vec3(0.8, 0.82, 0.88);
uniform float strength = 0.5;

void vertex() {
	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(
			vec4(normalize(cross(vec3(0.0, 1.0, 0.0), INV_VIEW_MATRIX[2].xyz)), 0.0),
			vec4(0.0, 1.0, 0.0, 0.0),
			vec4(normalize(cross(INV_VIEW_MATRIX[0].xyz, vec3(0.0, 1.0, 0.0))), 0.0),
			MODEL_MATRIX[3]);
}

float hash(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

float noise(vec2 p) {
	vec2 i = floor(p), f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}

float fbm(vec2 p) {
	return noise(p) * 0.5 + noise(p * 2.03 + 11.7) * 0.3 + noise(p * 4.1 + 3.1) * 0.2;
}

void fragment() {
	float h = 1.0 - UV.y;                       // 0 at the ash, 1 where it is gone
	float x = UV.x - 0.5;
	float t = TIME;
	float rise = h * 3.2 - t * 0.42;            // what was at the ash a moment ago is higher now
	// the middle of the ribbon: straight at first, then it leans and curls
	float sway = pow(h, 1.5);
	float mid = (fbm(vec2(rise, 0.3)) - 0.5) * 0.5 * sway + (fbm(vec2(rise * 2.6, 5.1)) - 0.5) * 0.16 * sway + 0.07 * h * h;
	// its two edges: half the width either side, the width itself swinging through nothing as the sheet turns edge on
	float turn = sin(rise * 2.4 + 3.0 * fbm(vec2(rise * 0.7, 9.0)));
	float half_w = mix(0.004, 0.07, smoothstep(0.05, 0.9, h)) * (0.25 + abs(turn));
	float d = x - mid;
	float fine = mix(0.0035, 0.03, h);          // how soft an edge is
	float edge = smoothstep(fine, 0.0, abs(abs(d) - half_w));
	float sheet = smoothstep(half_w + fine, half_w - fine, abs(d));
	float a = edge * 0.9 + sheet * (0.22 + 0.2 * fbm(vec2(d * 30.0, rise * 3.0)));
	// higher up it tears: holes drift through it and it thins to nothing
	float torn = smoothstep(0.25, 0.75, fbm(vec2(x * 7.0 + 3.0, rise * 1.6)) + 0.55 - h * 0.75);
	a *= mix(1.0, torn, smoothstep(0.3, 0.75, h));
	a *= smoothstep(0.0, 0.03, h) * (1.0 - smoothstep(0.55, 1.0, h)) * mix(1.0, 0.45, h);
	ALBEDO = tint;
	ALPHA = clamp(a * strength, 0.0, 1.0);
}
