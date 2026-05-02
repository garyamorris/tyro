#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3  uCameraPos;
uniform vec3  uAlbedo;
uniform vec3  uEmissive;
uniform float uTime;

out vec4 FragColor;

// Soap-bubble-ish iridescence: Fresnel-driven hue shift + a flowing detail.
vec3 spectrum(float t) {
    // Cheap rainbow palette — clamp to [0,1].
    return 0.5 + 0.5 * cos(6.28318 * (vec3(0.0, 0.33, 0.67) + t));
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    float NdV = max(dot(N, V), 0.0);
    float fres = pow(1.0 - NdV, 3.0);

    // The hue is driven by the dot product so the surface "shifts" with view.
    float t = NdV * 0.7
            + uTime * 0.05
            + sin(vWorldPos.x * 3.0 + vWorldPos.y * 2.0 + vWorldPos.z) * 0.05;

    vec3 inner = uAlbedo * 0.04;
    vec3 outer = spectrum(t);
    vec3 color = mix(inner, outer, fres);

    // Specular sheen on top.
    float sheen = pow(NdV, 80.0) * 0.3;
    color += vec3(sheen);
    color += uEmissive;

    FragColor = vec4(color, 1.0);
}
