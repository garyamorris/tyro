#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in  vec3 vWorldPos[];
in  vec3 vWorldNormal[];

uniform mat4  uViewProj;
uniform float uTime;
uniform float uMagnitude;

out vec3 gWorldPos;
out vec3 gFaceNormal;

void main() {
    // Face normal recovered from world-space positions of the triangle.
    vec3 e0 = vWorldPos[1] - vWorldPos[0];
    vec3 e1 = vWorldPos[2] - vWorldPos[0];
    vec3 fn = normalize(cross(e0, e1));

    // Smooth-pulse displacement: 0..uMagnitude over time.
    float push = (sin(uTime * 1.4) * 0.5 + 0.5) * uMagnitude;

    for (int i = 0; i < 3; ++i) {
        vec3 p = vWorldPos[i] + fn * push;
        gWorldPos   = p;
        gFaceNormal = fn;
        gl_Position = uViewProj * vec4(p, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}
