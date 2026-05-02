#version 330 core

in vec3 vNormalWS;

uniform vec3 uAlbedo;
uniform vec3 uEmissive;

out vec4 FragColor;

void main() {
    // Cheap "matcap-ish" tint based on world-space normal direction —
    // looks distinct from the lit Phong material so the shader swap is obvious.
    vec3 n = normalize(vNormalWS);
    vec3 tint = 0.5 + 0.5 * n;
    FragColor = vec4(uEmissive + uAlbedo * tint, 1.0);
}
