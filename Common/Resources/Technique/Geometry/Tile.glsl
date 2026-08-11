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
in uvec4 a_Metrics;     // size.xy in whole tiles, layer, reserved
in uvec4 a_Lattice;     // motif period.xy in whole tiles, phase.xy of the run within it
in uint  a_Atlas;       // slice of the array texture
in vec4  a_Color;

out vec2 v_Texture;
out vec4 v_Color;
flat out uint v_Slice;

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
    // V runs against world Y, which keeps the art upright while the ground climbs the screen.
    vec2 Period = vec2(a_Lattice.xy);
    vec2 Phase  = vec2(a_Lattice.zw);
    vec2 Size   = vec2(a_Metrics.xy);

    v_Texture = vec2((Phase.x + Corner.x * Size.x) / Period.x, (Period.y - Phase.y - Corner.y * Size.y) / Period.y);
    v_Color   = a_Color;
    v_Slice   = a_Atlas & 0xFFFFu;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2DArray t_Albedo;

in vec2 v_Texture;
in vec4 v_Color;
flat in uint v_Slice;

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

    // A tile lies flat on the ground, so its normal is the world's up axis rather than the viewer's.
    out_Normal = vec4(0.5, 1.0, 0.5, out_Albedo.a);
}

#endif // FRAGMENT_SHADER