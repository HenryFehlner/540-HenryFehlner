#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};

VertexToPixelSky main(VertexShaderInput input)
{
    // Set up output struct
    VertexToPixelSky output;
    
    // Create view matrix with no translation
    matrix viewNoTranslation = viewMatrix;
    viewNoTranslation._14 = 0;
    viewNoTranslation._24 = 0;
    viewNoTranslation._34 = 0;
    
    // Apply view and proj to input position
    matrix vp = mul(projectionMatrix, viewNoTranslation);
    output.screenPosition = mul(vp, float4(input.localPosition, 1.0));
    
    // Set output depth to 1.0
    output.screenPosition.z = output.screenPosition.w;
    
    // Sample direction
    output.sampleDir = input.localPosition;
    
    // Return struct
    return output;
};