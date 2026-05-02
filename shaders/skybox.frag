#version 330 core
in  vec3 vDir;
out vec4 FragColor;

uniform samplerCube uEnv;
uniform float       uExposure;

void main() {
    // Output linear HDR; the post-FX chain's final pass tonemaps + gammas.
    vec3 c = texture(uEnv, normalize(vDir)).rgb * uExposure;
    FragColor = vec4(c, 1.0);
}
