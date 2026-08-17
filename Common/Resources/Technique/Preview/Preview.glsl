// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#if defined(PREVIEW_DEPTH)
layout(std140, binding = 0) uniform cb_Global
{
    mat4 u_Camera;
    mat4 u_CameraInverse;
};
#endif

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

out vec2 v_Texture;
out vec2 v_Probe;     // the pixel's clip-space coordinates

void main()
{
    v_Texture   = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(v_Texture * 2.0 - 1.0, 0.0, 1.0);
    v_Probe     = gl_Position.xy;
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2D t_Source;

in vec2 v_Texture;
in vec2 v_Probe;

layout(location = 0) out vec4 out_Color;

vec3 sRGBEncode(vec3 Value)
{
    return mix(Value / 12.92, pow((Value + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), Value));
}

void main()
{
#if defined(PREVIEW_NORMAL)

    vec3 Normal = texture(t_Source, v_Texture).rgb;
    out_Color = vec4(sRGBEncode(Normal), 1.0);

#elif defined(PREVIEW_DEPTH)

    float Depth = texture(t_Source, v_Texture).r;
    vec4  Probe = u_CameraInverse * vec4(v_Probe, Depth, 1.0);
    vec3  World = Probe.xyz / Probe.w;

    float Elevation = clamp(World.y / ELEVATION_SCALE, 0.0, 1.0);

    vec3 Color = mix(vec3(0.12), vec3(1.0), Elevation);

    Color = mix(Color, vec3(0.15, 0.45, 1.00), step(Depth, PREVIEW_MIDGROUND));
    Color = mix(Color, vec3(1.00, 0.20, 0.15), step(PREVIEW_BACKGROUND, Depth));

    out_Color = vec4(sRGBEncode(Color * step(Depth, 0.99999)), 1.0);

#else

    out_Color = vec4(texture(t_Source, v_Texture).rgb, 1.0);

#endif
}

#endif // FRAGMENT_SHADER