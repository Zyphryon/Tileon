// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 by Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

cbuffer cb_Global : register(b0)
{
    float4x4 u_Camera;
};

cbuffer cb_Pass : register(b1)
{
    float4x4 u_Inverse;    // turns a clip-space probe back into world space
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Attributes
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct vs_Input
{
    uint   VertexID  : SV_VertexID;
    float4 Params0   : SLOT0;    // center.xyz, radius || range
#if defined(LIGHT_SPOT)
    float4 Params1   : SLOT1;    // direction.xyz, cos(inner)
#endif
    float4 Color     : SLOT2;    // rgb, falloff
#if defined(LIGHT_SPOT)
    float  Outer     : SLOT3;    // cos(outer)
#endif
};

struct fs_Input
{
    float4 Position  : SV_POSITION;
    float4 Probe     : TEXCOORD0;    // clip.xy, screen.xy
    float4 Light     : TEXCOORD1;    // center.xyz, radius || range
    float4 Color     : COLOR0;
#if defined(LIGHT_SPOT)
    float4 Spot      : TEXCOORD2;    // direction.xyz, cos(inner)
    float  Outer     : TEXCOORD3;    // cos(outer)
#endif
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

float2 TessellateRect(uint VertexID)
{
    const float2 kUnitRectCorners[4] = {
        float2(0, 0),
        float2(1, 0),
        float2(0, 1),
        float2(1, 1)
    };
    return kUnitRectCorners[VertexID];
}

fs_Input main(vs_Input Input)
{
    fs_Input Result;

    const float2 Corner = TessellateRect(Input.VertexID);
    const float3 Center = Input.Params0.xyz;
    const float  Extent = Input.Params0.w;

    // The projection is affine, so the sphere's screen half-extent is what each world axis contributes to the
    // screen at that radius. Summing them bounds the sphere by the box around it, conservative by the corners.
    const float4 Middle = mul(u_Camera, float4(Center, 1.0));
    const float2 Radius = abs(mul(u_Camera, float4(Extent, 0.0, 0.0, 0.0)).xy)
                        + abs(mul(u_Camera, float4(0.0, Extent, 0.0, 0.0)).xy)
                        + abs(mul(u_Camera, float4(0.0, 0.0, Extent, 0.0)).xy);

    const float2 Clip = Middle.xy + (Corner * 2.0 - 1.0) * Radius;

    Result.Position = float4(Clip, 0.0, 1.0);
    Result.Probe    = float4(Clip, Clip * float2(0.5, -0.5) + 0.5);
    Result.Light    = Input.Params0;
    Result.Color    = Input.Color;

#if defined(LIGHT_SPOT)
    Result.Spot     = Input.Params1;
    Result.Outer    = Input.Outer;
#endif

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2D    t_Normal : register(t0);
SamplerState s_Normal : register(s0);
Texture2D    t_Depth  : register(t1);
SamplerState s_Depth  : register(s1);

float4 main(fs_Input Input) : SV_Target0
{
    // Depth is position compressed to one channel: the pixel's clip coordinates plus its depth are three
    // knowns, and the inverse of the camera turns them back into the one world point that produced them.
    const float  Depth = t_Depth.Sample(s_Depth, Input.Probe.zw).r;
    const float4 Probe = mul(u_Inverse, float4(Input.Probe.xy, Depth, 1.0));
    const float3 World = Probe.xyz / Probe.w;

    const float3 Relative = Input.Light.xyz - World;
    const float  Distance = length(Relative);
    const float3 Incident = (Distance > 0.0001) ? Relative / Distance : float3(0.0, 1.0, 0.0);

    float Attenuation = saturate(1.0 - pow(saturate(Distance / Input.Light.w), Input.Color.a));

#if defined(LIGHT_SPOT)
    const float CosAngle = dot(-Incident, normalize(Input.Spot.xyz));
    Attenuation *= smoothstep(Input.Outer, Input.Spot.w, CosAngle);
#endif

    clip(Attenuation - 0.001);

#if defined(ENABLE_NORMAL_MAPPING)
    const float3 Normal     = normalize(t_Normal.Sample(s_Normal, Input.Probe.zw).rgb * 2.0 - 1.0);
    const float  NormalDotL = saturate(dot(Normal, Incident));
#else
    const float  NormalDotL = 1.0;
#endif

    return float4(Input.Color.rgb * (Attenuation * NormalDotL), Attenuation);
}

#endif // FRAGMENT_SHADER