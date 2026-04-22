#include "ShaderIncludes.hlsli"

// Constant buffer
cbuffer ExternalData : register(b0)
{
    matrix worldMatrix;	// matrix is always 4x4, or you can do float4x4
    matrix worldInverseTransposeMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix;
    matrix lightProjectionMatrix;
};

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel main(VertexShaderInput input)
{
	// Set up output struct
	VertexToPixel output;

	// Calculate screen position from local position and world view proj matrix
    matrix wvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0));	// Ordered like this because of row/col major differences
    
    // Calculate world position
    output.worldPosition = mul(worldMatrix, float4(input.localPosition, 1.0)).xyz;

	// Pass the vertex data through the pipeline
    output.uv = input.uv;
	
	// Transform normal
    output.normal = mul((float3x3) worldInverseTransposeMatrix, input.normal);
    output.normal = normalize(output.normal);
    
    // Transform tangent
    output.tangent = mul((float3x3) worldMatrix, input.tangent);
    output.tangent = normalize(output.tangent);
    
    // Shadow map calculation
    matrix shadowWVP = mul(lightProjectionMatrix, mul(lightViewMatrix, worldMatrix));
    output.shadowMapPos = mul(shadowWVP, float4(input.localPosition, 1.0));

	// Return struct
	return output;
}