#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

out vec3 vBary;

void main() {
    const vec3 b[3] = vec3[3](vec3(1.0, 0.0, 0.0),
                              vec3(0.0, 1.0, 0.0),
                              vec3(0.0, 0.0, 1.0));
    for (int i = 0; i < 3; ++i) {
        gl_Position = gl_in[i].gl_Position;
        vBary = b[i];
        EmitVertex();
    }
    EndPrimitive();
}
