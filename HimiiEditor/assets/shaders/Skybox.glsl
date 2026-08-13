#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

layout(location = 0) out vec3 v_TexCoords;

layout(std140, binding = 1) uniform SkyboxUniforms
{
	mat4 u_View;
	mat4 u_Projection;
};

void main()
{
    v_TexCoords = a_Position;
    vec4 pos = u_Projection * u_View * vec4(a_Position, 1.0);
    gl_Position = pos.xyww;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec3 v_TexCoords;

layout(binding = 0) uniform samplerCube u_Skybox;

vec3 ApproximateLinearToSrgb(vec3 color)
{
	return pow(color, vec3(1.0 / 2.2));
}

vec3 ApplyCheapToneMap(vec3 color)
{
	return color / (color + vec3(1.0));
}

void main()
{
	vec3 environmentColor = texture(u_Skybox, v_TexCoords).rgb;
	environmentColor = ApplyCheapToneMap(environmentColor);
	o_Color = vec4(ApproximateLinearToSrgb(environmentColor), 1.0);
}
