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
    uint   VertexID : SV_VertexID;

    int2   Position : SLOT0;  // ground origin, in whole tiles relative to the camera
    uint4  Metrics  : SLOT1;  // size.xy in whole tiles, layer, reserved
    uint4  Lattice  : SLOT2;  // motif period.xy in whole tiles, phase.xy of the run within it
    uint   Atlas    : SLOT3;  // slice of the array texture
    float4 Color    : SLOT4;
};

struct fs_Input
{
    float4               Position : SV_POSITION;
    float2               Texture  : TEXCOORD0;
    float4               Color    : COLOR0;
    nointerpolation uint Slice    : TEXCOORD1;
};

struct fs_Output
{
    float4 Albedo : SV_Target0;
    float4 Normal : SV_Target1;
};

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

    float2 Corner = TessellateRect(Input.VertexID);

    // A tile lies flat on the ground rather than standing upright, so its quad spans world X and world Z
    // and carries no elevation of its own.
    float2 Ground = float2(Input.Position) + Corner * float2(Input.Metrics.xy);

    // Tile layers stack on the same ground position, so only this bias separates them.
    Result.Position    = mul(u_Camera, float4(Ground.x, 0.0, Ground.y, 1.0));
    Result.Position.z -= float(Input.Metrics.z) * TILE_LAYER_BIAS;

    // The art lands on a lattice the ground defines, so the run carries only its phase within the period.
    // V runs against world Y, which keeps the art upright while the ground climbs the screen.
    const float2 Period = float2(Input.Lattice.xy);
    const float2 Phase  = float2(Input.Lattice.zw);
    const float2 Size   = float2(Input.Metrics.xy);

    Result.Texture = float2((Phase.x + Corner.x * Size.x) / Period.x, (Period.y - Phase.y - Corner.y * Size.y) / Period.y);
    Result.Color   = Input.Color;
    Result.Slice   = Input.Atlas & 0xFFFFu;

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2DArray t_Albedo : register(t0);
SamplerState   s_Albedo : register(s0);

fs_Output main(fs_Input Input)
{
    fs_Output Result;

    float4 Texel = t_Albedo.Sample(s_Albedo, float3(Input.Texture, Input.Slice));

#ifdef ENABLE_ALPHA_TEST
    clip(Texel.a - 0.5);
#endif

    Result.Albedo = Input.Color * Texel;

    // A tile lies flat on the ground, so its normal is the world's up axis rather than the viewer's.
    // The ground faces the sky and passes no light through, so it is solid in the opacity channel.
    Result.Normal = float4(0.5, 1.0, 0.5, 1.0);

    return Result;
}

#endif // FRAGMENT_SHADER