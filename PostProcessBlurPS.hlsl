#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0)
{
    int blurRadius;
    float pixelWidth;
    float pixelHeight;
    float aberrationAmount;
}

Texture2D Pixels			: register(t0);
SamplerState ClampSampler	: register(s0);

float4 main(VertexToPixelPostProcess input) : SV_TARGET
{
    // Sample render texture
    float4 sampledColor = Pixels.Sample(ClampSampler, input.uv);
    
    // Apply chromatic aberration
    float r = Pixels.Sample(ClampSampler, input.uv - float2(aberrationAmount, 0)).x;
    float g = Pixels.Sample(ClampSampler, input.uv).y;
    float b = Pixels.Sample(ClampSampler, input.uv - float2(0, aberrationAmount)).z;
    float4 chromAbColor = float4(r, g, b, 1.0);
    
    // Track total color and sample number
    float4 blurColor = 0.0;
    int sampleCount = 0;
    
    // Loop through box of surrounding pixels
    for (int x = -blurRadius; x <= blurRadius; x++)
    {
        for (int y = -blurRadius; y <= blurRadius; y++)
        {
            // Calculate UV value
            float2 uv = input.uv;
            uv += float2(x * pixelWidth, y * pixelHeight);
            
            // Add color to total
            blurColor += Pixels.Sample(ClampSampler, uv);
            sampleCount++;
        }
    }
    
    // Average color
    blurColor /= sampleCount;
    
    return (chromAbColor + blurColor) / 2.0;
}