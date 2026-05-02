#version 330 core
// Render a unit cube. Strip the translation from the view matrix so the cube
// is anchored to the camera. Push the depth to 1.0 in the fragment so the box
// always passes a GL_LEQUAL depth test against any cleared scene depth.
layout(location = 0) in vec3 aPos;

uniform mat4 uView;
uniform mat4 uProj;

out vec3 vDir;

void main() {
    vDir = aPos;
    mat4 viewNoTrans = mat4(mat3(uView));
    vec4 clip = uProj * viewNoTrans * vec4(aPos, 1.0);
    // Force z to w so the resulting depth (z/w) is exactly 1.0 — at the far
    // plane. Use GL_LEQUAL so the skybox is drawn after the scene.
    gl_Position = clip.xyww;
}
