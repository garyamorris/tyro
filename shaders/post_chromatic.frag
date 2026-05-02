#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform float     uAmount;

void main() {
    vec2 dir = vUV - vec2(0.5);
    float r = texture(uColor, vUV - dir * uAmount).r;
    float g = texture(uColor, vUV).g;
    float b = texture(uColor, vUV + dir * uAmount).b;
    FragColor = vec4(r, g, b, 1.0);
}
