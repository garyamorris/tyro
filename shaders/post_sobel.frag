#version 330 core
// Sobel edge detector.
// Convolves luminance with the 3x3 Sobel kernels (Gx, Gy), takes the gradient
// magnitude sqrt(gx^2 + gy^2), and adds it to a dimmed copy of the source —
// bright outlines on a darkened background. Eight texture taps per pixel.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform vec2      uTexelSize;

float lum(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main() {
    vec2 ts = uTexelSize;
    float tl = lum(texture(uColor, vUV + vec2(-ts.x,  ts.y)).rgb);
    float t  = lum(texture(uColor, vUV + vec2( 0.0,   ts.y)).rgb);
    float tr = lum(texture(uColor, vUV + vec2( ts.x,  ts.y)).rgb);
    float l  = lum(texture(uColor, vUV + vec2(-ts.x,  0.0)).rgb);
    float r  = lum(texture(uColor, vUV + vec2( ts.x,  0.0)).rgb);
    float bl = lum(texture(uColor, vUV + vec2(-ts.x, -ts.y)).rgb);
    float b  = lum(texture(uColor, vUV + vec2( 0.0,  -ts.y)).rgb);
    float br = lum(texture(uColor, vUV + vec2( ts.x, -ts.y)).rgb);

    float gx = -tl - 2.0*l - bl + tr + 2.0*r + br;
    float gy =  tl + 2.0*t  + tr - bl - 2.0*b - br;
    float g  = sqrt(gx*gx + gy*gy);

    vec3 base = texture(uColor, vUV).rgb * 0.25;
    vec3 edge = vec3(g);
    FragColor = vec4(base + edge, 1.0);
}
