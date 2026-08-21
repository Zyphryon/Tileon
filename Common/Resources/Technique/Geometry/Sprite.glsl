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

in vec4 a_Transform0;
in vec4 a_Transform1;
in vec4 a_Transform2;

in vec4 a_Frame;
in vec2 a_Size;
in vec4 a_Color;
in uint a_Facing;

out vec2 v_Texture;
out vec4 v_Color;

#ifdef ENABLE_NORMAL_MAPPING
out vec3 v_AxisX;
out vec3 v_AxisY;
#endif
out vec3 v_AxisZ;

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

    // The columns of the affine are the local axes the art is laid down along.
    vec3 ColumnX = vec3(a_Transform0.x, a_Transform1.x, a_Transform2.x);
    vec3 ColumnY = vec3(a_Transform0.y, a_Transform1.y, a_Transform2.y);
    vec3 ColumnZ = vec3(a_Transform0.z, a_Transform1.z, a_Transform2.z);
    vec3 Origin  = vec3(a_Transform0.w, a_Transform1.w, a_Transform2.w);

    // Two of the three axes span the quad; the one left over is the face it turns towards. An upright
    // sprite turns back down -z at the viewer, whereas art laid against a surface turns away from it.
    uint Plane   = (a_Facing >> FACING_PLANE_SHIFT) & FACING_PLANE_MASK;

    vec3 AxisU   = (Plane == PLANE_WALL)    ? ColumnY : ColumnX;
    vec3 AxisV   = (Plane == PLANE_UPRIGHT) ? ColumnY : ColumnZ;
    vec3 Facing  = (Plane == PLANE_GROUND)  ? ColumnY : (Plane == PLANE_WALL) ? ColumnX : -ColumnZ;

    vec3 Position = Origin + Corner.x * a_Size.x * AxisU + Corner.y * a_Size.y * AxisV;

    gl_Position = u_Camera * vec4(Position, 1.0);

    // Art laid flat against a surface is coplanar with it, and would z-fight it without a nudge forward.
    if ((a_Facing & FACING_COPLANAR) != 0u)
    {
        gl_Position.z -= COPLANAR_DEPTH_BIAS;
    }

    // The art is laid down mirrored or turned by exchanging the corner before it picks a point in the frame.
    vec2 Sample = vec2(Corner.x, 1.0 - Corner.y);

    if ((a_Facing & FACING_MIRROR_X) != 0u)
    {
        Sample.x = 1.0 - Sample.x;
    }
    if ((a_Facing & FACING_MIRROR_Y) != 0u)
    {
        Sample.y = 1.0 - Sample.y;
    }

    v_Texture = mix(a_Frame.xy, a_Frame.zw, Sample);
    v_Color   = a_Color;


#ifdef ENABLE_NORMAL_MAPPING
    v_AxisX = normalize(AxisU);
    v_AxisY = normalize(AxisV);
#endif

    v_AxisZ = -normalize(Facing);
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2D t_Albedo;

#ifdef ENABLE_NORMAL_MAPPING
uniform sampler2D t_Normal;
#endif

in vec2 v_Texture;
in vec4 v_Color;

#ifdef ENABLE_NORMAL_MAPPING
in vec3 v_AxisX;
in vec3 v_AxisY;
#endif
in vec3 v_AxisZ;

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normal;

void main()
{
    vec4 Texel = texture(t_Albedo, v_Texture);

#ifdef ENABLE_ALPHA_TEST
    if (Texel.a < 0.5)
    {
        discard;
    }
#endif

    out_Albedo = v_Color * Texel;

#ifdef ENABLE_NORMAL_MAPPING
    vec4 Sampled = texture(t_Normal, v_Texture);
    vec3 Tangent = normalize(Sampled.rgb * 2.0 - 1.0);
    vec3 Normal  = normalize(Tangent.x * v_AxisX + Tangent.y * v_AxisY - Tangent.z * v_AxisZ);
#if defined(ENABLE_TRANSLUCENCY)
    float Opacity = Sampled.a;
#elif defined(ENABLE_ALPHA_TEST)
    float Opacity = 1.0;
#else
    float Opacity = out_Albedo.a;
#endif

    out_Normal = vec4(Normal * 0.5 + 0.5, Opacity);
#else
#if defined(ENABLE_ALPHA_TEST)
    float Opacity = 1.0;
#else
    float Opacity = out_Albedo.a;
#endif
    out_Normal = vec4(-v_AxisZ * 0.5 + 0.5, Opacity);
#endif
}

#endif // FRAGMENT_SHADER