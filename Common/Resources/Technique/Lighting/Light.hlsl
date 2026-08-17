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
    float4x4 u_CameraInverse;
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

    // A sphere reaches the length of each screen axis' world mix; summing instead bounds the box around it.
    const float2 Ex = mul(u_Camera, float4(1.0, 0.0, 0.0, 0.0)).xy;
    const float2 Ey = mul(u_Camera, float4(0.0, 1.0, 0.0, 0.0)).xy;
    const float2 Ez = mul(u_Camera, float4(0.0, 0.0, 1.0, 0.0)).xy;

    const float2 Spread = float2(length(float3(Ex.x, Ey.x, Ez.x)),
                                 length(float3(Ex.y, Ey.y, Ez.y)));

#if defined(LIGHT_SPOT)
    // A cone barely fills its range sphere, so bound the apex and the disc it opens onto instead.
    const float  Widest = Extent * sqrt(saturate(1.0 - Input.Outer * Input.Outer));
    const float2 Apex   = mul(u_Camera, float4(Center, 1.0)).xy;
    const float2 Mouth  = mul(u_Camera, float4(Center + Input.Params1.xyz * Extent, 1.0)).xy;

    const float2 Lower = min(Apex, Mouth - Widest * Spread);
    const float2 Upper = max(Apex, Mouth + Widest * Spread);

    const float2 Clip = lerp(Lower, Upper, Corner);
#else
    const float4 Middle = mul(u_Camera, float4(Center, 1.0));
    const float2 Clip   = Middle.xy + (Corner * 2.0 - 1.0) * (Extent * Spread);
#endif

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

float3 main(fs_Input Input) : SV_Target0
{
    // Clip position and depth recover the world point through the inverse camera.
    const int3   Texel = int3(Input.Position.xy, 0);
    const float  Depth = t_Depth.Load(Texel).r;
    const float4 Probe = mul(u_CameraInverse, float4(Input.Probe.xy, Depth, 1.0));
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
    // Filtering a normal across an edge blends two surfaces, so read it by texel.
    const float4 Surface    = t_Normal.Load(Texel);
    const float3 Normal     = normalize(Surface.rgb * 2.0 - 1.0);

    // Normal alpha is opacity, reused as thickness so a light behind a surface bleeds through. Nothing
    // else may claim that channel.
    const float  Lambert    = saturate(dot(Normal, Incident));
    const float  Through    = saturate(dot(-Normal, Incident)) * (1.0 - Surface.a);

    // The dots are negatives, so at most one survives the clamp. A sum, not a max, so it still holds if
    // either gains a wrap term.
    const float  NormalDotL = Lambert + Through;
#else
    const float  NormalDotL = 1.0;
#endif

    return Input.Color.rgb * (Attenuation * NormalDotL);
}

#endif // FRAGMENT_SHADER