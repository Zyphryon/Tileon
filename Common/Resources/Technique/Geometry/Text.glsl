// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

layout(std140, binding = 0) uniform cb_Global
{
    mat4 u_Camera;
};

layout(std140, binding = 2) uniform cb_Material
{
    vec2 u_Range;
};

struct PackedFontParameters
{
    vec4  u_Transform0;
    vec4  u_Transform1;
    vec4  u_Transform2;

    uint  u_OutsetTint;
    float u_OutsetOffset;
    float u_OutsetWidth;
    float u_OutsetBias;
    float u_OutsetBlur;
    float u_InsetRoundness;
    float u_InsetThreshold;
};

layout(std140, binding = 3) uniform cb_Instance
{
    PackedFontParameters u_Parameters[128];
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

in vec4  a_Frame;   // normalized atlas edges: minimum.xy, maximum.xy
in ivec2 a_Offset;  // corner within the text layout, in subpixel steps
in uvec2 a_Size;    // extent, in subpixel steps
in float a_Effect;  // the interned effect slot, reinterpreted as a float
in vec4  a_Color;

out vec4 v_Color;
out vec2 v_Texture;
flat out uint v_Effect;

vec2 TessellateRect(int VertexID)
{
    const vec2 kUnitRectCorners[4] = vec2[4](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0)
    );

    return kUnitRectCorners[VertexID];
}

void main()
{
    vec2 Corner = TessellateRect(gl_VertexID);

    PackedFontParameters Run = u_Parameters[floatBitsToUint(a_Effect)];

    vec2 Plane    = (vec2(a_Offset) + Corner * vec2(a_Size)) * GLYPH_SUBPIXEL;
    vec3 Local    = vec3(Plane, 0.0);
    vec3 Position = vec3(
        dot(Local, Run.u_Transform0.xyz) + Run.u_Transform0.w,
        dot(Local, Run.u_Transform1.xyz) + Run.u_Transform1.w,
        dot(Local, Run.u_Transform2.xyz) + Run.u_Transform2.w
    );

    gl_Position = u_Camera * vec4(Position, 1.0);

    v_Texture = mix(a_Frame.xy, a_Frame.zw, Corner);
    v_Color   = a_Color;
    v_Effect  = floatBitsToUint(a_Effect);
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2D t_Albedo;

in vec4 v_Color;
in vec2 v_Texture;
flat in uint v_Effect;

layout(location = 0) out vec4 out_Color;

// Returns the median of the three MSDF channels, i.e. the reconstructed signed distance.
float Median(vec3 Color)
{
    return max(min(Color.r, Color.g), min(max(Color.r, Color.g), Color.b));
}

// Returns the screen-space scale that maps an atlas distance onto pixel coverage.
float Spread(vec2 Coordinates, vec2 Unit)
{
    return max(dot(Unit, vec2(1.0) / fwidth(Coordinates)) * 0.5, 1.0);
}

void main()
{
    PackedFontParameters Font = u_Parameters[v_Effect];

    vec4  Sample       = texture(t_Albedo, v_Texture);
    float DistanceSDF  = Sample.a;
    float DistanceMSDF = Median(Sample.rgb);

    // Convert atlas distance into normalized screen-space range.
    float Scale = Spread(v_Texture, u_Range);

    // Interpolated distance field depending on rounded vs sharp style.
    float StrokeDistance = mix(DistanceMSDF, DistanceSDF, Font.u_InsetRoundness);
    float StrokeBase     = StrokeDistance - Font.u_InsetThreshold;

    // Convert distance to alpha coverage.
    float InnerStrokeA = Scale * StrokeBase + 0.5 + Font.u_OutsetOffset;
    float OuterStrokeA = Scale * (StrokeBase + Font.u_OutsetWidth) + 0.5 + Font.u_OutsetOffset + Font.u_OutsetBias;

    vec4 InnerColor    = v_Color;
    vec4 OuterColor    = unpackUnorm4x8(Font.u_OutsetTint);
    float InnerOpacity = clamp(InnerStrokeA, 0.0, 1.0);
    float OuterOpacity = clamp(OuterStrokeA, 0.0, 1.0);

    // Optional: soften the outset edge.
    float BlurStart  = Font.u_OutsetWidth + Font.u_OutsetBias / Scale;
    float BlurEnd    = BlurStart * (1.0 - Font.u_OutsetBlur);
    float BlurDist   = Font.u_InsetThreshold - DistanceSDF - Font.u_OutsetOffset / Scale;
    float BlurFactor = mix(1.0, 1.0 - smoothstep(BlurEnd, BlurStart, BlurDist), step(0.0001, Font.u_OutsetBlur));

    out_Color = InnerColor * InnerOpacity + (OuterColor * BlurFactpr) * (OuterOpacity - InnerOpacity);
}

#endif // FRAGMENT_SHADER