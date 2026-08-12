// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

cbuffer cb_Pass : register(b1)
{
    float4 u_Exposure;      // X = Exposure, YZW = Unused
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Attributes
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct fs_Input
{
    float4 Position : SV_POSITION;
    float2 Texture  : TEXCOORD0;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

fs_Input main(uint VertexID : SV_VertexID)
{
    fs_Input Result;

    Result.Texture   = float2((VertexID << 1) & 2, VertexID & 2);

    Result.Position  = float4(Result.Texture * 2.0 - 1.0, 0.0, 1.0);
    Result.Texture.y = 1.0 - Result.Texture.y;

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2D    t_Albedo   : register(t0);
SamplerState s_Albedo   : register(s0);

Texture2D    t_Radiance : register(t1);
SamplerState s_Radiance : register(s1);

// Khronos PBR Neutral
float3 Tonemap(float3 Color)
{
    const float kStartCompression = 0.8 - 0.04;
    const float kDesaturation     = 0.15;

    // Lift the darkest channel off the floor, which keeps blacks neutral rather than tinting them.
    const float Darkest = min(Color.r, min(Color.g, Color.b));
    const float Offset  = Darkest < 0.08 ? Darkest - 6.25 * Darkest * Darkest : 0.04;
    Color -= Offset;

    const float Peak = max(Color.r, max(Color.g, Color.b));

    if (Peak < kStartCompression)
    {
        return Color;
    }

    // Roll the peak into the range hyperbolically, then bleed the excess toward white.
    const float Range   = 1.0 - kStartCompression;
    const float NewPeak = 1.0 - Range * Range / (Peak + Range - kStartCompression);
    Color *= NewPeak / Peak;

    const float Blend = 1.0 - 1.0 / (kDesaturation * (Peak - NewPeak) + 1.0);
    return lerp(Color, float3(NewPeak, NewPeak, NewPeak), Blend);
}

float4 main(fs_Input Input) : SV_Target
{
    const float3 Albedo   = t_Albedo.Sample(s_Albedo, Input.Texture).rgb;
    const float3 Radiance = t_Radiance.Sample(s_Radiance, Input.Texture).rgb;

    return float4(Tonemap(Albedo * Radiance * u_Exposure.x), 1.0);
}

#endif // FRAGMENT_SHADER
