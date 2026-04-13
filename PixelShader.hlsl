#include "ShaderIncludes.hlsli"

// Constant buffer
cbuffer ExternalData : register(b0)
{
    float2 uvScale;
    float2 uvOffset;
    float4 colorTint;
    float3 cameraPosition;
    float totalTime;
    float3 ambientColor;
    Light lights[5];
};

// Texture resources
Texture2D SurfaceTexture  : register(t0);
Texture2D NormalMap       : register(t1);
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
    // Normalize normal, rasterizer interpolation results in non-unit vectors
    input.normal = normalize(input.normal);
    
    // Apply UV offset and scale
    float2 transformedUV = (input.uv * uvScale) + uvOffset;
    
    // Normal mapping
    {
        // Unpack normal from texture
        float3 sampledNormal = NormalMap.Sample(BasicSampler, transformedUV).xyz;
        float3 unpackedNormal = normalize(sampledNormal * 2.0 - 1.0);
        
        // Create TBN matrix
        float3 N = input.normal;
        float3 T = normalize(input.tangent - dot(input.tangent, N) * N); // Orthonormalize here
        float3 B = cross(T, N);
        float3x3 TBN = float3x3(T, B, N);
        
        // Transform normal from map
        input.normal = mul(unpackedNormal, TBN);
    }
	
    // Get surface color from texture
    float4 surfaceColor = SurfaceTexture.Sample(BasicSampler, transformedUV);
    
    // Lighting operations
    float3 sumOfLights = float3(0, 0, 0);
    for (int i = 0; i < 5; ++i)
    {
        // Get current light
        Light currentLight = lights[i];
        
        // Calculate contribution based on light type
        switch (currentLight.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                sumOfLights += CalcDirectionalLight(currentLight, input, surfaceColor, cameraPosition);
                break;
            case LIGHT_TYPE_POINT:
                sumOfLights += CalcPointLight(currentLight, input, surfaceColor, cameraPosition);
                break;
            case LIGHT_TYPE_SPOT:
                sumOfLights += CalcSpotLight(currentLight, input, surfaceColor, cameraPosition);
                break;
        }
    }
	
	// Combine ambient, diffuse, and specular terms and apply color tint
    float4 finalColor = surfaceColor * float4(sumOfLights + ambientColor, 1.0) * colorTint;
	
	// Return final color
    return finalColor;
}