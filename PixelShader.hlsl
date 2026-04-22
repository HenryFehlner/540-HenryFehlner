#include "ShaderIncludes.hlsli"

// Constant buffer
cbuffer ExternalData : register(b0)
{
    float2 uvScale;
    float2 uvOffset;
    float4 colorTint;
    float3 cameraPosition;
    float totalTime;
    Light lights[5];
};

// Texture resources
Texture2D Albedo                        : register(t0);
Texture2D NormalMap                     : register(t1);
Texture2D RoughnessMap                  : register(t2);
Texture2D MetalnessMap                  : register(t3);
Texture2D ShadowMap                     : register(t4);
SamplerState BasicSampler               : register(s0);
SamplerComparisonState ShadowSampler    : register(s1);

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
    // Shadow operations
    float shadowAmount = 0.0;
    {
        // Perspective divide
        input.shadowMapPos /= input.shadowMapPos.w;
        
        // Convert noralized device coords to UVs for sampling
        float2 shadowUV = input.shadowMapPos.xy * 0.5 + 0.5;
        shadowUV.y = 1 - shadowUV.y; // Flip Y
        
        // Get light-to-pixel distance
        float distToLight = input.shadowMapPos.z;
        
        // Sample shadow map for shadow amount
        shadowAmount = ShadowMap.SampleCmpLevelZero(
            ShadowSampler,
            shadowUV,
            distToLight).r;
    }
    
    // Normalize normal, rasterizer interpolation results in non-unit vectors
    input.
        normal = normalize(input.normal);
    
    // Apply UV offset and scale
    float2 transformedUV = (input.uv * uvScale) + uvOffset;
    
    // Get surface color from texture and un-gamma correct
    float4 albedoColor = Albedo.Sample(BasicSampler, transformedUV);
    float4 correctedAlbedoColor = float4(pow(albedoColor.rgb, 2.2), albedoColor.a);
    
    // Get and map normals
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
    
    // Get roughness value
    float roughness = RoughnessMap.Sample(BasicSampler, transformedUV).r;
    
    // Get metalness value
    float metalness = MetalnessMap.Sample(BasicSampler, transformedUV).r;
    
    // Get specular color
    float3 specularColor = lerp(0.04, correctedAlbedoColor.rgb, metalness);
    
    // Lighting operations
    float3 sumOfLights = float3(0, 0, 0);
    for (int i = 0; i < 5; ++i)
    {
        // Get current light
        Light currentLight = lights[i];
        
        // Create value for current light
        float3 lightValue = float3(0, 0, 0);
        
        // Calculate contribution based on light type
        switch (currentLight.type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                lightValue += CalcDirectionalLight(currentLight, input, correctedAlbedoColor.rgb, roughness, metalness, specularColor, cameraPosition);
                break;
            case LIGHT_TYPE_POINT:
                lightValue += CalcPointLight(currentLight, input, correctedAlbedoColor.rgb, roughness, metalness, specularColor, cameraPosition);
                break;
            case LIGHT_TYPE_SPOT:
                lightValue += CalcSpotLight(currentLight, input, correctedAlbedoColor.rgb, roughness, metalness, specularColor, cameraPosition);
                break;
        }
        
        // Apply shadows to the first light
        if (i == 0)
        {
            lightValue *= shadowAmount;
        }
        
        // Add to total lighting
        sumOfLights += lightValue;
    }
	
	// Combine diffuse and specular terms and apply color tint
    float4 finalColor = correctedAlbedoColor * float4(sumOfLights, 1.0) * colorTint;
    
    // Gamma correction (only rgb portion)
    float4 finalGammaColor = float4(pow(finalColor.rgb, 1.0 / 2.2), finalColor.a);
	
	// Return final color
    return finalGammaColor;
}