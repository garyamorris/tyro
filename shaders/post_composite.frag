#version 330 core
// Bloom step 3 of 3: add the blurred brightpass on top of the scene and
// optionally apply a vignette. Additive bloom gives soft glow around already-
// bright pixels without lifting black levels, which is what makes it look
// like camera/eye behaviour rather than a global brighten.

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
    FragColor = vec4(c, 1.0);
}
