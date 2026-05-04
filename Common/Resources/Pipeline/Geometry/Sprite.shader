// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Resources

Texture2D    ColorTexture  : register(t0);
Texture2D    NormalTexture : register(t1);
SamplerState LinearSampler : register(s0);

// Parameters

cbuffer cb_Global : register(b0)
{
    float4x4 u_Camera;
};

// Attributes

struct vs_Input
{
    uint     VertexID   : SV_VertexID;

    float4   Transform0 : CUSTOM0;
    float4   Transform1 : CUSTOM1;
    float4   Frame      : TEXCOORD0;
    float2   Size       : CUSTOM2;
    float4   Color      : COLOR0;
};

struct ps_Input
{
    float4 Position  : SV_POSITION;
    float2 Texture   : TEXCOORD0;
    float4 Color     : COLOR0;

#ifdef    ENABLE_NORMAL_MAPPING
    float4 Direction : TEXCOORD1;
#endif // ENABLE_NORMAL_MAPPING
};

struct ps_Output
{
    float4 Albedo : SV_Target0;
    float4 Normal : SV_Target1;
};

// VS Main

float2 TessellateRect(uint VertexID)
{
    static const float2 kUnitRectCorners[4] = {
        float2(0, 0),
        float2(1, 0),
        float2(0, 1),
        float2(1, 1)
    };
    return kUnitRectCorners[VertexID];
}

ps_Input vertex(vs_Input Input)
{
    ps_Input Result;

    const float2 Corner   = TessellateRect(Input.VertexID);
    const float2 Local    = Corner * Input.Size;
    const float2 Position = float2(
        dot(Local, Input.Transform0.xy) + Input.Transform0.z,
        dot(Local, Input.Transform1.xy) + Input.Transform1.z
    );

    Result.Position   = mul(u_Camera, float4(Position, Input.Transform0.w, 1.0));
    Result.Texture    = lerp(Input.Frame.xy, Input.Frame.zw, float2(Corner.x, 1.0 - Corner.y));
    Result.Color      = Input.Color;

#ifdef    ENABLE_NORMAL_MAPPING
    Result.Direction  = float4(normalize(Input.Transform0.xy), normalize(Input.Transform1.xy));
#endif // ENABLE_NORMAL_MAPPING

    return Result;
}

// PS Main

ps_Output fragment(ps_Input Input)
{
    ps_Output Result;

    float4 Texel = ColorTexture.Sample(LinearSampler, Input.Texture);

#ifdef    ENABLE_ALPHA_TEST
    clip(Texel.a - 0.001);
#endif // ENABLE_ALPHA_TEST

    Result.Albedo = Input.Color * Texel;

#ifdef    ENABLE_NORMAL_MAPPING
    const float3 NormalTangent = normalize(NormalTexture.Sample(LinearSampler, Input.Texture).rgb * 2.0 - 1.0);
    const float2 NormalWorldXY = NormalTangent.x * Input.Direction.xy + NormalTangent.y * Input.Direction.zw;
    Result.Normal = float4(NormalWorldXY * 0.5 + 0.5, 0.0, Result.Albedo.a);
#else
    Result.Normal = float4(0.5, 0.5, 0.0, Result.Albedo.a);
#endif // ENABLE_NORMAL_MAPPING

    return Result;
}