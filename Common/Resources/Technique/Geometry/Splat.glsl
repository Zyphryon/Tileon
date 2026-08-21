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

in ivec2 a_Origin;    // region corner, in tiles, relative to the frame's origin
in uint  a_Weights;   // slice of the weight array this region wrote its map into
in uvec4 a_Palette;   // slice of the terrain array each of the four slots draws
in vec4  a_Mapping;   // how often each slot repeats across one world unit
in vec2  a_Phase0;    // where slot 0 already stands in its repeat, at the region's corner
in vec2  a_Phase1;    // the same, for slot 1
in vec2  a_Phase2;    // the same, for slot 2
in vec2  a_Phase3;    // the same, for slot 3
in uvec4 a_Tint;      // the color each slot's art is multiplied by, packed as RGBA8

out vec2 v_Ground;
flat out uint  v_Weights;
flat out uvec4 v_Palette;
flat out vec4  v_Mapping;
flat out vec2  v_Phase[4];
flat out uvec4 v_Tint;

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
    vec2 Tile   = vec2(a_Origin) + Corner * SPLAT_UNITS_PER_REGION;

    gl_Position = u_Camera * vec4(Tile.x, 0.0, Tile.y, 1.0);

    v_Ground  = Corner;
    v_Weights = a_Weights;
    v_Palette = a_Palette;
    v_Mapping    = a_Mapping;
    v_Phase[0]   = a_Phase0;
    v_Phase[1]   = a_Phase1;
    v_Phase[2]   = a_Phase2;
    v_Phase[3]   = a_Phase3;
    v_Tint       = a_Tint;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2DArray t_Weight;
uniform sampler2DArray t_Albedo;
#ifdef ENABLE_NORMAL_MAPPING
uniform sampler2DArray t_Normal;
#endif

in vec2 v_Ground;
flat in uint  v_Weights;
flat in uvec4 v_Palette;
flat in vec4  v_Mapping;
flat in vec2  v_Phase[4];
flat in uvec4 v_Tint;

vec4 UnpackTint(uint Packed)
{
    uvec4 Bytes = uvec4(Packed, Packed >> 8u, Packed >> 16u, Packed >> 24u) & 0xFFu;

    return vec4(Bytes) * (1.0 / 255.0);
}

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normal;

void main()
{
    float Size    = SPLAT_UNITS_PER_REGION + 2.0 * SPLAT_MAP_BORDER;
    vec2  Sampled = (v_Ground * SPLAT_UNITS_PER_REGION + SPLAT_MAP_BORDER) / Size;

    vec4 Weight = texture(t_Weight, vec3(Sampled, float(v_Weights)));

    if (dot(Weight, vec4(1.0)) < SPLAT_WEIGHT_FLOOR)
    {
        discard;
    }

    Weight /= max(dot(Weight, vec4(1.0)), 0.0001);

    // Every terrain sweeps over the world at its own rate, picked up from where the sweep already stood at
    // this region's corner, so the art runs on across a border instead of starting over at it.
    vec2 Texture[4];
    vec4 Source[4];

    for (int Slot = 0; Slot < 4; ++Slot)
    {
        Texture[Slot] = v_Phase[Slot] + v_Ground * SPLAT_UNITS_PER_REGION * v_Mapping[Slot];

        Source[Slot] = (Weight[Slot] > SPLAT_WEIGHT_FLOOR)
            ? texture(t_Albedo, vec3(Texture[Slot], float(v_Palette[Slot])))
            : vec4(0.0);
    }

    vec4 Albedo = vec4(0.0);
    vec3 Normal = vec3(0.0);

    for (int Slot = 0; Slot < 4; ++Slot)
    {
        if (Weight[Slot] > SPLAT_WEIGHT_FLOOR)
        {
            Albedo += Source[Slot] * UnpackTint(v_Tint[Slot]) * Weight[Slot];

#ifdef ENABLE_NORMAL_MAPPING
            // Every slice carries relief, flat where nobody authored any, so the sample needs no guard.
            vec3 Tangent = texture(t_Normal, vec3(Texture[Slot], float(v_Palette[Slot]))).xyz * 2.0 - 1.0;

            Normal += Tangent * Weight[Slot];
#endif
        }
    }

    out_Albedo = Albedo;

#ifdef ENABLE_NORMAL_MAPPING
    // The ground faces up, so tangent Z is world up and the other two lie along the plane.
    vec3 Surface = normalize(vec3(Normal.x, max(Normal.z, 0.0001), Normal.y));
    out_Normal = vec4(Surface * 0.5 + 0.5, 1.0);
#else
    out_Normal = vec4(0.5, 1.0, 0.5, 1.0);
#endif
}

#endif // FRAGMENT_SHADER