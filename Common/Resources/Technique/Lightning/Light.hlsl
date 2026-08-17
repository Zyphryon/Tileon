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

    // The projection is affine, so each screen axis is a fixed combination of the three world axes. The
    // furthest a unit sphere reaches along one is the length of that combination, not the sum of its parts:
    // summing bounds the box the sphere sits in, which is wider by half again on an isometric camera.
    const float2 Ex = mul(u_Camera, float4(1.0, 0.0, 0.0, 0.0)).xy;
    const float2 Ey = mul(u_Camera, float4(0.0, 1.0, 0.0, 0.0)).xy;
    const float2 Ez = mul(u_Camera, float4(0.0, 0.0, 1.0, 0.0)).xy;

    const float2 Spread = float2(length(float3(Ex.x, Ey.x, Ez.x)),
                                 length(float3(Ex.y, Ey.y, Ez.y)));

#if defined(LIGHT_SPOT)
    // A cone fits inside its range's sphere but barely fills it. Its hull is the apex together with the disc
    // it opens onto, so bounding those two and boxing the pair is far tighter wherever the beam is narrow.
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
    // Depth is position compressed to one channel: the pixel's clip coordinates plus its depth are three
    // knowns, and the inverse of the camera turns them back into the one world point that produced them.
    const int3   Texel = int3(Input.Position.xy, 0);
    const float  Depth = t_Depth.Load(Texel).r;
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
    // Filtering a normal across an edge blends two unrelated surfaces, so it is read by texel as well.
    const float4 Surface    = t_Normal.Load(Texel);
    const float3 Normal     = normalize(Surface.rgb * 2.0 - 1.0);

    // The alpha of the normal buffer is opacity, which doubles as thickness: the more of it a surface
    // lets through, the more a light behind it shows. That is what gives foliage its backlight, and it
    // is why nothing else may claim that channel.
    const float  Lambert    = saturate(dot(Normal, Incident));
    const float  Through    = saturate(dot(-Normal, Incident)) * (1.0 - Surface.a);

    // The two dots are exact negatives, so at most one of them survives saturation and the sum is the
    // one that did. Written as a sum rather than a maximum because it stays correct if either ever
    // gains a wrap term and stops being mutually exclusive.
    const float  NormalDotL = Lambert + Through;
#else
    const float  NormalDotL = 1.0;
#endif

    // Three channels, because that is what the radiance target has. The accumulation blend is a plain
    // additive on colour alone, so the contribution is the whole of what this pass has to say.
    return Input.Color.rgb * (Attenuation * NormalDotL);
}

#endif // FRAGMENT_SHADER