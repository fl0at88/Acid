#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UboScene
{
	mat4 projection;
	mat4 view;
	vec3 cameraPos;
} scene;

layout(set = 0, binding = 1) uniform UboObject
{
#if ANIMATED
	mat4 jointTransforms[MAX_JOINTS];
#endif
	mat4 transform;

	vec4 baseDiffuse;
	float metallic;
	float roughness;
	float ignoreFog;
	float ignoreLighting;
} object;

layout(set = 0, location = 0) in vec3 inPosition;
layout(set = 0, location = 1) in vec2 inUv;
layout(set = 0, location = 2) in vec3 inNormal;
#if NORMAL_MAPPING
layout(set = 0, location = 3) in vec3 inTangent;
#endif
#if ANIMATED
layout(set = 0, location = 4) in vec3 inJointIds;
layout(set = 0, location = 5) in vec3 inWeights;
#endif

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;
#if NORMAL_MAPPING
layout(location = 2) out vec3 outTangent;
#endif

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
#if ANIMATED
	vec4 position = vec4(0.0f);
	vec4 normal = vec4(0.0f);

	for (int i = 0; i < MAX_WEIGHTS; i++)
	{
		mat4 jointTransform = object.jointTransforms[int(inJointIds[i])];
		vec4 posePosition = jointTransform * vec4(inPosition, 1.0f);
		position += posePosition * inWeights[i];

		vec4 worldNormal = jointTransform * vec4(inNormal, 0.0f);
		normal += worldNormal * inWeights[i];
	}
#else
	vec4 position = vec4(inPosition, 1.0f);
	vec4 normal = vec4(inNormal, 0.0f);
#endif

	vec4 worldPosition = object.transform * position;

	gl_Position = scene.projection * scene.view * worldPosition;

	outUv = inUv;
	outNormal = normal.xyz;

#if NORMAL_MAPPING
	mat3 matrixNormal = transpose(inverse(mat3(object.transform)));
	outNormal = matrixNormal * normalize(inNormal);
	outTangent = matrixNormal * normalize(inTangent);
#endif
}
