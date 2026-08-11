// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

cbuffer cb_Pass : register(b1)
{
    float4 u_SunColor;      // RGB = Color * Intensity, A = Sun Direction X
    float4 u_SkyColor;      // RGB = Color * Intensity, A = Sun Direction Y
    float4 u_GroundColor;   // RGB = Color * Intensity, A = Sun Direction Z
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

    Result.Texture  = float2((VertexID << 1) & 2, VertexID & 2);
    Result.Position = float4(Result.Texture * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2D    t_Normal : register(t0);
SamplerState s_Normal : register(s0);

float4 main(fs_Input Input) : SV_Target0
{
    const float3 Normal = normalize(t_Normal.Sample(s_Normal, Input.Texture).rgb * 2.0 - 1.0);

    // Hemisphere ambient
    const float  Weight  = Normal.y * 0.5 + 0.5;
    const float3 Ambient = lerp(u_GroundColor.rgb, u_SkyColor.rgb, Weight);

    // Directional
    const float3 SunDir = float3(u_SunColor.w, u_SkyColor.w, u_GroundColor.w);
    const float3 Sun    = u_SunColor.rgb * saturate(dot(Normal, SunDir * rsqrt(dot(SunDir, SunDir))));

    return float4((Ambient + Sun) * 0.5, 1.0);
}

#endif // FRAGMENT_SHADER