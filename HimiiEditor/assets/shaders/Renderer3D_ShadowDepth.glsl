#type vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

layout(location = 3) in vec4 a_Color;
layout(location = 4) in vec4 a_CustomData;
layout(location = 5) in mat4 a_Transform;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_CameraPosition;
};

void main()
{
	gl_Position = u_ViewProjection * a_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

void main()
{
}
