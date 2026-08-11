// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Uniforms
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

layout(std140, binding = 0) uniform cb_Global
{
    mat4 u_Camera;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Constants
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

const vec4 kOutlineColor = vec4(0.033105, 1.000000, 0.170645, 0.85); // Wireframe, linear for sRGB 20/100/45.

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Vertex Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef VERTEX_SHADER

in vec3 a_Center;
in vec3 a_Extent;

#ifdef BOUNDARY_FLAT

vec2 TessellateRect(int VertexID)
{
    const vec2 kRectEdges[8] = vec2[8](
        vec2(-1, -1), vec2( 1, -1),
        vec2( 1, -1), vec2( 1,  1),
        vec2( 1,  1), vec2(-1,  1),
        vec2(-1,  1), vec2(-1, -1)
    );

    return kRectEdges[VertexID];
}

void main()
{
    vec4 Origin = u_Camera * vec4(a_Center, 1.0);
    vec2 Span   = abs((u_Camera * vec4(a_Extent.x, 0.0, 0.0, 0.0)).xy)
                + abs((u_Camera * vec4(0.0, a_Extent.y, 0.0, 0.0)).xy)
                + abs((u_Camera * vec4(0.0, 0.0, a_Extent.z, 0.0)).xy);

    gl_Position = vec4(Origin.xy + TessellateRect(gl_VertexID) * Span, Origin.z, Origin.w);
}

#else

vec3 TessellateBox(int VertexID)
{
    const vec3 kBoxEdges[24] = vec3[24](
        vec3(-1, -1, -1), vec3( 1, -1, -1),
        vec3(-1,  1, -1), vec3( 1,  1, -1),
        vec3(-1, -1,  1), vec3( 1, -1,  1),
        vec3(-1,  1,  1), vec3( 1,  1,  1),
        vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1, -1, -1), vec3( 1,  1, -1),
        vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1, -1,  1), vec3( 1,  1,  1),
        vec3(-1, -1, -1), vec3(-1, -1,  1),
        vec3( 1, -1, -1), vec3( 1, -1,  1),
        vec3(-1,  1, -1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1,  1,  1)
    );

    return kBoxEdges[VertexID];
}

void main()
{
    vec3 World = a_Center + TessellateBox(gl_VertexID) * a_Extent;

    gl_Position = u_Camera * vec4(World, 1.0);
}

#endif // BOUNDARY_FLAT

#endif // VERTEX_SHADER

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Fragment Shader
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 out_Color;

void main()
{
    out_Color = kOutlineColor;
}

#endif // FRAGMENT_SHADER