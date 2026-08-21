#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TextureCoordinate;
layout(location = 3) in vec4 a_Tangent;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_CameraPosition;
};

layout(std140, binding = 3) uniform MeshLitUniforms
{
	mat4 u_Transform;
	vec4 u_AlbedoColor;
	float u_Metallic;
	float u_Roughness;
	int u_UseAlbedoTexture;
	int u_UseMetallicTexture;
	int u_UseRoughnessTexture;
	int u_SharedMetallicRoughnessTexture;
	int u_UseNormalTexture;
	int u_NormalFlipGreen;
	int u_EntityID;
};

layout (location = 0) out vec2 v_TextureCoordinate;
layout (location = 1) out vec3 v_Normal;
layout (location = 2) out vec3 v_WorldPosition;
layout (location = 3) out vec3 v_Tangent;
layout (location = 4) out vec3 v_Bitangent;

void main()
{
	v_TextureCoordinate = a_TextureCoordinate;
	vec4 worldPosition = u_Transform * vec4(a_Position, 1.0);
	v_WorldPosition = worldPosition.xyz;
	mat3 normalMatrix = transpose(inverse(mat3(u_Transform)));
	vec3 normal = normalize(normalMatrix * a_Normal);
	vec3 tangent = normalize(mat3(u_Transform) * a_Tangent.xyz);
	tangent = normalize(tangent - normal * dot(normal, tangent));
	vec3 bitangent = cross(normal, tangent) * a_Tangent.w;
	v_Normal = normal;
	v_Tangent = tangent;
	v_Bitangent = bitangent;
	gl_Position = u_ViewProjection * worldPosition;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

layout (location = 0) in vec2 v_TextureCoordinate;
layout (location = 1) in vec3 v_Normal;
layout (location = 2) in vec3 v_WorldPosition;
layout (location = 3) in vec3 v_Tangent;
layout (location = 4) in vec3 v_Bitangent;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_CameraPosition;
};

layout(std140, binding = 3) uniform MeshLitUniforms
{
	mat4 u_Transform;
	vec4 u_AlbedoColor;
	float u_Metallic;
	float u_Roughness;
	int u_UseAlbedoTexture;
	int u_UseMetallicTexture;
	int u_UseRoughnessTexture;
	int u_SharedMetallicRoughnessTexture;
	int u_UseNormalTexture;
	int u_NormalFlipGreen;
	int u_EntityID;
};

layout(std140, binding = 4) uniform SceneLighting
{
	vec4 u_DirectionalLightDirectionIntensity; // xyz = travel direction, w = hasLight
	vec4 u_DirectionalLightColor;
	vec4 u_AmbientColorIntensity;
	mat4 u_LightViewProjection[4];
	vec4 u_ShadowParameters; // x = hasShadowMap, y = depthBias, z = atlas texel UV size, w = cascade near
	vec4 u_CascadeSplitDistances; // xyz = interior splits, w = shadow far
	vec4 u_ShadowTexelWorldSize; // per-cascade world texel size
	vec4 u_ShadowViewerForwardAndOverlap; // xyz = viewer forward, w = overlap ratio
	vec4 u_PointLightCount; // x = count
	vec4 u_PointLightPositionRange[8]; // xyz = position, w = range
	vec4 u_PointLightColorIntensity[8]; // rgb = color, a = intensity
	vec4 u_ImageBasedLightingParameters; // x = hasIBL, y = intensity, z = prefilterMipCount
};

layout(binding = 0) uniform sampler2D u_AlbedoTexture;
layout(binding = 1) uniform sampler2D u_MetallicTexture;
layout(binding = 2) uniform sampler2D u_RoughnessTexture;
layout(binding = 3) uniform sampler2D u_NormalTexture;
layout(binding = 4) uniform samplerCube u_IrradianceMap;
layout(binding = 5) uniform samplerCube u_PrefilterMap;
layout(binding = 6) uniform sampler2D u_BrdfLookupTexture;
layout(binding = 31) uniform sampler2DShadow u_ShadowMap;

const float PI = 3.14159265358979323846;

vec3 ApproximateSrgbToLinear(vec3 color)
{
	return pow(color, vec3(2.2));
}

vec3 ApproximateLinearToSrgb(vec3 color)
{
	return pow(color, vec3(1.0 / 2.2));
}

vec2 ComputeCascadeAtlasCoordinates(vec2 projectedCoordinates, int cascadeIndex)
{
	float paddingUv = u_ShadowParameters.z * 2.0;
	float innerSize = 0.5 - 2.0 * paddingUv;
	vec2 tileOrigin = vec2(float(cascadeIndex % 2), float(cascadeIndex / 2)) * 0.5;
	return tileOrigin + vec2(paddingUv) + projectedCoordinates * innerSize;
}

