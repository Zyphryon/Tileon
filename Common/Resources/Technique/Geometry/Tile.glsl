// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

layout(std140, binding = 0) uniform cb_Global
{
    mat4 u_Camera;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

in ivec2 a_Position;    // ground origin, in whole tiles relative to the camera
in uvec4 a_Metrics;     // size.xy in whole tiles, layer, orientation
in uvec4 a_Lattice;     // motif period.xy in whole tiles, phase.xy of the run within it
in uint  a_Atlas;       // slice of the array texture
in vec4  a_Color;

out vec2 v_Texture;
out vec4 v_Color;
flat out uint v_Slice;

#ifdef ENABLE_NORMAL_MAPPING
flat out vec4 v_Basis;
#endif

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

    // A tile lies flat on the ground rather than standing upright, so its quad spans world X and world Z
    // and carries no elevation of its own.
    vec2 Ground = vec2(a_Position) + Corner * vec2(a_Metrics.xy);

    // Tile layers stack on the same ground position, so only this bias separates them.
    gl_Position    = u_Camera * vec4(Ground.x, 0.0, Ground.y, 1.0);
    gl_Position.z -= float(a_Metrics.z) * TILE_LAYER_BIAS;

    // The art lands on a lattice the ground defines, so the run carries only its phase within the period.
    vec2 Period = vec2(a_Lattice.xy);
    vec2 Phase  = vec2(a_Lattice.zw);
    vec2 Size   = vec2(a_Metrics.xy);
    uint Facing = a_Metrics.w;

    vec2 Lattice = Phase + Corner * Size;

    if ((Facing & TILE_TRANSPOSE) != 0u)
    {
        Lattice = Lattice.yx;
    }
    if ((Facing & TILE_MIRROR_X) != 0u)
    {
        Lattice.x = Period.x - Lattice.x;
    }
    if ((Facing & TILE_MIRROR_Y) != 0u)
    {
        Lattice.y = Period.y - Lattice.y;
    }

    v_Texture = vec2(Lattice.x / Period.x, (Period.y - Lattice.y) / Period.y);
    v_Color   = a_Color;
    v_Slice   = a_Atlas & 0xFFFFu;

#ifdef ENABLE_NORMAL_MAPPING

    vec2 Sign = vec2((Facing & TILE_MIRROR_X) != 0u ? -1.0 : 1.0, (Facing & TILE_MIRROR_Y) != 0u ? -1.0 : 1.0);

    v_Basis = (Facing & TILE_TRANSPOSE) != 0u ? vec4(0.0, Sign.y, Sign.x, 0.0) : vec4(Sign.x, 0.0, 0.0, Sign.y);

#endif
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2DArray t_Albedo;

#ifdef ENABLE_NORMAL_MAPPING
uniform sampler2DArray t_Normal;
#endif

in vec2 v_Texture;
in vec4 v_Color;
flat in uint v_Slice;

#ifdef ENABLE_NORMAL_MAPPING
flat in vec4 v_Basis;
#endif

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normal;

void main()
{
    vec4 Texel = texture(t_Albedo, vec3(v_Texture, float(v_Slice)));

#ifdef ENABLE_ALPHA_TEST
    if (Texel.a < 0.5)
    {
        discard;
    }
#endif

    out_Albedo = v_Color * Texel;

#ifdef ENABLE_NORMAL_MAPPING

    // A tile never tilts, so tangent space maps onto the ground with a fixed swizzle: the map's own
    // up axis becomes the world's, and a flat map lands back on the constant an unlit tile writes.
    vec3 Tangent = texture(t_Normal, vec3(v_Texture, float(v_Slice))).xyz * 2.0 - 1.0;

    vec2 Plane  = vec2(dot(v_Basis.xy, Tangent.xy), dot(v_Basis.zw, Tangent.xy));
    vec3 Normal = normalize(vec3(Plane.x, Tangent.z, Plane.y));

    out_Normal = vec4(Normal * 0.5 + 0.5, 1.0);

#else

    // A tile lies flat on the ground, so its normal is the world's up axis rather than the viewer's.
    // The ground faces the sky and passes no light through, so it is solid in the opacity channel.
    out_Normal = vec4(0.5, 1.0, 0.5, 1.0);

#endif
}

#endif // FRAGMENT_SHADER
