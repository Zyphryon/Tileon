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

#if defined(ENABLE_TONEMAP_ACES)

float3 Tonemap(float3 Color)
{
    const float3x3 kInput  = float3x3(0.59719,  0.35458,  0.04823,  0.07600, 0.90834,  0.01566,  0.02840,  0.13383, 0.83777);
    const float3x3 kOutput = float3x3(1.60475, -0.53108, -0.07367, -0.10208, 1.10813, -0.00605, -0.00327, -0.07276, 1.07602);

    Color = mul(kInput, Color);

    const float3 Numerator   = Color * (Color + 0.0245786) - 0.000090537;
    const float3 Denominator = Color * (0.983729 * Color + 0.4329510) + 0.238081;
    return saturate(mul(kOutput, Numerator / Denominator));
}

#else

float3 Tonemap(float3 Color)
{
    const float kMiddle   = 0.22;
    const float kToe      = 1.33;
    const float kShoulder = 0.532;
    const float kDecay    = -1.0 / (1.0 - kShoulder);

    const float3 Toe      = kMiddle * pow(Color / kMiddle, kToe);
    const float3 Shoulder = 1.0 - (1.0 - kShoulder) * exp(kDecay * (Color - kShoulder));

    const float3 Lower = lerp(Toe, Color, smoothstep(0.0, kMiddle, Color));
    return lerp(Lower, Shoulder, step(kShoulder, Color));
}

#endif // ENABLE_TONEMAP_ACES

float4 main(fs_Input Input) : SV_Target
{
    const float3 Albedo   = t_Albedo.Sample(s_Albedo, Input.Texture).rgb;
    const float3 Radiance = t_Radiance.Sample(s_Radiance, Input.Texture).rgb;

    return float4(Tonemap(Albedo * Radiance * u_Exposure.x), 1.0);
}

#endif // FRAGMENT_SHADER
