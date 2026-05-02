#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform vec2      uDirection; // (texelSize, 0) horizontal or (0, texelSize) vertical

void main() {
    // 9-tap separable Gaussian.
    float w[5];
    w[0] = 0.227027;
    w[1] = 0.1945946;
    w[2] = 0.1216216;
    w[3] = 0.054054;
    w[4] = 0.016216;

    vec3 c = texture(uColor, vUV).rgb * w[0];
    for (int i = 1; i < 5; ++i) {
        vec2 off = uDirection * float(i);
        c += texture(uColor, vUV + off).rgb * w[i];
        c += texture(uColor, vUV - off).rgb * w[i];
    }
    FragColor = vec4(c, 1.0);
}