int SelectDirectionalShadowCascade(float viewDistance)
{
	if (viewDistance < u_CascadeSplitDistances.x)
		return 0;
	if (viewDistance < u_CascadeSplitDistances.y)
		return 1;
	if (viewDistance < u_CascadeSplitDistances.z)
		return 2;
	return 3;
}

float SampleDirectionalShadowCascade(vec3 worldPosition, vec3 surfaceNormal, float surfaceSlope, int cascadeIndex)
{
	float texelWorldSize = 0.0;
	mat4 lightViewProjection = u_LightViewProjection[0];
	if (cascadeIndex == 0)
	{
		texelWorldSize = u_ShadowTexelWorldSize.x;
		lightViewProjection = u_LightViewProjection[0];
	}
	else if (cascadeIndex == 1)
	{
		texelWorldSize = u_ShadowTexelWorldSize.y;
		lightViewProjection = u_LightViewProjection[1];
	}
	else if (cascadeIndex == 2)
	{
		texelWorldSize = u_ShadowTexelWorldSize.z;
		lightViewProjection = u_LightViewProjection[2];
	}
	else
	{
		texelWorldSize = u_ShadowTexelWorldSize.w;
		lightViewProjection = u_LightViewProjection[3];
	}

	float normalOffsetWorld = texelWorldSize * 1.5 * (1.0 + surfaceSlope * 2.0);
	vec4 lightSpacePosition =
		lightViewProjection * vec4(worldPosition + surfaceNormal * normalOffsetWorld, 1.0);
	vec3 projectedCoordinates = lightSpacePosition.xyz / lightSpacePosition.w;
	projectedCoordinates = projectedCoordinates * 0.5 + 0.5;
	if (projectedCoordinates.z > 1.0
		|| projectedCoordinates.x < 0.0 || projectedCoordinates.x > 1.0
		|| projectedCoordinates.y < 0.0 || projectedCoordinates.y > 1.0)
	{
		return 1.0;
	}

	vec2 atlasCoordinates = ComputeCascadeAtlasCoordinates(projectedCoordinates.xy, cascadeIndex);
	float depthBias = u_ShadowParameters.y * (1.0 + surfaceSlope * 2.0);
	return texture(u_ShadowMap, vec3(atlasCoordinates, projectedCoordinates.z - depthBias));
}

