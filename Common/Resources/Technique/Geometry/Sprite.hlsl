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
};

struct fs_Input
{
    float4 Position   : SV_POSITION;
    float2 Texture    : TEXCOORD0;
    float4 Color      : COLOR0;
#ifdef ENABLE_NORMAL_MAPPING
    float3 AxisX      : TEXCOORD1;
    float3 AxisY      : TEXCOORD2;
    float3 AxisZ      : TEXCOORD3;
#endif
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
    float3 Local  = float3(Corner * Input.Size, 0.0);

    float3 Position = float3(
        dot(Local, Input.Transform0.xyz) + Input.Transform0.w,
        dot(Local, Input.Transform1.xyz) + Input.Transform1.w,
        dot(Local, Input.Transform2.xyz) + Input.Transform2.w);

    // A sprite stands upright, so the projection already resolves its depth from the world position the
    // affine put it at; it carries no bias of its own.
    Result.Position = mul(u_Camera, float4(Position, 1.0));
    Result.Texture  = lerp(Input.Frame.xy, Input.Frame.zw, float2(Corner.x, 1.0 - Corner.y));
    Result.Color    = Input.Color;

#ifdef ENABLE_NORMAL_MAPPING
    Result.AxisX = normalize(float3(Input.Transform0.x, Input.Transform1.x, Input.Transform2.x));
    Result.AxisY = normalize(float3(Input.Transform0.y, Input.Transform1.y, Input.Transform2.y));
    Result.AxisZ = normalize(float3(Input.Transform0.z, Input.Transform1.z, Input.Transform2.z));
#endif

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

#ifdef ENABLE_NORMAL_MAPPING
    float4 Sampled = t_Normal.Sample(s_Normal, Input.Texture);
    float3 Tangent = normalize(Sampled.rgb * 2.0 - 1.0);
    float3 Normal  = normalize(Tangent.x * Input.AxisX + Tangent.y * Input.AxisY - Tangent.z * Input.AxisZ);

    // The sprite plane faces +Z, so the tangent-space Z carries over to world space unchanged.
#if defined(ENABLE_TRANSLUCENCY)
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

    Result.Normal = float4(0.5, 0.5, 0.0, Opacity);
#endif

    return Result;
}

#endif // FRAGMENT_SHADER