#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_NearPoint;
layout(location = 1) out vec3 v_FarPoint;

layout(std140, binding = 2) uniform GridUniforms
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Near;
    float u_Far;
    float u_CameraDistance;
    float u_UseXyPlane;
};

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection)
{
    mat4 viewInverse = inverse(view);
    mat4 projectionInverse = inverse(projection);
    vec4 unprojectedPoint = viewInverse * projectionInverse * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main()
{
    vec3 position = a_Position;
    v_NearPoint = UnprojectPoint(position.x, position.y, -1.0, u_View, u_Proj);
    v_FarPoint = UnprojectPoint(position.x, position.y, 1.0, u_View, u_Proj);
    gl_Position = vec4(position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec3 v_NearPoint;
layout(location = 1) in vec3 v_FarPoint;

layout(std140, binding = 2) uniform GridUniforms
{
    mat4 u_View;
    mat4 u_Proj;
    float u_Near;
    float u_Far;
    float u_CameraDistance;
    float u_UseXyPlane;
};

float ComputeDepth(vec3 worldPosition)
{
    vec4 clipSpacePosition = u_Proj * u_View * vec4(worldPosition, 1.0);
    return (clipSpacePosition.z / clipSpacePosition.w) * 0.5 + 0.5;
}

float ComputeLinearDepth(vec3 worldPosition)
{
    vec4 clipSpacePosition = u_Proj * u_View * vec4(worldPosition, 1.0);
    float clipSpaceDepth = (clipSpacePosition.z / clipSpacePosition.w) * 2.0 - 1.0;
    float linearDepth = (2.0 * u_Near * u_Far) /
                        (u_Far + u_Near - clipSpaceDepth * (u_Far - u_Near));
    return linearDepth / u_Far;
}

// Anti-aliased grid lines on a plane (world XZ in shader space).
// densityFade kills lines when more than ~one cell fits in a pixel — prevents far-field moiré.
float GridLineAlpha(vec2 planePosition, float cellSize)
{
    vec2 coordinate = planePosition / cellSize;
    vec2 derivative = fwidth(coordinate);
    float cellsPerPixel = max(derivative.x, derivative.y);
    float densityFade = 1.0 - smoothstep(0.15, 0.55, cellsPerPixel);
    if (densityFade <= 0.0)
        return 0.0;

    vec2 distanceToLine = abs(fract(coordinate - 0.5) - 0.5);
    vec2 line = distanceToLine / max(derivative, vec2(1e-6));
    float alpha = 1.0 - min(min(line.x, line.y), 1.0);
    return alpha * densityFade;
}

float AxisLineAlpha(float planeCoordinate)
{
    float derivative = fwidth(planeCoordinate);
    // Same idea: once the axis footprint exceeds a few pixels of uncertainty, fade it.
    float densityFade = 1.0 - smoothstep(2.0, 8.0, derivative);
    float alpha = 1.0 - min(abs(planeCoordinate) / max(derivative * 1.35, 1e-6), 1.0);
    return alpha * densityFade;
}

void main()
{
    float intersectionParameter = -v_NearPoint.y / (v_FarPoint.y - v_NearPoint.y);
    if (intersectionParameter < 0.0)
        discard;

    vec3 fragmentPosition = v_NearPoint + intersectionParameter * (v_FarPoint - v_NearPoint);
    gl_FragDepth = ComputeDepth(fragmentPosition);

    vec2 planePosition = fragmentPosition.xz;

    float referenceDistance = max(u_CameraDistance, 0.5);
    float logDistance = log(referenceDistance) / log(10.0);
    float level = floor(logDistance);
    float blend = fract(logDistance);

    float minorCellSize = pow(10.0, level - 1.0);
    float majorCellSize = pow(10.0, level);
    float nextMajorCellSize = pow(10.0, level + 1.0);

    float minorAlpha = GridLineAlpha(planePosition, minorCellSize) * (1.0 - blend);
    float majorAlpha = GridLineAlpha(planePosition, majorCellSize);
    float nextMajorAlpha = GridLineAlpha(planePosition, nextMajorCellSize) * blend;

    // Minor: thin/faint; major: stronger. Matches UE-style hierarchy.
    float lineAlpha = max(minorAlpha * 0.32, max(majorAlpha * 0.55, nextMajorAlpha * 0.55));

    // X axis = line where plane Z == 0 (planePosition.y). Always red.
    float xAxisAlpha = AxisLineAlpha(planePosition.y);
    // Other plane axis = line where plane X == 0. 3D: world Z (blue); 2D xyPlane: world Y (green).
    float secondaryAxisAlpha = AxisLineAlpha(planePosition.x);

    vec3 lineColor = vec3(0.40, 0.40, 0.42);
    vec3 xAxisColor = vec3(0.90, 0.22, 0.22);
    vec3 secondaryAxisColor = u_UseXyPlane > 0.5 ? vec3(0.25, 0.82, 0.32) : vec3(0.22, 0.45, 0.95);

    vec3 color = lineColor;
    float alpha = lineAlpha;

    color = mix(color, xAxisColor, xAxisAlpha);
    alpha = max(alpha, xAxisAlpha * 0.95);
    color = mix(color, secondaryAxisColor, secondaryAxisAlpha);
    alpha = max(alpha, secondaryAxisAlpha * 0.95);

    float linearDepth = ComputeLinearDepth(fragmentPosition);
    // Fade earlier so dense far grids never reach the moiré regime.
    float depthFade = 1.0 - smoothstep(0.05, 0.45, linearDepth);

    float planeDistance = length(planePosition);
    float planeFade = 1.0 - smoothstep(referenceDistance * 1.8, referenceDistance * 7.0, planeDistance);

    alpha *= depthFade * planeFade;

    if (alpha <= 0.004)
        discard;

    o_Color = vec4(color, alpha);
}
