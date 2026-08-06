#type vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

layout(location = 3) in vec4 a_Color;
layout(location = 4) in vec4 a_CustomData; // .x = TexIndex, .y = EntityID, .z = Specular, .w = Shininess
layout(location = 5) in mat4 a_Transform;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_CameraPosition;
};

struct VertexOutput
{
	vec4 Color;
	vec3 Normal;
	vec2 TexCoord;
	vec3 WorldPosition;
	float Specular;
	float Shininess;
};

layout (location = 0) out VertexOutput v_Output;
layout (location = 6) out flat float v_TexIndex;
layout (location = 7) out flat int v_EntityID;

void main()
{
	v_Output.Color = a_Color;
	v_Output.TexCoord = a_TexCoord;
	v_Output.Specular = a_CustomData.z;
	v_Output.Shininess = a_CustomData.w;
	v_TexIndex = a_CustomData.x;
	v_EntityID = int(a_CustomData.y);

	vec4 worldPosition = a_Transform * vec4(a_Position, 1.0);
	v_Output.WorldPosition = worldPosition.xyz;
	gl_Position = u_ViewProjection * worldPosition;

	mat3 normalMatrix = transpose(inverse(mat3(a_Transform)));
	v_Output.Normal = normalize(normalMatrix * a_Normal);
}

#type fragment
#version 450 core
layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
	vec3 Normal;
	vec2 TexCoord;
	vec3 WorldPosition;
	float Specular;
	float Shininess;
};

layout (location = 0) in VertexOutput v_Output;
layout (location = 6) in flat float v_TexIndex;
layout (location = 7) in flat int v_EntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_CameraPosition;
};

layout(std140, binding = 4) uniform SceneLighting
{
	vec4 u_DirectionalLightDirectionIntensity; // xyz = travel direction, w = hasLight (1/0)
	vec4 u_DirectionalLightColor;              // rgb = color, a = intensity
	vec4 u_AmbientColorIntensity;              // rgb = color, a = intensity
	mat4 u_LightViewProjection;
	vec4 u_ShadowParameters;                   // x = hasShadowMap, y = depthBias, z = shadowTexelWorldSize
	vec4 u_PointLightCount;                    // x = count
	vec4 u_PointLightPositionRange[8];         // xyz = position, w = range
	vec4 u_PointLightColorIntensity[8];        // rgb = color, a = intensity
};

layout(binding = 0) uniform sampler2D u_Textures[31];
layout(binding = 31) uniform sampler2DShadow u_ShadowMap;

vec3 ApproximateSrgbToLinear(vec3 color)
{
	return pow(color, vec3(2.2));
}

vec3 ApproximateLinearToSrgb(vec3 color)
{
	return pow(color, vec3(1.0 / 2.2));
}

float SampleHardShadow(vec3 worldPosition, vec3 normal, vec3 lightDirectionTowardSurface)
{
	if (u_ShadowParameters.x < 0.5)
		return 1.0;

	vec3 surfaceNormal = normalize(normal);
	float surfaceSlope = clamp(1.0 - max(dot(surfaceNormal, lightDirectionTowardSurface), 0.0), 0.0, 1.0);

	// Normal-offset bias: move the lookup along the surface normal by ~one shadow texel instead of
	// leaning on a large depth bias, which is what detaches contact shadows (peter-panning).
	float normalOffsetWorld = u_ShadowParameters.z * 1.5 * (1.0 + surfaceSlope * 2.0);
	vec4 lightSpacePosition =
		u_LightViewProjection * vec4(worldPosition + surfaceNormal * normalOffsetWorld, 1.0);
	vec3 projectedCoordinates = lightSpacePosition.xyz / lightSpacePosition.w;
	projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

	if (projectedCoordinates.z > 1.0
		|| projectedCoordinates.x < 0.0 || projectedCoordinates.x > 1.0
		|| projectedCoordinates.y < 0.0 || projectedCoordinates.y > 1.0)
	{
		return 1.0;
	}

	float depthBias = u_ShadowParameters.y * (1.0 + surfaceSlope * 2.0);
	return texture(u_ShadowMap, vec3(projectedCoordinates.xy, projectedCoordinates.z - depthBias));
}

