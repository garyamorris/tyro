#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform float uTime;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

float waveHeight(vec2 p, float t) {
    float h = 0.0;
    h += sin(dot(p, vec2( 1.0,  0.6)) * 1.2 + t * 1.6) * 0.10;
    h += sin(dot(p, vec2(-0.5,  0.8)) * 1.7 + t * 1.2) * 0.07;
    h += sin(dot(p, vec2( 0.7, -1.0)) * 2.5 + t * 0.8) * 0.05;
    h += sin(dot(p, vec2(-1.0, -0.2)) * 3.5 + t * 2.0) * 0.03;
    return h;
}

void main() {
    vec3 p = aPos;
    vec2 xz = p.xz;
    float t = uTime;

    float h = waveHeight(xz, t);
    p.y += h;

    // Recompute the surface normal from finite differences of the wave field.
    float eps = 0.05;
    float hx = waveHeight(xz + vec2(eps, 0.0), t);
    float hz = waveHeight(xz + vec2(0.0, eps), t);
    vec3 n = normalize(vec3(h - hx, eps, h - hz));

    vec4 world = uModel * vec4(p, 1.0);
    vWorldPos = world.xyz;
    vNormal   = mat3(uModel) * n;
    vUV       = aUV;
    gl_Position = uProj * uView * world;
}
