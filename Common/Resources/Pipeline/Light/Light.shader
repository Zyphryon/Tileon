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
    float2 World     : TEXCOORD0;
	float2 Center    : TEXCOORD1;
    float4 Color     : COLOR0;
    float2 Screen    : TEXCOORD2;
	float2 Extra     : TEXCOORD3;	// radius || range, falloff
	
#if   defined(LIGHT_SPOT) 
    float2 Direction : TEXCOORD4;
    float2 Cone      : TEXCOORD5;
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

ps_Input vertex(vs_Input Input)
{
    ps_Input Result;

    const float2 Corner = TessellateRect(Input.VertexID);
    const float2 World  = Input.Params0.xy + (Corner * 2.0 - 1.0) * Input.Params0.z;

    Result.Position = mul(u_Camera, float4(World, 0.0, 1.0));
    Result.World    = World;
    Result.Center   = Input.Params0.xy;
    Result.Color    = Input.Color;
    Result.Screen   = Result.Position.xy * float2(0.5, -0.5) + 0.5;
    Result.Extra    = Input.Params0.zw;
		
#if   defined(LIGHT_SPOT) 

    Result.Direction = Input.Params1.xy;
    Result.Cone      = Input.Params1.zw;

#endif

	return Result;
}

float4 fragment(ps_Input Input) : SV_Target0
{
	const float2 Relative   = Input.Center - Input.World;
	const float  Distance   = length(Relative);
	const float2 Distance2D = (Distance > 0.0001) ? Relative / Distance : float2(0.0, 0.0);

    float Attenuation = saturate(1.0 - pow(saturate(Distance / Input.Extra.x), Input.Extra.y));

#if   defined(LIGHT_SPOT) 

    const float CosAngle = (Distance > 0.0001) ? dot(-Distance2D, Input.Direction) : 1.0;
	Attenuation	= Attenuation * smoothstep(Input.Cone.y, Input.Cone.x, CosAngle);

#endif

    clip(Attenuation - 0.001);
	
#if   defined(ENABLE_NORMAL_MAPPING)
    const float2 NormalXY   = NormalTexture.Sample(LinearSampler, Input.Screen).rg * 2.0 - 1.0;
    const float  NormalZ    = sqrt(saturate(1.0 - dot(NormalXY, NormalXY)));
    const float3 Normal     = normalize(float3(NormalXY, NormalZ));
	
    const float  NormalDotL = saturate(dot(Normal, normalize(float3(Distance2D, 0.5))));
#else
	const float  NormalDotL = 1.0;
#endif

    return float4(Input.Color.rgb * (Attenuation * NormalDotL), Attenuation);
}