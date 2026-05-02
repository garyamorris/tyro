#version 330 core

in  vec2 vUV;
in  vec3 vColor;
out vec4 FragColor;

uniform sampler2D uAtlas;

void main() {
    float a = texture(uAtlas, vUV).r;
    if (a < 0.05) discard;
    FragColor = vec4(vColor, a);
}
