// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

cbuffer cb_Global   : register(b0)
{
    float4x4 u_Camera;
};

cbuffer cb_Material : register(b2)
{
    float2   u_Range;
};

cbuffer cb_Instance : register(b3)
{
    struct PackedFontParameters
    {
        float4 u_Transform0;
        float4 u_Transform1;
        float4 u_Transform2;

        uint   u_OutsetTint;
        float  u_OutsetOffset;
        float  u_OutsetWidth;
        float  u_OutsetBias;
        float  u_OutsetBlur;
        float  u_InsetRoundness;
        float  u_InsetThreshold;
    };
    PackedFontParameters u_Parameters[128];
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Attributes
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct vs_Input
{
    uint     VertexID   : SV_VertexID;

    float4   Frame      : SLOT0;  // normalized atlas edges: minimum.xy, maximum.xy
    int2     Offset     : SLOT1;  // corner within the text layout, in subpixel steps
    uint2    Size       : SLOT2;  // extent, in subpixel steps
    float    Effect     : SLOT3;  // the interned run slot, reinterpreted as a float
    float4   Color      : SLOT4;
};

struct fs_Input
{
    float4               Position : SV_POSITION;
    float2               Texture  : TEXCOORD0;
    float4               Color    : COLOR0;
    nointerpolation uint Effect   : TEXCOORD1;
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
    const float2 Corner = TessellateRect(Input.VertexID);

    const PackedFontParameters Run = u_Parameters[asuint(Input.Effect)];

    const float2 Plane    = (float2(Input.Offset) + Corner * float2(Input.Size)) * GLYPH_SUBPIXEL;
    const float3 Local    = float3(Plane, 0.0);
    const float3 Position = float3(
        dot(Local, Run.u_Transform0.xyz) + Run.u_Transform0.w,
        dot(Local, Run.u_Transform1.xyz) + Run.u_Transform1.w,
        dot(Local, Run.u_Transform2.xyz) + Run.u_Transform2.w
    );

    fs_Input Result;

    Result.Position = mul(u_Camera, float4(Position, 1.0));
    Result.Texture  = lerp(Input.Frame.xy, Input.Frame.zw, Corner);
    Result.Color    = Input.Color;
    Result.Effect   = asuint(Input.Effect);

    return Result;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

float4 UnpackTint(uint Value)
{
    return float4(uint4(Value, Value >> 8, Value >> 16, Value >> 24) & 0xFF) * (1.0 / 255.0);
}

Texture2D    t_Albedo : register(t0);
SamplerState s_Albedo : register(s0);

// Returns the median of the three MSDF channels, i.e. the reconstructed signed distance.
float Median(float3 Color)
{
    return max(min(Color.r, Color.g), min(max(Color.r, Color.g), Color.b));
}

// Returns the screen-space scale that maps an atlas distance onto pixel coverage.
float Spread(float2 Coordinates, float2 Unit)
{
    return max(dot(Unit, 1.0 / fwidth(Coordinates)) * 0.5, 1.0);
}

float4 main(fs_Input Input) : SV_Target
{
    const PackedFontParameters Font = u_Parameters[Input.Effect];

    const float4 Sample       = t_Albedo.Sample(s_Albedo, Input.Texture);
    const float  DistanceSDF  = Sample.a;
    const float  DistanceMSDF = Median(Sample.rgb);

    // Convert atlas distance into normalized screen-space range.
    const float Scale = Spread(Input.Texture, u_Range);

    // Interpolated distance field depending on rounded vs sharp style.
    const float StrokeDistance = lerp(DistanceMSDF, DistanceSDF, Font.u_InsetRoundness);
    const float StrokeBase     = StrokeDistance - Font.u_InsetThreshold;

    // Convert distance to alpha coverage.
    const float InnerStrokeA = Scale * (StrokeBase) + 0.5 + Font.u_OutsetOffset;
    const float OuterStrokeA = Scale * (StrokeBase + Font.u_OutsetWidth) + 0.5 + Font.u_OutsetOffset + Font.u_OutsetBias;

    float4 InnerColor  = Input.Color;
    float4 OuterColor  = UnpackTint(Font.u_OutsetTint);
    float InnerOpacity = clamp(InnerStrokeA, 0.0, 1.0);
    float OuterOpacity = clamp(OuterStrokeA, 0.0, 1.0);

    // Optional: soften the outset edge.
    const float BlurStart  = Font.u_OutsetWidth + Font.u_OutsetBias / Scale;
    const float BlurEnd    = BlurStart * (1.0 - Font.u_OutsetBlur);
    const float BlurDist   = Font.u_InsetThreshold - DistanceSDF - Font.u_OutsetOffset / Scale;
    const float BlurFactor = lerp(1.0, 1.0 - smoothstep(BlurEnd, BlurStart, BlurDist), step(0.0001, Font.u_OutsetBlur));

    float4 Result = (InnerColor * InnerOpacity) + ((OuterColor * BlurFactor) * (OuterOpacity - InnerOpacity));
    return Result;
}

#endif // FRAGMENT_SHADER