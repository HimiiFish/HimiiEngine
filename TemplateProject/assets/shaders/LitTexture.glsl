#type vertex
#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in float a_TexIndex;
layout(location = 5) in float a_TilingFactor;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;
out float v_TilingFactor;

void main()
{
    vec4 world = u_Transform * vec4(a_Position, 1.0);
    v_WorldPos = world.xyz;
    // 法线变换忽略非均匀缩放，假设模型空间单位
    v_Normal = mat3(u_Transform) * a_Normal;
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TexIndex = a_TexIndex;
    v_TilingFactor = a_TilingFactor;
    gl_Position = u_ViewProjection * world;
}

#type fragment
#version 410 core

layout(location=0) out vec4 color;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;

uniform sampler2D u_Texture[32];

// 全局定向光 + 环境光
uniform vec3 u_AmbientColor = vec3(0.35, 0.40, 0.45);
uniform float u_AmbientIntensity = 0.4;
uniform vec3 u_LightDir = normalize(vec3(-0.4, -1.0, -0.3));
uniform vec3 u_LightColor = vec3(1.0, 0.98, 0.92);
uniform float u_LightIntensity = 1.0;

void main()
{
    vec3 N = normalize(v_Normal);
    vec3 L = normalize(-u_LightDir);
    float NdotL = max(dot(N, L), 0.0);
    vec3 ambient = u_AmbientColor * u_AmbientIntensity;
    vec3 diffuse = u_LightColor * u_LightIntensity * NdotL;

    vec4 albedo = texture(u_Texture[int(v_TexIndex)], v_TexCoord * v_TilingFactor) * v_Color;
    vec3 lit = albedo.rgb * (ambient + diffuse);
    color = vec4(lit, albedo.a);
}