float EvaluatePointLightAttenuation(float distanceToLight, float range)
{
	float safeRange = max(range, 0.0001);
	float distanceOverRange = distanceToLight / safeRange;
	if (distanceOverRange >= 1.0)
		return 0.0;

	float inverseSquare = 1.0 / (1.0 + distanceToLight * distanceToLight);
	float smoothCutoff = 1.0 - distanceOverRange * distanceOverRange;
	smoothCutoff *= smoothCutoff;
	return inverseSquare * smoothCutoff;
}

vec3 EvaluateSpecular(vec3 normal, vec3 lightDirectionTowardSurface, vec3 viewDirection,
	float specularStrength, float shininess)
{
	vec3 halfwayDirection = normalize(lightDirectionTowardSurface + viewDirection);
	return vec3(pow(max(dot(normal, halfwayDirection), 0.0), max(shininess, 1.0)) * specularStrength);
}

void main()
{
	vec4 albedo = v_Output.Color;
	if (v_TexIndex > 0.0)
	{
		int textureIndex = int(v_TexIndex);
		vec4 sampled = texture(u_Textures[textureIndex], v_Output.TexCoord);
		sampled.rgb = ApproximateSrgbToLinear(sampled.rgb);
		albedo *= sampled;
	}

	float hasDirectionalLight = u_DirectionalLightDirectionIntensity.w;
	int pointLightCount = int(u_PointLightCount.x + 0.5);
	if (hasDirectionalLight < 0.5 && pointLightCount <= 0)
	{
		o_Color = vec4(0.0, 0.0, 0.0, albedo.a);
		o_EntityID = v_EntityID;
		return;
	}

	vec3 normal = normalize(v_Output.Normal);
	vec3 viewDirection = normalize(u_CameraPosition.xyz - v_Output.WorldPosition);
	vec3 accumulatedLight = vec3(0.0);

	if (hasDirectionalLight >= 0.5)
	{
		vec3 lightTravelDirection = normalize(u_DirectionalLightDirectionIntensity.xyz);
		vec3 lightDirectionTowardSurface = -lightTravelDirection;
		float diffuseFactor = max(dot(normal, lightDirectionTowardSurface), 0.0);
		vec3 lightColor = u_DirectionalLightColor.rgb * u_DirectionalLightColor.a;
		vec3 ambient = u_AmbientColorIntensity.rgb * u_AmbientColorIntensity.a;

		vec3 specular = vec3(0.0);
		if (diffuseFactor > 0.0)
			specular = EvaluateSpecular(normal, lightDirectionTowardSurface, viewDirection,
				v_Output.Specular, v_Output.Shininess);

		float shadowFactor = SampleHardShadow(v_Output.WorldPosition, normal, lightDirectionTowardSurface);
		accumulatedLight += ambient + shadowFactor * lightColor * (diffuseFactor + specular);
	}

	for (int pointLightIndex = 0; pointLightIndex < 8; ++pointLightIndex)
	{
		if (pointLightIndex >= pointLightCount)
			break;

		vec3 lightPosition = u_PointLightPositionRange[pointLightIndex].xyz;
		float range = u_PointLightPositionRange[pointLightIndex].w;
		vec3 toLight = lightPosition - v_Output.WorldPosition;
		float distanceToLight = length(toLight);
		float attenuation = EvaluatePointLightAttenuation(distanceToLight, range);
		if (attenuation <= 0.0)
			continue;

		vec3 lightDirectionTowardSurface = toLight / max(distanceToLight, 0.0001);
		float diffuseFactor = max(dot(normal, lightDirectionTowardSurface), 0.0);
		vec3 lightColor =
			u_PointLightColorIntensity[pointLightIndex].rgb * u_PointLightColorIntensity[pointLightIndex].a;

		vec3 specular = vec3(0.0);
		if (diffuseFactor > 0.0)
			specular = EvaluateSpecular(normal, lightDirectionTowardSurface, viewDirection,
				v_Output.Specular, v_Output.Shininess);

		accumulatedLight += lightColor * attenuation * (diffuseFactor + specular);
	}

	vec3 litLinear = accumulatedLight * albedo.rgb;
	o_Color = vec4(ApproximateLinearToSrgb(litLinear), albedo.a);
	o_EntityID = v_EntityID;
}
