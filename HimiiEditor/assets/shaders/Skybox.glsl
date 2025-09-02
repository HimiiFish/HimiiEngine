#type vertex
#version 410 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection; // 这里传入的是 P * mat4(mat3(V))，移除了平移
uniform mat4 u_Transform;      // 通常为单位矩阵

out vec3 v_Dir;

void main()
{
    // 使用立方体顶点作为方向（已移除平移的视图矩阵即可正确渲染天空盒）
    v_Dir = a_Position;
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 410 core

layout(location=0) out vec4 color;

in vec3 v_Dir;

// 简单的程序化渐变天空：上蓝下浅
uniform vec3 u_TopColor    = vec3(0.24, 0.52, 0.88);
uniform vec3 u_HorizonColor= vec3(0.85, 0.90, 0.98);
uniform vec3 u_BottomColor = vec3(0.95, 0.95, 1.00);

void main()
{
    vec3 dir = normalize(v_Dir);
    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    // 天顶->地平->地面分段过渡
    vec3 sky = mix(u_HorizonColor, u_TopColor, smoothstep(0.3, 1.0, t));
    vec3 ground = mix(u_HorizonColor, u_BottomColor, smoothstep(0.0, 0.7, 1.0 - t));
    vec3 c = mix(ground, sky, t);
    color = vec4(c, 1.0);
}
