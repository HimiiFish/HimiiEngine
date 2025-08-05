#type vertex
#version 410 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection; // 视图投影矩阵
uniform mat4 u_Transform; // 变换矩阵

out vec2 v_TexCoord;

void main()
{
    v_TexCoord=a_TexCoord;
    gl_Position = u_ViewProjection*u_Transform* vec4(a_Position, 1.0);
}

#type fragment
#version 410 core

layout(location=0) out vec4 color;
in vec2 v_TexCoord;

uniform vec4 u_Color;
uniform sampler2D u_Texture; // 颜色

void main()
{
    color = texture(u_Texture,v_TexCoord)*u_Color;
}