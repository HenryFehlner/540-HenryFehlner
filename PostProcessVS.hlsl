#include "ShaderIncludes.hlsli"

VertexToPixelPostProcess main(uint id : SV_VertexID)
{
    VertexToPixelPostProcess output;
    
    // Calculate the UV (0, 0) to (2, 2) using ID
    output.uv = float2(
        (id << 1) & 2,
        id & 2);
    
    // Calculate position from UV
    output.position = float4(output.uv, 0, 1);
    output.position.x = output.position.x * 2.0 - 1.0;
    output.position.y = output.position.y * -2.0 + 1.0;
    
    return output;
}