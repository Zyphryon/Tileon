// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#if defined(PREVIEW_DEPTH)
cbuffer cb_Global : register(b0)
{
    float4x4 u_Camera;
    float4x4 u_CameraInverse;
};
#endif

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Attributes
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct fs_Input
{
    float4 Position : SV_POSITION;
    float2 Texture  : TEXCOORD0;
    float2 Probe    : TEXCOORD1;    // the pixel's clip-space coordinates
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
    Result.Probe     = Result.Position.xy;
    Result.Texture.y = 1.0 - Result.Texture.y;

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2D    t_Source : register(t0);
SamplerState s_Source : register(s0);

float3 sRGBEncode(float3 Value)
{
    return lerp(Value / 12.92, pow((Value + 0.055) / 1.055, 2.4), step(0.04045, Value));
}

float4 main(fs_Input Input) : SV_Target0
{
#if defined(PREVIEW_NORMAL)

    const float3 Normal = t_Source.Sample(s_Source, Input.Texture).rgb;
    return float4(sRGBEncode(Normal), 1.0);

#elif defined(PREVIEW_DEPTH)

    const float  Depth = t_Source.Sample(s_Source, Input.Texture).r;
    const float4 Probe = mul(u_CameraInverse, float4(Input.Probe, Depth, 1.0));
    const float3 World = Probe.xyz / Probe.w;

    const float Elevation = saturate(World.y / ELEVATION_SCALE);

    float3 Color = lerp(float3(0.12, 0.12, 0.12), float3(1.0, 1.0, 1.0), Elevation);

    Color = lerp(Color, float3(0.15, 0.45, 1.00), step(Depth, PREVIEW_MIDGROUND));
    Color = lerp(Color, float3(1.00, 0.20, 0.15), step(PREVIEW_BACKGROUND, Depth));

    return float4(sRGBEncode(Color * step(Depth, 0.99999)), 1.0);

#else

    return float4(t_Source.Sample(s_Source, Input.Texture).rgb, 1.0);

#endif
}

#endif // FRAGMENT_SHADER