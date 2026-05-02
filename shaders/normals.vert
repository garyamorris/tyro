#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat3 uNormalMatrix;

out vec3 vNormalWS;
out vec3 vPosWS;

void main() {
    vec4 w = uModel * vec4(aPos, 1.0);
    vPosWS    = w.xyz;
    vNormalWS = normalize(uNormalMatrix * aNormal);
    gl_Position = w; // geometry shader handles projection
}
