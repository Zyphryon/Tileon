// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

cbuffer cb_Pass : register(b1)
{
    float4 u_SunColor;      // RGB = Color * Intensity * Headroom, A = Sun Direction X
    float4 u_SkyColor;      // RGB = Color * Intensity * Headroom, A = Sun Direction Y
    float4 u_GroundColor;   // RGB = Color * Intensity * Headroom, A = Sun Direction Z
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Attributes
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct fs_Input
{
    float4 Position : SV_POSITION;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

fs_Input main(uint VertexID : SV_VertexID)
{
    fs_Input Result;

    const float2 Corner = float2((VertexID << 1) & 2, VertexID & 2);
    Result.Position = float4(Corner * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2D    t_Normal : register(t0);
SamplerState s_Normal : register(s0);

float3 main(fs_Input Input) : SV_Target0
{
    const float4 Surface = t_Normal.Load(int3(Input.Position.xy, 0));
    const float3 Normal  = normalize(Surface.rgb * 2.0 - 1.0);

    // Hemisphere ambient. The weight is world Y, so this reads as "facing the sky" only because the normal
    // buffer stores world-space normals with Y up.
    const float  Weight  = Normal.y * 0.5 + 0.5;
    const float3 Ambient = lerp(u_GroundColor.rgb, u_SkyColor.rgb, Weight);
    const float3 Toward  = float3(u_SunColor.w, u_SkyColor.w, u_GroundColor.w);

    // The alpha of the normal buffer is opacity, which doubles as thickness: the more of it a surface lets
    // through, the more the sun behind it shows.
    const float  Facing = saturate(dot(Normal, Toward)) + saturate(dot(-Normal, Toward)) * (1.0 - Surface.a);
    const float3 Sun    = u_SunColor.rgb * Facing;

    // Two independent contributions, so they sum, at whatever intensity they were authored with. Three
    // channels, because that is what the radiance target has, and it is a float format with no ceiling to
    // protect: the composite's tone curve is what brings the range back down.
    return Ambient + Sun;
}

#endif // FRAGMENT_SHADER