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
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
    return float4(input.uv, 0.0, 1.0);
}