float SampleDirectionalShadow(vec3 worldPosition, vec3 normal, vec3 lightDirectionTowardSurface)
{
	if (u_ShadowParameters.x < 0.5)
		return 1.0;

	vec3 surfaceNormal = normalize(normal);
	float surfaceSlope = clamp(1.0 - max(dot(surfaceNormal, lightDirectionTowardSurface), 0.0), 0.0, 1.0);
	vec3 viewerForward = normalize(u_ShadowViewerForwardAndOverlap.xyz);
	float viewDistance = dot(worldPosition - u_CameraPosition.xyz, viewerForward);
	if (viewDistance > u_CascadeSplitDistances.w)
		return 1.0;

	int cascadeIndex = SelectDirectionalShadowCascade(viewDistance);
	float currentSample = SampleDirectionalShadowCascade(worldPosition, surfaceNormal, surfaceSlope, cascadeIndex);
	if (cascadeIndex >= 3)
		return currentSample;

	float cascadeNear = u_ShadowParameters.w;
	float cascadeFar = u_CascadeSplitDistances.x;
	if (cascadeIndex == 1)
	{
		cascadeNear = u_CascadeSplitDistances.x;
		cascadeFar = u_CascadeSplitDistances.y;
	}
	else if (cascadeIndex == 2)
	{
		cascadeNear = u_CascadeSplitDistances.y;
		cascadeFar = u_CascadeSplitDistances.z;
	}

	float overlapRatio = u_ShadowViewerForwardAndOverlap.w;
	float blendWidth = max((cascadeFar - cascadeNear) * overlapRatio, 0.0001);
	float blendFactor = clamp((cascadeFar - viewDistance) / blendWidth, 0.0, 1.0);
	if (blendFactor >= 1.0)
		return currentSample;

	float nextSample = SampleDirectionalShadowCascade(worldPosition, surfaceNormal, surfaceSlope, cascadeIndex + 1);
	return mix(nextSample, currentSample, blendFactor);
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

float DistributionGGX(vec3 normal, vec3 halfwayDirection, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSquared = alpha * alpha;
	float normalDotHalfway = max(dot(normal, halfwayDirection), 0.0);
	float normalDotHalfwaySquared = normalDotHalfway * normalDotHalfway;
	float denominator = normalDotHalfwaySquared * (alphaSquared - 1.0) + 1.0;
	denominator = PI * denominator * denominator;
	return alphaSquared / max(denominator, 0.0000001);
}

float GeometrySchlickGGX(float normalDotDirection, float roughness)
{
	float roughnessPlusOne = roughness + 1.0;
	float k = (roughnessPlusOne * roughnessPlusOne) / 8.0;
	return normalDotDirection / (normalDotDirection * (1.0 - k) + k);
}

float GeometrySmith(vec3 normal, vec3 viewDirection, vec3 lightDirection, float roughness)
{
	float normalDotView = max(dot(normal, viewDirection), 0.0);
	float normalDotLight = max(dot(normal, lightDirection), 0.0);
	return GeometrySchlickGGX(normalDotView, roughness) * GeometrySchlickGGX(normalDotLight, roughness);
}

vec3 FresnelSchlick(float cosineTheta, vec3 reflectanceAtZeroIncidence)
{
	return reflectanceAtZeroIncidence + (1.0 - reflectanceAtZeroIncidence)
		* pow(clamp(1.0 - cosineTheta, 0.0, 1.0), 5.0);
}

float ResolveMetallicValue(vec2 textureCoordinate)
{
	float metallic = u_Metallic;
	if (u_UseMetallicTexture != 0)
	{
		vec4 sampled = texture(u_MetallicTexture, textureCoordinate);
		float textureMetallic = u_SharedMetallicRoughnessTexture != 0 ? sampled.b : sampled.r;
		metallic *= textureMetallic;
	}
	return clamp(metallic, 0.0, 1.0);
}

float ResolveRoughnessValue(vec2 textureCoordinate)
{
	float roughness = u_Roughness;
	if (u_UseRoughnessTexture != 0)
	{
		vec4 sampled = texture(u_RoughnessTexture, textureCoordinate);
		float textureRoughness = u_SharedMetallicRoughnessTexture != 0 ? sampled.g : sampled.r;
		roughness *= textureRoughness;
	}
	return clamp(roughness, 0.04, 1.0);
}

vec3 ResolveWorldNormal(vec2 textureCoordinate)
{
	vec3 geometricNormal = normalize(v_Normal);
	if (u_UseNormalTexture == 0)
		return geometricNormal;

	vec3 tangentNormal = texture(u_NormalTexture, textureCoordinate).xyz * 2.0 - 1.0;
	if (u_NormalFlipGreen != 0)
		tangentNormal.y = -tangentNormal.y;

	mat3 tangentToWorld = mat3(normalize(v_Tangent), normalize(v_Bitangent), geometricNormal);
	return normalize(tangentToWorld * tangentNormal);
}

vec3 EvaluateDirectPbrLight(vec3 albedoLinear, float metallic, float roughness, vec3 normal,
	vec3 viewDirection, vec3 lightDirectionTowardSurface, vec3 lightColor)
{
	float normalDotLight = max(dot(normal, lightDirectionTowardSurface), 0.0);
	float normalDotView = max(dot(normal, viewDirection), 0.0);
	if (normalDotLight <= 0.0 || normalDotView <= 0.0)
		return vec3(0.0);

	vec3 halfwayDirection = normalize(viewDirection + lightDirectionTowardSurface);
	vec3 reflectanceAtZeroIncidence = mix(vec3(0.04), albedoLinear, metallic);

	float normalDistribution = DistributionGGX(normal, halfwayDirection, roughness);
	float geometry = GeometrySmith(normal, viewDirection, lightDirectionTowardSurface, roughness);
	vec3 fresnel = FresnelSchlick(max(dot(halfwayDirection, viewDirection), 0.0),
		reflectanceAtZeroIncidence);

	vec3 specularNumerator = normalDistribution * geometry * fresnel;
	float specularDenominator = 4.0 * normalDotView * normalDotLight + 0.0001;
	vec3 specular = specularNumerator / specularDenominator;

	vec3 diffuseCoefficient = (vec3(1.0) - fresnel) * (1.0 - metallic);
	vec3 diffuse = diffuseCoefficient * albedoLinear / PI;

	return (diffuse + specular) * lightColor * normalDotLight;
}

vec3 FresnelSchlickRoughness(float cosineTheta, vec3 reflectanceAtZeroIncidence, float roughness)
{
	return reflectanceAtZeroIncidence
		+ (max(vec3(1.0 - roughness), reflectanceAtZeroIncidence) - reflectanceAtZeroIncidence)
		* pow(clamp(1.0 - cosineTheta, 0.0, 1.0), 5.0);
}

vec3 EvaluateImageBasedLighting(vec3 albedoLinear, float metallic, float roughness, vec3 normal,
	vec3 viewDirection)
{
	if (u_ImageBasedLightingParameters.x < 0.5)
		return vec3(0.0);

	float intensity = u_ImageBasedLightingParameters.y;
	float prefilterMipCount = max(u_ImageBasedLightingParameters.z, 1.0);
	vec3 reflectanceAtZeroIncidence = mix(vec3(0.04), albedoLinear, metallic);
	float normalDotView = max(dot(normal, viewDirection), 0.0);
	vec3 fresnel = FresnelSchlickRoughness(normalDotView, reflectanceAtZeroIncidence, roughness);
	vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
	vec3 diffuse = (vec3(1.0) - fresnel) * (1.0 - metallic) * irradiance * albedoLinear;

	vec3 reflectionDirection = reflect(-viewDirection, normal);
	float mipLevel = roughness * (prefilterMipCount - 1.0);
	vec3 prefilteredColor = textureLod(u_PrefilterMap, reflectionDirection, mipLevel).rgb;
	vec2 brdf = texture(u_BrdfLookupTexture, vec2(normalDotView, roughness)).rg;
	vec3 specular = prefilteredColor * (fresnel * brdf.x + brdf.y);

	return (diffuse + specular) * intensity;
}

vec3 ApplyCheapToneMap(vec3 color)
{
	return color / (color + vec3(1.0));
}

void main()
{
	vec4 albedo = u_AlbedoColor;
	if (u_UseAlbedoTexture != 0)
	{
		vec4 sampled = texture(u_AlbedoTexture, v_TextureCoordinate);
		sampled.rgb = ApproximateSrgbToLinear(sampled.rgb);
		albedo *= sampled;
	}

	vec3 albedoLinear = albedo.rgb;
	float metallic = ResolveMetallicValue(v_TextureCoordinate);
	float roughness = ResolveRoughnessValue(v_TextureCoordinate);

	float hasDirectionalLight = u_DirectionalLightDirectionIntensity.w;
	int pointLightCount = int(u_PointLightCount.x + 0.5);
	float hasImageBasedLighting = u_ImageBasedLightingParameters.x;
	if (hasDirectionalLight < 0.5 && pointLightCount <= 0 && hasImageBasedLighting < 0.5)
	{
		o_Color = vec4(0.0, 0.0, 0.0, albedo.a);
		o_EntityID = u_EntityID;
		return;
	}

	vec3 normal = ResolveWorldNormal(v_TextureCoordinate);
	vec3 viewDirection = normalize(u_CameraPosition.xyz - v_WorldPosition);
	vec3 accumulatedLight = vec3(0.0);
	vec3 ambient = u_AmbientColorIntensity.rgb * u_AmbientColorIntensity.a;

	if (hasDirectionalLight >= 0.5)
	{
		vec3 lightTravelDirection = normalize(u_DirectionalLightDirectionIntensity.xyz);
		vec3 lightDirectionTowardSurface = -lightTravelDirection;
		vec3 lightColor = u_DirectionalLightColor.rgb * u_DirectionalLightColor.a;
		float shadowFactor = SampleDirectionalShadow(v_WorldPosition, normal, lightDirectionTowardSurface);
		vec3 directLight = EvaluateDirectPbrLight(albedoLinear, metallic, roughness, normal,
			viewDirection, lightDirectionTowardSurface, lightColor);
		accumulatedLight += ambient * albedoLinear + shadowFactor * directLight;
	}
	else
	{
		accumulatedLight += ambient * albedoLinear;
	}

	for (int pointLightIndex = 0; pointLightIndex < 8; ++pointLightIndex)
	{
		if (pointLightIndex >= pointLightCount)
			break;

		vec3 lightPosition = u_PointLightPositionRange[pointLightIndex].xyz;
		float range = u_PointLightPositionRange[pointLightIndex].w;
		vec3 toLight = lightPosition - v_WorldPosition;
		float distanceToLight = length(toLight);
		float attenuation = EvaluatePointLightAttenuation(distanceToLight, range);
		if (attenuation <= 0.0)
			continue;

		vec3 lightDirectionTowardSurface = toLight / max(distanceToLight, 0.0001);
		vec3 lightColor =
			u_PointLightColorIntensity[pointLightIndex].rgb * u_PointLightColorIntensity[pointLightIndex].a;
		vec3 directLight = EvaluateDirectPbrLight(albedoLinear, metallic, roughness, normal,
			viewDirection, lightDirectionTowardSurface, lightColor);
		accumulatedLight += directLight * attenuation;
	}

	accumulatedLight += EvaluateImageBasedLighting(albedoLinear, metallic, roughness, normal, viewDirection);
	accumulatedLight = ApplyCheapToneMap(accumulatedLight);
	o_Color = vec4(ApproximateLinearToSrgb(accumulatedLight), albedo.a);
	o_EntityID = u_EntityID;
}
