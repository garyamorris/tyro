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
    vec3 c = vec3(r, g, b);
    c = pow(c, vec3(1.0/2.2));
    FragColor = vec4(c, 1.0);
}
