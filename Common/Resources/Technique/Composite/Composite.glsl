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

// Khronos PBR Neutral
vec3 Tonemap(vec3 Color)
{
    const float kStartCompression = 0.8 - 0.04;
    const float kDesaturation     = 0.15;

    // Lift the darkest channel off the floor, which keeps blacks neutral rather than tinting them.
    float Darkest = min(Color.r, min(Color.g, Color.b));
    float Offset  = Darkest < 0.08 ? Darkest - 6.25 * Darkest * Darkest : 0.04;
    Color -= Offset;

    float Peak = max(Color.r, max(Color.g, Color.b));

    if (Peak < kStartCompression)
    {
        return Color;
    }

    // Roll the peak into the range hyperbolically, then bleed the excess toward white.
    float Range   = 1.0 - kStartCompression;
    float NewPeak = 1.0 - Range * Range / (Peak + Range - kStartCompression);
    Color *= NewPeak / Peak;

    float Blend = 1.0 - 1.0 / (kDesaturation * (Peak - NewPeak) + 1.0);
    return mix(Color, vec3(NewPeak, NewPeak, NewPeak), Blend);
}

void main()
{
    vec3 Albedo   = texture(t_Albedo, v_Texture).rgb;
    vec3 Radiance = texture(t_Radiance, v_Texture).rgb;

    out_Color = vec4(Tonemap(Albedo * Radiance * u_Exposure.x), 1.0);
}

#endif // FRAGMENT_SHADER
