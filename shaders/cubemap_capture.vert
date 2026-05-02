#version 330 core
// Shared by every cubemap-bake pass: render a unit cube and pass the local
// position to the fragment shader. The fragment uses it as the sample
// direction (normalised) into either the equirect source or the cubemap.
layout(location = 0) in vec3 aPos;

uniform mat4 uView;
uniform mat4 uProj;

out vec3 vDir;

void main() {
    vDir = aPos;
    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
