// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Resources

Texture2D    GeometryTexture : register(t0);
Texture2D    RadianceTexture : register(t1);
SamplerState SamplerLinear   : register(s0);

// Attributes

struct ps_Input
{
    float4 Position : SV_POSITION;
    float2 Texture  : TEXCOORD0;
};

// VS Main

ps_Input vertex(uint ID : SV_VertexID)
{
    ps_Input Result;

    Result.Texture  = float2((ID << 1) & 2, ID & 2);
    Result.Position = float4(Result.Texture * 2.0 - 1.0, 0.0, 1.0);

    Result.Texture.y = 1.0 - Result.Texture.y;

    return Result;
}

// PS Main

float4 fragment(ps_Input Input) : SV_Target
{
    const float3 Albedo   = GeometryTexture.Sample(SamplerLinear, Input.Texture).rgb;
    const float3 Radiance = RadianceTexture.Sample(SamplerLinear, Input.Texture).rgb;
    return float4(Albedo * Radiance, 1.0);
}