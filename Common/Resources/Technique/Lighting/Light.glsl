// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

layout(std140, binding = 0) uniform cb_Global
{
    mat4 u_Camera;
    mat4 u_CameraInverse;
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

    // A sphere reaches the length of each screen axis' world mix; summing instead bounds the box around it.
    vec2 Ex = (u_Camera * vec4(1.0, 0.0, 0.0, 0.0)).xy;
    vec2 Ey = (u_Camera * vec4(0.0, 1.0, 0.0, 0.0)).xy;
    vec2 Ez = (u_Camera * vec4(0.0, 0.0, 1.0, 0.0)).xy;

    vec2 Spread = vec2(length(vec3(Ex.x, Ey.x, Ez.x)),
                       length(vec3(Ex.y, Ey.y, Ez.y)));

#if defined(LIGHT_SPOT)
    // A cone barely fills its range sphere, so bound the apex and the disc it opens onto instead.
    float Widest = Extent * sqrt(clamp(1.0 - a_Outer * a_Outer, 0.0, 1.0));
    vec2  Apex   = (u_Camera * vec4(Center, 1.0)).xy;
    vec2  Mouth  = (u_Camera * vec4(Center + a_Params1.xyz * Extent, 1.0)).xy;

    vec2 Lower = min(Apex, Mouth - Widest * Spread);
    vec2 Upper = max(Apex, Mouth + Widest * Spread);

    vec2 Clip = mix(Lower, Upper, Corner);
#else
    vec4 Middle = u_Camera * vec4(Center, 1.0);
    vec2 Clip   = Middle.xy + (Corner * 2.0 - 1.0) * (Extent * Spread);
#endif

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

layout(location = 0) out vec3 out_Color;

void main()
{
    // Clip position and depth recover the world point through the inverse camera.
    ivec2 Texel = ivec2(gl_FragCoord.xy);
    float Depth = texelFetch(t_Depth, Texel, 0).r;
    vec4  Probe = u_CameraInverse * vec4(v_Probe.xy, Depth, 1.0);
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
    // Filtering a normal across an edge blends two surfaces, so read it by texel.
    vec4 Surface = texelFetch(t_Normal, Texel, 0);
    vec3 Normal  = normalize(Surface.rgb * 2.0 - 1.0);

    // Normal alpha is opacity, reused as thickness so a light behind a surface bleeds through. Nothing
    // else may claim that channel.
    float Lambert = clamp(dot(Normal, Incident), 0.0, 1.0);
    float Through = max(dot(-Normal, Incident), 0.0) * (1.0 - Surface.a);

    // The dots are negatives, so at most one survives the clamp. A sum, not a max, so it still holds if
    // either gains a wrap term.
    float NormalDotL = Lambert + Through;
#else
    float NormalDotL = 1.0;
#endif

    out_Color = v_Color.rgb * (Attenuation * NormalDotL);
}

#endif // FRAGMENT_SHADER