#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform float     uVignette; // 0 = off

void main() {
    vec3 color = texture(uColor, vUV).rgb;

    // Subtle vignette to prove the post pass is doing something.
    if (uVignette > 0.0) {
        float d = distance(vUV, vec2(0.5));
        color *= mix(1.0, 1.0 - d * 1.2, uVignette);
    }

    // Simple gamma.
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
