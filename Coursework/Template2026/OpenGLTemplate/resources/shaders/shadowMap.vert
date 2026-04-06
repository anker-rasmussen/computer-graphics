#version 400 core

uniform mat4 lightMVP;

layout (location = 0) in vec3 inPosition;

void main()
{
    gl_Position = lightMVP * vec4(inPosition, 1.0);
}
