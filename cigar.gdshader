shader_type spatial;
render_mode cull_disabled;
// A cigar's wrapper leaf, or (ash > 0.5) the ash it has grown. UV.x runs round it, UV.y along it in metres (the
// lathe in room_kit.gd writes them). The leaf is wound on in a spiral: its edge shows as a fine dark line, its veins
// as paler streaks along the wind. The ash is flaky grey in rings, with the fire showing through its cracks near the
// burning end.

uniform float ash = 0.0;
uniform float fire_at = 0.06;       // UV.y of the burning end
uniform vec3 leaf : source_color = vec3(0.3, 0.17, 0.09);

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
	return noise(p) * 0.5 + noise(p * 2.1 + 7.3) * 0.3 + noise(p * 4.3 + 1.9) * 0.2;
}

void fragment() {
	// round the cigar the pattern must close on itself: two turns of noise blended across the seam
	float u = UV.x;
	vec2 p = vec2(u * 9.0, UV.y * 260.0);
	vec2 q = vec2((u - 1.0) * 9.0, UV.y * 260.0);
	float n = mix(fbm(p), fbm(q), smoothstep(0.85, 1.0, u));
	if (ash < 0.5) {
		float wind = fract(u + UV.y * 26.0);                          // the spiral of the wrapper
		float seam = smoothstep(0.035, 0.0, abs(wind - 0.5));
		float vein = smoothstep(0.75, 1.0, noise(vec2(wind * 14.0, UV.y * 90.0 + u * 3.0)));
		vec3 c = leaf * (0.72 + 0.6 * n);
		c = mix(c, leaf * 1.5, vein * 0.35);
		c = mix(c, leaf * 0.45, seam * 0.7);
		ALBEDO = c;
		ROUGHNESS = 0.62 + 0.2 * n;                                    // an oily leaf has a low sheen
		SPECULAR = 0.35;
		NORMAL_MAP = vec3(0.5 + (vein - seam) * 0.2, 0.5 + (n - 0.5) * 0.3, 1.0);
		NORMAL_MAP_DEPTH = 0.6;
	} else {
		float ring = noise(vec2(u * 3.0, UV.y * 700.0));
		float flake = noise(vec2(u * 40.0, UV.y * 1500.0));
		vec3 c = mix(vec3(0.34, 0.33, 0.32), vec3(0.78, 0.77, 0.75), ring * 0.6 + flake * 0.4);
		float near = smoothstep(fire_at + 0.012, fire_at, UV.y);        // how close to the fire
		float crack = smoothstep(0.62, 0.7, noise(vec2(u * 26.0, UV.y * 900.0))) * near;
		float breath = 0.75 + 0.25 * sin(TIME * 1.3) + 0.1 * sin(TIME * 7.1 + u * 20.0);
		ALBEDO = mix(c, vec3(0.08, 0.03, 0.02), crack);
		EMISSION = vec3(1.0, 0.3, 0.05) * crack * 3.0 * breath;
		ROUGHNESS = 1.0;
		SPECULAR = 0.1;
	}
}
