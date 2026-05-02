#version 330 core

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in  vec3 vNormalWS[];
in  vec3 vPosWS[];

uniform mat4  uViewProj;
uniform float uLength;

void main() {
    for (int i = 0; i < 3; ++i) {
        vec3 p = vPosWS[i];
        vec3 n = normalize(vNormalWS[i]);
        gl_Position = uViewProj * vec4(p, 1.0);
        EmitVertex();
        gl_Position = uViewProj * vec4(p + n * uLength, 1.0);
        EmitVertex();
        EndPrimitive();
    }
}
