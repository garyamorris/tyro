#version 330 core
// Final post-process pass: tonemap linear HDR → LDR, then encode sRGB.
// Always runs as the last step in the chain; everything upstream operates
// on linear HDR values.
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform float     uExposure;
uniform float     uVignette;

// Knarkowicz fit of the ACES filmic curve.
vec3 acesFilm(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) /
                 (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 c = texture(uColor, vUV).rgb * uExposure;
    if (uVignette > 0.0) {
        float d = distance(vUV, vec2(0.5));
        c *= mix(1.0, 1.0 - d * 1.2, uVignette);
    }
    c = acesFilm(c);
    c = pow(c, vec3(1.0 / 2.2));
    FragColor = vec4(c, 1.0);
}
