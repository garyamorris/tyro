#version 330 core
// Sample an equirectangular HDR image by direction and write to one face of a
// cubemap (the destination face is selected by the caller via the framebuffer
// attachment).
in  vec3 vDir;
out vec4 FragColor;

uniform sampler2D uEquirect;

const vec2 kInvAtan = vec2(0.1591, 0.3183); // 1/(2*PI), 1/PI

vec2 sampleSpherical(vec3 v) {
    return vec2(atan(v.z, v.x), asin(v.y)) * kInvAtan + 0.5;
}

void main() {
    vec3  d  = normalize(vDir);
    vec2  uv = sampleSpherical(d);
    vec3  c  = texture(uEquirect, uv).rgb;
    FragColor = vec4(c, 1.0);
}
