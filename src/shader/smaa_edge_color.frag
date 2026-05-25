#version 450

layout(set = 0, binding = 0) uniform sampler2D colorImg;

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 textureCoord;
layout(location = 1) in vec4[3] offsets;

#include "smaa_settings.h"
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include <SMAA.hlsl>

void main()
{
    fragColor = vec4(SMAAColorEdgeDetectionPS(textureCoord, offsets, colorImg), 0.0, 0.0);
}

