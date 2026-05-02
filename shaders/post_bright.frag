#version 330 core

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
