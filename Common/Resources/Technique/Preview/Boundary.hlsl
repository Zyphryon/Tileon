// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

cbuffer cb_Global : register(b0)
{
    float4x4 u_Camera;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Constants
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

static const float4 kOutlineColor = float4(0.033105, 1.000000, 0.170645, 0.85); // Wireframe, linear for sRGB 20/100/45.

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Attributes
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct vs_Input
{
    uint   VertexID : SV_VertexID;
    float3 Center   : SLOT0;
    float3 Extent   : SLOT1;
};

struct fs_Input
{
    float4 Position : SV_POSITION;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

#ifdef BOUNDARY_FLAT

float2 TessellateRect(uint VertexID)
{
    const float2 kRectEdges[8] = {
        float2(-1, -1), float2( 1, -1),
        float2( 1, -1), float2( 1,  1),
        float2( 1,  1), float2(-1,  1),
        float2(-1,  1), float2(-1, -1)
    };
    return kRectEdges[VertexID];
}

fs_Input main(vs_Input Input)
{
    fs_Input Result;

    const float4 Origin = mul(u_Camera, float4(Input.Center, 1.0));
    const float2 Span   = abs(mul(u_Camera, float4(Input.Extent.x, 0.0, 0.0, 0.0)).xy)
                        + abs(mul(u_Camera, float4(0.0, Input.Extent.y, 0.0, 0.0)).xy)
                        + abs(mul(u_Camera, float4(0.0, 0.0, Input.Extent.z, 0.0)).xy);

    Result.Position = float4(Origin.xy + TessellateRect(Input.VertexID) * Span, Origin.zw);

    return Result;
}

#else

float3 TessellateBox(uint VertexID)
{
    const float3 kBoxEdges[24] = {
        float3(-1, -1, -1), float3( 1, -1, -1),
        float3(-1,  1, -1), float3( 1,  1, -1),
        float3(-1, -1,  1), float3( 1, -1,  1),
        float3(-1,  1,  1), float3( 1,  1,  1),
        float3(-1, -1, -1), float3(-1,  1, -1),
        float3( 1, -1, -1), float3( 1,  1, -1),
        float3(-1, -1,  1), float3(-1,  1,  1),
        float3( 1, -1,  1), float3( 1,  1,  1),
        float3(-1, -1, -1), float3(-1, -1,  1),
        float3( 1, -1, -1), float3( 1, -1,  1),
        float3(-1,  1, -1), float3(-1,  1,  1),
        float3( 1,  1, -1), float3( 1,  1,  1)
    };
    return kBoxEdges[VertexID];
}

fs_Input main(vs_Input Input)
{
    fs_Input Result;

    const float3 World = Input.Center + TessellateBox(Input.VertexID) * Input.Extent;

    Result.Position = mul(u_Camera, float4(World, 1.0));

    return Result;
}

#endif // BOUNDARY_FLAT

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

float4 main(fs_Input Input) : SV_Target0
{
    return kOutlineColor;
}

#endif // FRAGMENT_SHADER