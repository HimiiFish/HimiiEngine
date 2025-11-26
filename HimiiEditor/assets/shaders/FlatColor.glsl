#type vertex
#version 410 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection; // ��ͼͶӰ����
uniform mat4 u_Transform; // �任����

out vec2 v_TexCoord;

void main()
{
    gl_Position = u_ViewProjection*u_Transform* vec4(a_Position, 1.0);
}

#type fragment
#version 410 core

layout(location=0) out vec4 color;

uniform vec4 u_Color; // ��ɫ

void main()
{
    color = u_Color;
}