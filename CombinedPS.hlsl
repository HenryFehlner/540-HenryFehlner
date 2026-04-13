#include "ShaderIncludes.hlsli"

// Constant buffer
cbuffer ExternalData : register(b0)
{
    float2 uvScale;
    float2 uvOffset;
    float4 colorTint;
    float totalTime;
};

// Texture resources
Texture2D SurfaceTexture : register(t0);
Texture2D OverlayTexture : register(t1);
SamplerState BasicSampler : register(s0);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{	
	// Apply UV offset and scale
    float2 transformedUV = (input.uv * uvScale) + uvOffset;
	
	// Sample color from texture and overlay texture
    float4 surfaceColor = SurfaceTexture.Sample(BasicSampler, transformedUV);
    float4 overlayColor = OverlayTexture.Sample(BasicSampler, transformedUV);
	
	// Combine colors from both textures
    float4 combinedColor = surfaceColor;
	if (overlayColor.x && overlayColor.y && overlayColor.z)
    {
        // Multiply blend mode
        combinedColor = surfaceColor * overlayColor;

    }
	
	// Apply color tint
    combinedColor *= colorTint;
	
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
    return combinedColor;
}