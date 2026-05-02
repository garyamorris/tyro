#version 330 core
// "None" entry in the post-FX cycle. Copies uColor to the output, with an
// optional vignette so even the no-effect path can carry one.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform float     uVignette;

void main() {
    vec3 c = texture(uColor, vUV).rgb;
    if (uVignette > 0.0) {
        float d = distance(vUV, vec2(0.5));
        c *= mix(1.0, 1.0 - d * 1.2, uVignette);
    }
    FragColor = vec4(c, 1.0);
}
