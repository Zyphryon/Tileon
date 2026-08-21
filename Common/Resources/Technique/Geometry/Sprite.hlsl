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
    uint   VertexID   : SV_VertexID;

    float4 Transform0 : SLOT0;
    float4 Transform1 : SLOT1;
    float4 Transform2 : SLOT2;

    float4 Frame      : SLOT3;
    float2 Size       : SLOT4;
    float4 Color      : SLOT5;
    uint   Facing     : SLOT6;
};

struct fs_Input
{
    float4 Position   : SV_POSITION;
    float2 Texture    : TEXCOORD0;
    float4 Color      : COLOR0;
#ifdef ENABLE_NORMAL_MAPPING
    float3 AxisX      : TEXCOORD1;
    float3 AxisY      : TEXCOORD2;
#endif
    float3 AxisZ      : TEXCOORD3;   // the face the art turns towards, pointing out of it
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

    // The columns of the affine are the local axes the art is laid down along.
    float3 ColumnX = float3(Input.Transform0.x, Input.Transform1.x, Input.Transform2.x);
    float3 ColumnY = float3(Input.Transform0.y, Input.Transform1.y, Input.Transform2.y);
    float3 ColumnZ = float3(Input.Transform0.z, Input.Transform1.z, Input.Transform2.z);
    float3 Origin  = float3(Input.Transform0.w, Input.Transform1.w, Input.Transform2.w);

    // Two of the three axes span the quad; the one left over is the face it turns towards. An upright
    // sprite turns back down -z at the viewer, whereas art laid against a surface turns away from it.
    uint   Plane  = (Input.Facing >> FACING_PLANE_SHIFT) & FACING_PLANE_MASK;

    float3 AxisU  = (Plane == PLANE_WALL)    ? ColumnY : ColumnX;
    float3 AxisV  = (Plane == PLANE_UPRIGHT) ? ColumnY : ColumnZ;
    float3 Facing = (Plane == PLANE_GROUND)  ? ColumnY : (Plane == PLANE_WALL) ? ColumnX : -ColumnZ;

    float3 Position = Origin + Corner.x * Input.Size.x * AxisU + Corner.y * Input.Size.y * AxisV;

    Result.Position = mul(u_Camera, float4(Position, 1.0));

    // Art laid flat against a surface is coplanar with it, and would z-fight it without a nudge forward.
    if ((Input.Facing & FACING_COPLANAR) != 0u)
    {
        Result.Position.z -= COPLANAR_DEPTH_BIAS;
    }

    // The art is laid down mirrored or turned by exchanging the corner before it picks a point in the frame.
    float2 Sample = float2(Corner.x, 1.0 - Corner.y);

    if ((Input.Facing & FACING_MIRROR_X) != 0u)
    {
        Sample.x = 1.0 - Sample.x;
    }
    if ((Input.Facing & FACING_MIRROR_Y) != 0u)
    {
        Sample.y = 1.0 - Sample.y;
    }

    Result.Texture  = lerp(Input.Frame.xy, Input.Frame.zw, Sample);
    Result.Color    = Input.Color;

#ifdef ENABLE_NORMAL_MAPPING
    Result.AxisX = normalize(AxisU);
    Result.AxisY = normalize(AxisV);
#endif

    Result.AxisZ = normalize(Facing);

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

Texture2D    t_Albedo : register(t0);
SamplerState s_Albedo : register(s0);

#ifdef ENABLE_NORMAL_MAPPING
Texture2D    t_Normal : register(t1);
SamplerState s_Normal : register(s1);
#endif

fs_Output main(fs_Input Input)
{
    fs_Output Result;

    float4 Texel = t_Albedo.Sample(s_Albedo, Input.Texture);

#ifdef ENABLE_ALPHA_TEST
    clip(Texel.a - 0.5);
#endif

    Result.Albedo = Input.Color * Texel;

#ifdef ENABLE_ALPHA_TEST
    Result.Albedo.a = 1.0;
#endif

#ifdef ENABLE_NORMAL_MAPPING
    float4 Sampled = t_Normal.Sample(s_Normal, Input.Texture);
    float3 Tangent = normalize(Sampled.rgb * 2.0 - 1.0);
    float3 Normal  = normalize(Tangent.x * Input.AxisX + Tangent.y * Input.AxisY + Tangent.z * Input.AxisZ);

    #if   defined(ENABLE_TRANSLUCENCY)
        const float Opacity = Sampled.a;
    #elif defined(ENABLE_ALPHA_TEST)
        const float Opacity = 1.0;
    #else
        const float Opacity = Result.Albedo.a;
    #endif

    Result.Normal = float4(Normal * 0.5 + 0.5, Opacity);
#else

    #if defined(ENABLE_ALPHA_TEST)
        const float Opacity = 1.0;
    #else
        const float Opacity = Result.Albedo.a;
    #endif

    Result.Normal = float4(Input.AxisZ * 0.5 + 0.5, Opacity);
#endif

    return Result;
}

#endif // FRAGMENT_SHADER