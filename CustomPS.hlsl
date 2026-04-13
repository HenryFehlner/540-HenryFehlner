#include "ShaderIncludes.hlsli"

// Constant buffer
cbuffer ExternalData : register(b0)
{
    float4 colorTint;
    float totalTime;
};

// The entry point (main method) for our pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
    // I kind of lost track of whats happening here but the colors oscillate 
    // with sin and cos and also the screen position are doing something
    return float4(
        cos(totalTime + 1.0) / 2.0,
        (sin(totalTime) + 1.0) / 2.0 * (input.uv.y / (input.screenPosition.y / 720)),
        (cos(totalTime) + 1.0) / 2.0 * (input.uv.x / (input.screenPosition.x / 1280)),
        1.0);
}