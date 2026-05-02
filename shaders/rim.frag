#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3  uCameraPos;
uniform vec3  uAlbedo;
uniform vec3  uEmissive;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 4.0);
    vec3 base = uAlbedo * 0.05;
    vec3 rim  = mix(vec3(0.0), uEmissive + uAlbedo, fresnel);
    FragColor = vec4(base + rim, 1.0);
}
