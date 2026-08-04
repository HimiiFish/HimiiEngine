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
};

layout(binding = 0) uniform sampler2D u_Textures[32];

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
	vec4 albedo = v_Output.Color;
	if (v_TexIndex > 0.0)
	{
		int textureIndex = int(v_TexIndex);
		vec4 sampled = texture(u_Textures[textureIndex], v_Output.TexCoord);
		sampled.rgb = ApproximateSrgbToLinear(sampled.rgb);
		albedo *= sampled;
	}

	float hasDirectionalLight = u_DirectionalLightDirectionIntensity.w;
	if (hasDirectionalLight < 0.5)
	{
		o_Color = vec4(0.0, 0.0, 0.0, albedo.a);
		o_EntityID = v_EntityID;
		return;
	}

	vec3 normal = normalize(v_Output.Normal);
	vec3 lightTravelDirection = normalize(u_DirectionalLightDirectionIntensity.xyz);
	vec3 lightDirectionTowardSurface = -lightTravelDirection;
	float diffuseFactor = max(dot(normal, lightDirectionTowardSurface), 0.0);

	vec3 lightColor =
		u_DirectionalLightColor.rgb * u_DirectionalLightColor.a;
	vec3 ambient =
		u_AmbientColorIntensity.rgb * u_AmbientColorIntensity.a;

	float specularFactor = 0.0;
	if (diffuseFactor > 0.0)
	{
		vec3 viewDirection = normalize(u_CameraPosition.xyz - v_Output.WorldPosition);
		vec3 halfwayDirection = normalize(lightDirectionTowardSurface + viewDirection);
		float shininess = max(v_Output.Shininess, 1.0);
		specularFactor = pow(max(dot(normal, halfwayDirection), 0.0), shininess) * v_Output.Specular;
	}

	vec3 litLinear = (ambient + lightColor * diffuseFactor + lightColor * specularFactor) * albedo.rgb;
	o_Color = vec4(ApproximateLinearToSrgb(litLinear), albedo.a);
	o_EntityID = v_EntityID;
}
