#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TextureCoordinate;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_CameraPosition;
};

layout(std140, binding = 3) uniform MeshUnlitUniforms
{
	mat4 u_Transform;
	vec4 u_AlbedoColor;
	int u_UseAlbedoTexture;
	int u_EntityID;
	int u_Padding0;
	int u_Padding1;
};

layout (location = 0) out vec2 v_TextureCoordinate;

void main()
{
	v_TextureCoordinate = a_TextureCoordinate;
	gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

layout (location = 0) in vec2 v_TextureCoordinate;

layout(std140, binding = 3) uniform MeshUnlitUniforms
{
	mat4 u_Transform;
	vec4 u_AlbedoColor;
	int u_UseAlbedoTexture;
	int u_EntityID;
	int u_Padding0;
	int u_Padding1;
};

layout(binding = 0) uniform sampler2D u_AlbedoTexture;

void main()
{
	vec4 color = u_AlbedoColor;
	if (u_UseAlbedoTexture != 0)
		color *= texture(u_AlbedoTexture, v_TextureCoordinate);

	o_Color = color;
	o_EntityID = u_EntityID;
}
