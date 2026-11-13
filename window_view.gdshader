shader_type spatial;
render_mode cull_disabled;
// The view through a window of the office: a pale, hazy sky over the capital, two rows of roofs and chimneys in the
// mist, the near one darker, and the street below lost in haze. Only light enough to read as day outside: the room's
// own lights do the rest. Emissive, not unshaded: the bounced light of the room (SDFGI) takes its daylight from it.

uniform float brightness = 0.78;
uniform float seed = 0.0;                  // each window sees its own stretch of roofs
uniform vec3 sky_top : source_color = vec3(0.66, 0.69, 0.68);
uniform vec3 sky_low : source_color = vec3(0.86, 0.84, 0.78);
uniform vec3 far_roofs : source_color = vec3(0.6, 0.6, 0.58);
uniform vec3 near_roofs : source_color = vec3(0.42, 0.41, 0.39);

float hash(float x) { return fract(sin(x * 127.1 + seed * 311.7) * 43758.5453); }

// a roofline: blocks of houses of differing height, pitched or flat, with chimney stacks on some
float roofline(float x, float base, float scale) {
	float cell = floor(x * scale), f = fract(x * scale);
	float h = base + hash(cell) * 0.09;
	float pitch = step(0.5, hash(cell + 7.0)) * (0.5 - abs(f - 0.5)) * 0.08;     // a gable on half the houses
	float stack = step(0.62, hash(cell + 13.0)) * step(abs(f - hash(cell + 3.0) * 0.8 - 0.1), 0.035) * 0.05;
	return h + pitch + stack;
}

void fragment() {
	vec2 uv = vec2(UV.x, 1.0 - UV.y);          // y up
	float x = uv.x + seed * 3.7;
	vec3 c = mix(sky_low, sky_top, smoothstep(0.25, 1.0, uv.y));
	float far = roofline(x, 0.34, 9.0);
	float near = roofline(x * 1.3 + 0.4, 0.2, 5.0);
	c = mix(c, far_roofs, step(uv.y, far) * 0.75);
	c = mix(c, near_roofs, step(uv.y, near) * 0.85);
	c = mix(c, sky_low, (1.0 - smoothstep(0.0, 0.22, uv.y)) * 0.6);   // the street goes into the haze
	ALBEDO = vec3(0.0);
	ROUGHNESS = 1.0;
	SPECULAR = 0.0;
	EMISSION = c * brightness;
}
