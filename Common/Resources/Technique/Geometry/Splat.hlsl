// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

cbuffer cb_Global : register(b0)
{
    float4x4 u_Camera;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Attributes
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct vs_Input
{
    uint   VertexID  : SV_VertexID;

    int2   Origin    : SLOT0;    // region corner, in units, relative to the frame's origin
    uint   Weights   : SLOT1;    // slice of the weight array this region wrote its map into
    uint4  Palette   : SLOT2;    // slice of the terrain array each of the four slots draws
    float4 Mapping   : SLOT3;    // how often each slot repeats across one world unit
    float2 Phase0    : SLOT4;    // where slot 0 already stands in its repeat, at the region's corner
    float2 Phase1    : SLOT5;    // the same, for slot 1
    float2 Phase2    : SLOT6;    // the same, for slot 2
    float2 Phase3    : SLOT7;    // the same, for slot 3
    uint4  Tint      : SLOT8;    // the color each slot's art is multiplied by, packed as RGBA8
};

struct fs_Input
{
    float4                Position : SV_POSITION;
    float2                Ground   : TEXCOORD0;
    nointerpolation uint   Weights    : TEXCOORD1;
    nointerpolation uint4  Palette    : TEXCOORD2;
    nointerpolation float4 Mapping    : TEXCOORD3;
    nointerpolation float2 Phase[4]   : TEXCOORD4;
    nointerpolation uint4  Tint       : TEXCOORD8;
};

float4 UnpackTint(uint Packed)
{
    const uint4 Bytes = uint4(Packed, Packed >> 8u, Packed >> 16u, Packed >> 24u) & 0xFFu;

    return float4(Bytes) * (1.0 / 255.0);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

float2 TessellateRect(uint VertexID)
{
    const float2 kUnitRectCorners[4] =
    {
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
    const float2 Unit   = float2(Input.Origin) + Corner * SPLAT_UNITS_PER_REGION;

    Result.Position   = mul(u_Camera, float4(Unit.x, 0.0, Unit.y, 1.0));
    Result.Ground     = Corner;
    Result.Weights    = Input.Weights;
    Result.Palette    = Input.Palette;
    Result.Mapping    = Input.Mapping;
    Result.Phase[0]   = Input.Phase0;
    Result.Phase[1]   = Input.Phase1;
    Result.Phase[2]   = Input.Phase2;
    Result.Phase[3]   = Input.Phase3;
    Result.Tint       = Input.Tint;

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2DArray t_Weight : register(t0);
SamplerState   s_Weight : register(s0);
Texture2DArray t_Albedo : register(t1);
SamplerState   s_Albedo : register(s1);
#ifdef ENABLE_NORMAL_MAPPING
Texture2DArray t_Normal : register(t2);
SamplerState   s_Normal : register(s2);
#endif

struct fs_Output
{
    float4 Albedo : SV_Target0;
    float4 Normal : SV_Target1;
};

fs_Output main(fs_Input Input)
{
    fs_Output Result;

    const float  Size    = SPLAT_UNITS_PER_REGION + 2.0 * SPLAT_MAP_BORDER;
    const float2 Sampled = (Input.Ground * SPLAT_UNITS_PER_REGION + SPLAT_MAP_BORDER) / Size;

    float4 Weight = t_Weight.Sample(s_Weight, float3(Sampled, Input.Weights));

    clip(dot(Weight, float4(1.0, 1.0, 1.0, 1.0)) - SPLAT_WEIGHT_FLOOR);

    Weight /= max(dot(Weight, float4(1.0, 1.0, 1.0, 1.0)), 0.0001);

    // Every terrain sweeps over the world at its own rate, picked up from where the sweep already stood at
    // this region's corner, so the art runs on across a border instead of starting over at it.
    float2 Texture[4];
    float4 Source[4];

    [unroll]
    for (uint Slot = 0; Slot < 4; ++Slot)
    {
        Texture[Slot] = Input.Phase[Slot] + Input.Ground * SPLAT_UNITS_PER_REGION * Input.Mapping[Slot];

        Source[Slot] = (Weight[Slot] > SPLAT_WEIGHT_FLOOR)
            ? t_Albedo.Sample(s_Albedo, float3(Texture[Slot], Input.Palette[Slot]))
            : float4(0.0, 0.0, 0.0, 0.0);
    }

    float4 Albedo = float4(0.0, 0.0, 0.0, 0.0);
    float3 Normal = float3(0.0, 0.0, 0.0);

    [unroll]
    for (uint Layer = 0; Layer < 4; ++Layer)
    {
        if (Weight[Layer] > SPLAT_WEIGHT_FLOOR)
        {
            Albedo += Source[Layer] * UnpackTint(Input.Tint[Layer]) * Weight[Layer];

#ifdef ENABLE_NORMAL_MAPPING
            // Every slice carries relief, flat where nobody authored any, so the sample needs no guard.
            const float3 Tangent
                = t_Normal.Sample(s_Normal, float3(Texture[Layer], Input.Palette[Layer])).xyz * 2.0 - 1.0;

            Normal += Tangent * Weight[Layer];
#endif
        }
    }

    Result.Albedo = Albedo;

#ifdef ENABLE_NORMAL_MAPPING
    // The ground faces up, so tangent Z is world up and the other two lie along the plane.
    const float3 Surface = normalize(float3(Normal.x, max(Normal.z, 0.0001), Normal.y));
    Result.Normal = float4(Surface * 0.5 + 0.5, 1.0);
#else
    Result.Normal = float4(0.5, 1.0, 0.5, 1.0);
#endif

    return Result;
}

#endif // FRAGMENT_SHADER
