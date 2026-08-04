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

layout(std140, binding = 3) uniform MeshLitUniforms
{
	mat4 u_Transform;
	vec4 u_AlbedoColor;
	float u_Specular;
	float u_Shininess;
	int u_UseAlbedoTexture;
	int u_EntityID;
};

layout (location = 0) out vec2 v_TextureCoordinate;
layout (location = 1) out vec3 v_Normal;
layout (location = 2) out vec3 v_WorldPosition;

void main()
{
	v_TextureCoordinate = a_TextureCoordinate;
	vec4 worldPosition = u_Transform * vec4(a_Position, 1.0);
	v_WorldPosition = worldPosition.xyz;
	mat3 normalMatrix = transpose(inverse(mat3(u_Transform)));
	v_Normal = normalize(normalMatrix * a_Normal);
	gl_Position = u_ViewProjection * worldPosition;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

layout (location = 0) in vec2 v_TextureCoordinate;
layout (location = 1) in vec3 v_Normal;
layout (location = 2) in vec3 v_WorldPosition;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 u_CameraPosition;
};

layout(std140, binding = 3) uniform MeshLitUniforms
{
	mat4 u_Transform;
	vec4 u_AlbedoColor;
	float u_Specular;
	float u_Shininess;
	int u_UseAlbedoTexture;
	int u_EntityID;
};

layout(std140, binding = 4) uniform SceneLighting
{
	vec4 u_DirectionalLightDirectionIntensity; // xyz = travel direction, w = hasLight
	vec4 u_DirectionalLightColor;
	vec4 u_AmbientColorIntensity;
};

layout(binding = 0) uniform sampler2D u_AlbedoTexture;

vec3 ApproximateSrgbToLinear(vec3 color)
{
	return pow(color, vec3(2.2));
}

vec3 ApproximateLinearToSrgb(vec3 color)
{
	return pow(color, vec3(1.0 / 2.2));
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

	float hasDirectionalLight = u_DirectionalLightDirectionIntensity.w;
	if (hasDirectionalLight < 0.5)
	{
		o_Color = vec4(0.0, 0.0, 0.0, albedo.a);
		o_EntityID = u_EntityID;
		return;
	}

	vec3 normal = normalize(v_Normal);
	vec3 lightTravelDirection = normalize(u_DirectionalLightDirectionIntensity.xyz);
	vec3 lightDirectionTowardSurface = -lightTravelDirection;
	float diffuseFactor = max(dot(normal, lightDirectionTowardSurface), 0.0);

	vec3 lightColor = u_DirectionalLightColor.rgb * u_DirectionalLightColor.a;
	vec3 ambient = u_AmbientColorIntensity.rgb * u_AmbientColorIntensity.a;

	float specularFactor = 0.0;
	if (diffuseFactor > 0.0)
	{
		vec3 viewDirection = normalize(u_CameraPosition.xyz - v_WorldPosition);
		vec3 halfwayDirection = normalize(lightDirectionTowardSurface + viewDirection);
		float shininess = max(u_Shininess, 1.0);
		specularFactor = pow(max(dot(normal, halfwayDirection), 0.0), shininess) * u_Specular;
	}

	vec3 litLinear = (ambient + lightColor * diffuseFactor + lightColor * specularFactor) * albedo.rgb;
	o_Color = vec4(ApproximateLinearToSrgb(litLinear), albedo.a);
	o_EntityID = u_EntityID;
}
