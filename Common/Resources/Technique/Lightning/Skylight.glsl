// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

layout(std140, binding = 1) uniform cb_Pass
{
    vec4 u_SunColor;      // RGB = Color * Intensity * Headroom, A = Sun Direction X
    vec4 u_SkyColor;      // RGB = Color * Intensity * Headroom, A = Sun Direction Y
    vec4 u_GroundColor;   // RGB = Color * Intensity * Headroom, A = Sun Direction Z
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

out vec2 v_Texture;

void main()
{
    v_Texture   = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(v_Texture * 2.0 - 1.0, 0.0, 1.0);
}

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

uniform sampler2D t_Normal;

in vec2 v_Texture;

layout(location = 0) out vec3 out_Color;

void main()
{
    vec4 Surface = texelFetch(t_Normal, ivec2(gl_FragCoord.xy), 0);
    vec3 Normal  = normalize(Surface.rgb * 2.0 - 1.0);

    // Hemisphere ambient. The weight is world Y, so this reads as "facing the sky" only because the normal
    // buffer stores world-space normals with Y up.
    float Weight = Normal.y * 0.5 + 0.5;
    vec3 Ambient = mix(u_GroundColor.rgb, u_SkyColor.rgb, Weight);
    vec3 Toward  = vec3(u_SunColor.w, u_SkyColor.w, u_GroundColor.w);

    // The alpha of the normal buffer is opacity, which doubles as thickness: the more of it a surface lets
    // through, the more the sun behind it shows.
    float Facing = max(dot(Normal, Toward), 0.0) + max(dot(-Normal, Toward), 0.0) * (1.0 - Surface.a);
    vec3  Sun    = u_SunColor.rgb * Facing;

    // Two independent contributions, so they sum, at whatever intensity they were authored with. Three
    // channels, because that is what the radiance target has, and it is a float format with no ceiling to
    // protect: the composite's tone curve is what brings the range back down.
    out_Color = Ambient + Sun;
}

#endif // FRAGMENT_SHADER