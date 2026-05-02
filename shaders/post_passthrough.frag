#version 330 core

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
    c = pow(c, vec3(1.0 / 2.2));
    FragColor = vec4(c, 1.0);
}
