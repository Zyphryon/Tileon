// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Resources

Texture2D    NormalTexture : register(t0);
SamplerState LinearSampler : register(s0);

// Parameters

cbuffer cb_Effect : register(b0)
{
    float4 u_SunColor;      // RGB = Color * Intensity, A = Sun Direction X
	float4 u_SkyColor;		// RGB = Color * Intensity, A = Sun Direction Y
	float4 u_GroundColor;	// RGB = Color * Intensity, A = Unused
};

struct ps_Input
{
    float4 Position : SV_POSITION;
	float2 Texture  : TEXCOORD0;
};

ps_Input vertex(uint ID : SV_VertexID)
{
    ps_Input Result;

    Result.Texture  = float2((ID << 1) & 2, ID & 2);
    Result.Position = float4(Result.Texture * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    return Result;
}

float4 fragment(ps_Input Input) : SV_Target0
{
    const float2 NormalXY = NormalTexture.Sample(LinearSampler, Input.Texture).rg * 2.0 - 1.0;
    const float  NormalZ  = sqrt(saturate(1.0 - dot(NormalXY, NormalXY)));
    const float3 Normal   = normalize(float3(NormalXY, NormalZ));

    // Hemisphere ambient
    const float  Weight  = Normal.y * 0.5 + 0.5;
    const float3 Ambient = lerp(u_GroundColor.rgb, u_SkyColor.rgb, Weight);

    // Directional
	const float3 SunDir = float3(u_SunColor.w, u_SkyColor.w, 0.5);
	const float3 Sun    = u_SunColor.rgb * saturate(dot(Normal, SunDir * rsqrt(dot(SunDir, SunDir))));

    return float4(Ambient + Sun, 1.0);
}