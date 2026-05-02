#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3  uCameraPos;
uniform vec3  uAlbedo;
uniform vec3  uEmissive;
uniform float uTime;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    float fres = pow(1.0 - max(dot(N, V), 0.0), 2.0);

    // Horizontal scanlines that drift downward over time.
    float scan = 0.5 + 0.5 * sin(vWorldPos.y * 60.0 - uTime * 6.0);
    scan = pow(scan, 4.0);

    // Vertical glitch — occasional bright bands.
    float glitch = step(0.97, sin(uTime * 12.0 + vWorldPos.y * 7.0));

    vec3 base = uAlbedo * fres;
    vec3 color = base + vec3(scan) * uAlbedo * 0.5
                + vec3(glitch) * uAlbedo * 0.8
                + uEmissive;

    // Slight cyan tint for the classic hologram look.
    color = mix(color, color * vec3(0.4, 1.0, 1.2), 0.5);

    // Edge glow.
    color += uAlbedo * fres * 1.5;

    FragColor = vec4(color, 0.95);
}
