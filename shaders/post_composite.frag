#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float     uBloomStrength;
uniform float     uVignette;

void main() {
    vec3 scene = texture(uScene, vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;
    vec3 c = scene + bloom * uBloomStrength;

    if (uVignette > 0.0) {
        float d = distance(vUV, vec2(0.5));
        c *= mix(1.0, 1.0 - d * 1.2, uVignette);
    }
    c = pow(c, vec3(1.0/2.2));
    FragColor = vec4(c, 1.0);
}
