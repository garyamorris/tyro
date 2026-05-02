#version 330 core
// Convolve the env cubemap with a cosine-weighted hemisphere kernel to bake
// the diffuse irradiance map. Sampled at low res (32^2/face) since the result
// is very smooth — the convolution itself acts as a heavy low-pass filter.
// Uniform sampling in (theta, phi) is cheap and good enough for diffuse;
// importance sampling is reserved for the specular prefilter pass.
in  vec3 vDir;
out vec4 FragColor;

uniform samplerCube uEnv;

const float PI = 3.14159265358979323846;

void main() {
    vec3 N = normalize(vDir);
    // Build an arbitrary tangent frame around N.
    vec3 up    = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = cross(N, right);

    vec3  irradiance = vec3(0.0);
    float nrSamples  = 0.0;
    const float dPhi   = 2.0 * PI / 90.0; // 90 azimuthal samples
    const float dTheta = 0.5  * PI / 30.0; // 30 polar samples

    for (float phi = 0.0; phi < 2.0 * PI; phi += dPhi) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += dTheta) {
            // Tangent-space sample then transform to world.
            vec3 tan = vec3(sin(theta) * cos(phi),
                            sin(theta) * sin(phi),
                            cos(theta));
            vec3 dir = tan.x * right + tan.y * up + tan.z * N;
            irradiance += texture(uEnv, dir).rgb * cos(theta) * sin(theta);
            nrSamples  += 1.0;
        }
    }
    irradiance = PI * irradiance / nrSamples;
    FragColor = vec4(irradiance, 1.0);
}
