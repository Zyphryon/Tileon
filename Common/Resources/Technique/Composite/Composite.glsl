// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

layout(std140, binding = 1) uniform cb_Pass
{
    vec4 u_Exposure;      // X = Exposure, YZW = Unused
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

out vec2 v_Texture;

void main()
{
    v_Texture   = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(v_Texture * 2.0 - 1.0, 0.0, 1.0);
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2D t_Albedo;
uniform sampler2D t_Radiance;

in vec2 v_Texture;

layout(location = 0) out vec4 out_Color;

#if defined(ENABLE_TONEMAP_ACES)

vec3 Tonemap(vec3 Color)
{
    const mat3 kInput  = mat3(0.59719,  0.07600,  0.02840,  0.35458, 0.90834,  0.13383,  0.04823,  0.01566, 0.83777);
    const mat3 kOutput = mat3(1.60475, -0.10208, -0.00327, -0.53108, 1.10813, -0.07276, -0.07367, -0.00605, 1.07602);

    Color = kInput   * Color;

    vec3 Numerator   = Color * (Color + 0.0245786) - 0.000090537;
    vec3 Denominator = Color * (0.983729 * Color + 0.4329510) + 0.238081;
    return clamp(kOutput * (Numerator / Denominator), 0.0, 1.0);
}

#else

vec3 Tonemap(vec3 Color)
{
    const float kMiddle   = 0.22;
    const float kToe      = 1.33;
    const float kShoulder = 0.532;
    const float kDecay    = -1.0 / (1.0 - kShoulder);

    vec3 Toe      = kMiddle * pow(Color / kMiddle, vec3(kToe));
    vec3 Shoulder = 1.0 - (1.0 - kShoulder) * exp(kDecay * (Color - kShoulder));

    vec3 Lower = mix(Toe, Color, smoothstep(0.0, kMiddle, Color));
    return mix(Lower, Shoulder, step(kShoulder, Color));
}

#endif // ENABLE_TONEMAP_ACES

void main()
{
    vec3 Albedo   = texture(t_Albedo, v_Texture).rgb;
    vec3 Radiance = texture(t_Radiance, v_Texture).rgb;

    out_Color = vec4(Tonemap(Albedo * Radiance * u_Exposure.x), 1.0);
}

#endif // FRAGMENT_SHADER
