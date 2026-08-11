// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

layout(std140, binding = 0) uniform cb_Global
{
    mat4 u_Camera;
};

layout(std140, binding = 1) uniform cb_Pass
{
    mat4 u_Inverse;    // turns a clip-space probe back into world space
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

in vec4 a_Params0;    // center.xyz, radius || range

#if defined(LIGHT_SPOT)
in vec4 a_Params1;    // direction.xyz, cos(inner)
#endif

in vec4 a_Color;      // rgb, falloff

#if defined(LIGHT_SPOT)
in float a_Outer;     // cos(outer)
#endif

out vec4 v_Probe;     // clip.xy, screen.xy
out vec4 v_Light;     // center.xyz, radius || range
out vec4 v_Color;

#if defined(LIGHT_SPOT)
out vec4  v_Spot;     // direction.xyz, cos(inner)
out float v_Outer;    // cos(outer)
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
    vec2  Corner = TessellateRect(gl_VertexID);
    vec3  Center = a_Params0.xyz;
    float Extent = a_Params0.w;

    // The projection is affine, so the sphere's screen half-extent is what each world axis contributes to the
    // screen at that radius. Summing them bounds the sphere by the box around it, conservative by the corners.
    vec4 Middle = u_Camera * vec4(Center, 1.0);
    vec2 Radius = abs((u_Camera * vec4(Extent, 0.0, 0.0, 0.0)).xy)
                + abs((u_Camera * vec4(0.0, Extent, 0.0, 0.0)).xy)
                + abs((u_Camera * vec4(0.0, 0.0, Extent, 0.0)).xy);

    vec2 Clip = Middle.xy + (Corner * 2.0 - 1.0) * Radius;

    gl_Position = vec4(Clip, 0.0, 1.0);

    v_Probe = vec4(Clip, Clip * 0.5 + 0.5);
    v_Light = a_Params0;
    v_Color = a_Color;

#if defined(LIGHT_SPOT)
    v_Spot  = a_Params1;
    v_Outer = a_Outer;
#endif
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2D t_Normal;
uniform sampler2D t_Depth;

in vec4 v_Probe;
in vec4 v_Light;
in vec4 v_Color;

#if defined(LIGHT_SPOT)
in vec4  v_Spot;
in float v_Outer;
#endif

layout(location = 0) out vec4 out_Color;

void main()
{
    // Depth is position compressed to one channel: the pixel's clip coordinates plus its depth are three
    // knowns, and the inverse of the camera turns them back into the one world point that produced them.
    float Depth = texture(t_Depth, v_Probe.zw).r;
    vec4  Probe = u_Inverse * vec4(v_Probe.xy, Depth, 1.0);
    vec3  World = Probe.xyz / Probe.w;

    vec3  Relative = v_Light.xyz - World;
    float Distance = length(Relative);
    vec3  Incident = (Distance > 0.0001) ? Relative / Distance : vec3(0.0, 1.0, 0.0);

    float Attenuation = clamp(1.0 - pow(clamp(Distance / v_Light.w, 0.0, 1.0), v_Color.a), 0.0, 1.0);

#if defined(LIGHT_SPOT)
    float CosAngle = dot(-Incident, normalize(v_Spot.xyz));
    Attenuation *= smoothstep(v_Outer, v_Spot.w, CosAngle);
#endif

    if (Attenuation < 0.001)
    {
        discard;
    }

#ifdef ENABLE_NORMAL_MAPPING
    vec4  Surface    = texture(t_Normal, v_Probe.zw);
    vec3  Normal     = normalize(Surface.rgb * 2.0 - 1.0);
    float Lambert    = clamp(dot(Normal, Incident) / 1.0, 0.0, 1.0);

    float Through    = max(dot(-Normal, Incident), 0.0) * (1.0 - Surface.a);
    float NormalDotL = max(Lambert, Through);
#else
    float NormalDotL = 1.0;
#endif

    out_Color = vec4(v_Color.rgb * (Attenuation * NormalDotL), Attenuation);
}

#endif // FRAGMENT_SHADER