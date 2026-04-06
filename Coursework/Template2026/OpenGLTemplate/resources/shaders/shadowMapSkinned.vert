#version 400 core

uniform mat4 lightMVP;

const int MAX_BONES = 128;
uniform mat4 boneMatrices[MAX_BONES];

layout (location = 0) in vec3 inPosition;
layout (location = 3) in ivec4 inBoneIDs;
layout (location = 4) in vec4 inWeights;

void main()
{
    mat4 boneTransform = mat4(0.0);
    float totalWeight = inWeights[0] + inWeights[1] + inWeights[2] + inWeights[3];

    if (totalWeight > 0.001) {
        for (int i = 0; i < 4; i++) {
            int boneID = inBoneIDs[i];
            if (boneID >= 0 && boneID < MAX_BONES && inWeights[i] > 0.0)
                boneTransform += boneMatrices[boneID] * inWeights[i];
        }
    } else {
        boneTransform = mat4(1.0);
    }

    gl_Position = lightMVP * boneTransform * vec4(inPosition, 1.0);
}
