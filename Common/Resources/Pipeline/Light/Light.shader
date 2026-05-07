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

cbuffer cb_Global : register(b0)
{
    float4x4 u_Camera;
};

struct vs_Input
{
    uint   VertexID : SV_VertexID;

    float4 Params0  : CUSTOM0;		// center.xy, radius ||	range, falloff
	
#if   defined(LIGHT_SPOT)
    float4 Params1  : CUSTOM1;		// direction.xy, angles.xy
#endif 

    float4 Color    : COLOR0;
};

struct ps_Input
{
    float4 Position  : SV_POSITION;
    float4 World     : TEXCOORD0;   // world.xy, screen.xy
    float4 Light     : TEXCOORD1;   // center.xy, radius || range, falloff
    float4 Color     : COLOR0;

#if   defined(LIGHT_SPOT) 
    float4 Spot      : TEXCOORD2;   // direction.xy, angles.xy
#endif

};

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

float2 TessellateTriangle(uint VertexID)
{
    static const float2 kTriangleCorners[3] = {
        float2(0,  0),
        float2(1,  1),
        float2(1, -1)
    };
    return kTriangleCorners[VertexID];
}

ps_Input vertex(vs_Input Input)
{
    ps_Input Result;

#if defined(LIGHT_SPOT)

    const float2 Corner = TessellateTriangle(Input.VertexID);
	
#else

    const float2 Corner = TessellateRect(Input.VertexID);
	
#endif

#if defined(LIGHT_SPOT)

    const float Spread = Input.Params0.z * sqrt(1.0 - Input.Params1.w * Input.Params1.w) / Input.Params1.w;

    const float2 Axis  = Input.Params1.xy * (Corner.x * Input.Params0.z);
    const float2 Side  = float2(-Input.Params1.y, Input.Params1.x) * (Corner.y * 2.0 - 1.0) * (Spread * Corner.x);
    const float2 World = Input.Params0.xy + Axis + Side;

#else

    const float2 World = Input.Params0.xy + (Corner * 2.0 - 1.0) * Input.Params0.z;

#endif

    Result.Position = mul(u_Camera, float4(World, 0.0, 1.0));
    Result.World    = float4(World, Result.Position.xy * float2(0.5, -0.5) + 0.5);
    Result.Light    = Input.Params0;
    Result.Color    = Input.Color;
		
#if   defined(LIGHT_SPOT) 

    Result.Spot     = Input.Params1;

#endif

    return Result;
}

float4 fragment(ps_Input Input) : SV_Target0
{
    const float2 Relative   = Input.Light.xy - Input.World.xy;
    const float  Distance   = length(Relative);
    const float2 Distance2D = (Distance > 0.0001) ? Relative / Distance : float2(0.0, 0.0);

    float Attenuation = saturate(1.0 - pow(saturate(Distance / Input.Light.z), Input.Light.w));

#if   defined(LIGHT_SPOT) 

    const float CosAngle = (Distance > 0.0001) ? dot(-Distance2D, Input.Spot.xy) : 1.0;
    Attenuation	= Attenuation * smoothstep(Input.Spot.w, Input.Spot.z, CosAngle);

#endif

    clip(Attenuation - 0.001);

#if   defined(ENABLE_NORMAL_MAPPING)
    const float3 Normal     = NormalTexture.Sample(LinearSampler, Input.World.zw).rgb * 2.0 - 1.0;
    const float  NormalDotL = saturate(dot(Normal, normalize(float3(Distance2D, -0.5))));
#else
    const float  NormalDotL = 1.0;
#endif

    return float4(Input.Color.rgb * (Attenuation * NormalDotL), Attenuation);
}