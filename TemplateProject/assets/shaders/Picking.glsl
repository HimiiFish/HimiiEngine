#type vertex
#version 410 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 410 core

// Route to color attachment 1 in FBO (see glDrawBuffers setup)
layout(location = 1) out uvec4 o_ID;
uniform uint u_EntityID;

void main()
{
    o_ID = uvec4(u_EntityID, 0, 0, 0);
}
