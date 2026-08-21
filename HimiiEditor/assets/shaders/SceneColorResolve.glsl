#type vertex
#version 450 core

layout(location = 0) out vec2 v_TextureCoordinate;

void main()
{
	vec2 positions[3] = vec2[](
		vec2(-1.0, -1.0),
		vec2(3.0, -1.0),
		vec2(-1.0, 3.0));
	vec2 textureCoordinates[3] = vec2[](
		vec2(0.0, 0.0),
		vec2(2.0, 0.0),
		vec2(0.0, 2.0));

	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	v_TextureCoordinate = textureCoordinates[gl_VertexIndex];
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 v_TextureCoordinate;

layout(std140, binding = 5) uniform SceneColorResolveUniforms
{
	vec4 u_ExposureParameters; // x = Exposure
};

layout(binding = 0) uniform sampler2D u_HdrColorTexture;

// Narkowicz 2015 ACES fitted approximation (common real-time filmic curve).
vec3 ApplyAcesFilmicToneMap(vec3 color)
{
	const float exposureBias = 2.51;
	const float whiteScaleA = 2.43;
	const float whiteScaleB = 0.59;
	const float whiteScaleC = 0.14;
	return clamp(
		(color * (exposureBias * color + vec3(0.03))) /
			(color * (whiteScaleA * color + vec3(whiteScaleB)) + vec3(whiteScaleC)),
		0.0,
		1.0);
}

vec3 ApproximateLinearToSrgb(vec3 color)
{
	return pow(color, vec3(1.0 / 2.2));
}

void main()
{
	vec4 hdrSample = texture(u_HdrColorTexture, v_TextureCoordinate);
	float safeExposure = max(u_ExposureParameters.x, 0.001);
	vec3 exposedLinear = hdrSample.rgb * safeExposure;
	vec3 toneMapped = ApplyAcesFilmicToneMap(exposedLinear);
	o_Color = vec4(ApproximateLinearToSrgb(toneMapped), hdrSample.a);
}
