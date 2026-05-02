#version 330 core
// Bloom step 1 of 3: extract bright pixels above uThreshold.
// Uses a soft smoothstep instead of a hard cutoff so the brightpass doesn't
// introduce visible banding. The result is then Gaussian-blurred (post_blur)
// and added back over the scene (post_composite).

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform float     uThreshold;

void main() {
    vec3 c = texture(uColor, vUV).rgb;
    float L = max(c.r, max(c.g, c.b));
    float k = smoothstep(uThreshold, uThreshold + 0.4, L);
    FragColor = vec4(c * k, 1.0);
}
