#version 330 core
// Standard model->world->clip transform. Forwards world-space position,
// world-space normal (via uNormalMatrix = inverse-transpose of uModel's 3x3),
// and world-space tangent so the fragment shader can build a TBN basis for
// tangent-space normal mapping.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vTangent;
out vec2 vUV;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal   = normalize(uNormalMatrix * aNormal);
    // Direction-only — model matrix's 3x3 is good enough; PBR samples normal
    // map in tangent space and re-projects via TBN.
    vTangent  = normalize(mat3(uModel) * aTangent);
    vUV       = aUV;
    gl_Position = uProj * uView * world;
}
