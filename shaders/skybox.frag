#version 330 core
in  vec3 vDir;
out vec4 FragColor;

uniform samplerCube uEnv;
uniform float       uExposure;

vec3 acesFilm(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) /
                 (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 c = texture(uEnv, normalize(vDir)).rgb * uExposure;
    c = acesFilm(c);
    c = pow(c, vec3(1.0 / 2.2));
    FragColor = vec4(c, 1.0);
}
