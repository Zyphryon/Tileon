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

out vec2 v_Texture;
out vec4 v_Color;

#ifdef ENABLE_NORMAL_MAPPING
out vec3 v_AxisX;
out vec3 v_AxisY;
out vec3 v_AxisZ;
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
    vec3 Local  = vec3(Corner * a_Size, 0.0);

    vec3 Position = vec3(
        dot(Local, a_Transform0.xyz) + a_Transform0.w,
        dot(Local, a_Transform1.xyz) + a_Transform1.w,
        dot(Local, a_Transform2.xyz) + a_Transform2.w);

    // A sprite stands upright, so the projection already resolves its depth from the world position the
    // affine put it at; it carries no bias of its own.
    gl_Position = u_Camera * vec4(Position, 1.0);

    v_Texture = mix(a_Frame.xy, a_Frame.zw, vec2(Corner.x, 1.0 - Corner.y));
    v_Color   = a_Color;


#ifdef ENABLE_NORMAL_MAPPING
    v_AxisX = normalize(vec3(a_Transform0.x, a_Transform1.x, a_Transform2.x));
    v_AxisY = normalize(vec3(a_Transform0.y, a_Transform1.y, a_Transform2.y));
    v_AxisZ = normalize(vec3(a_Transform0.z, a_Transform1.z, a_Transform2.z));
#endif
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
in vec3 v_AxisZ;
#endif

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

    // The sprite plane faces +Z, so the tangent-space Z carries over to world space unchanged.
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

    out_Normal = vec4(0.5, 0.5, 0.0, Opacity);
#endif
}

#endif // FRAGMENT_SHADER