#include "ShaderIncludes.hlsli"

TextureCube SkyTexture		: register(t0);
SamplerState BasicSampler	: register(s0);

float4 main(VertexToPixelSky input) : SV_TARGET
{
    return SkyTexture.Sample(BasicSampler, input.sampleDir);

}