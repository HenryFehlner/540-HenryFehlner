#ifndef __GGP_SHADER_INCLUDES__
#define __GGP_SHADER_INCLUDES__

#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2
#define MIN_ROUGHNESS           0.0000001
#define PI                      3.14159265359

// ** STRUCTS **
// Represents a single vertex worth of data
// Used in vertex shaders
struct VertexShaderInput
{
    float3 localPosition : POSITION; // XYZ position
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

// Represents data we recieve from earlier pipeline stages
// Used in pixel shaders
struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION;
    float3 tangent : TANGENT;
    float4 shadowMapPos : SHADOW_POSITION;
};

// Used in sky vertex shader
struct VertexToPixelSky
{
    float4 screenPosition : SV_POSITION;
    float3 sampleDir : DIRECTION;
};

// Used in post process vertex shader
struct VertexToPixelPostProcess
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Light struct
struct Light
{
    int type;
    float3 direction;
    
    float range;
    float3 position;
    
    float intensity;
    float3 color;
    
    float spotInnerAngle;
    float spotOuterAngle;
    float2 padding;
};

// ** LIGHTING FUNCTIONS **
float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.range * light.range)));
    return att * att;
};

float DiffusePBR(float3 normal, float3 dirToLight)
{
    return saturate(dot(normal, dirToLight));
};

// Specular lighting with Phong equation
float3 PhongSpecularTerm(Light light, VertexToPixel input, float3 surfaceColor, float3 cameraPosition, float3 normLightDirection)
{
    float shininess = 128.0;
    float specScale = 0.8; // TODO: get from a specular map
    float3 refl = reflect(normLightDirection, input.normal); // Calculate the reflection of incoming light using surface normal
    float RdotV = saturate(dot(refl, normalize(cameraPosition - input.worldPosition))); // Calculate difference between perfect reflection and camera angle
    float3 specularTerm =
        pow(RdotV, shininess) * specScale *
        light.color * light.intensity *
        surfaceColor;
    
    return specularTerm;
};

// Normal distribution
float D_GGX(float3 n, float3 h, float roughness)
{
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS);
    
    float denomToSquare = NdotH2 * (a2 - 1) + 1;
    
    return a2 / (PI * denomToSquare * denomToSquare);
};

// Fresnel
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
    float VdotH = saturate(dot(v, h));
    
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
};

// Geometric shadowing
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
    float k = pow(roughness + 1, 2) / 8.0;
    float NdotV = saturate(dot(n, v));
    
    return 1 / (NdotV * (1 - k) + k);
};

// Cook-Torrance BRDF for specular lighting
float3 MicrofacetBRDF(float3 normal, float3 halfAngle, float3 dirToLight, float3 dirToCamera, float roughness, float3 specColor)
{
    // Components
    float D = D_GGX(normal, halfAngle, roughness);
    float3 F = F_Schlick(dirToCamera, halfAngle, specColor);
    float G = G_SchlickGGX(normal, dirToCamera, roughness);
    
    float3 specularResult = (D * F * G) / 4;
    //float3 specularResult = (F) / 4;
    
    return specularResult * saturate(dot(normal, dirToLight));
};

float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
};

float3 CalcDirectionalLight(Light light, VertexToPixel input, float3 surfaceColor, float roughness, float metalness, float3 specColor, float3 cameraPosition)
{
    // Normalize light direction
    //float3 normLightDirection = normalize(input.worldPosition - light.position);
    float3 normLightDirection = normalize(light.direction);
    
    // Required terms
    float3 dirToLight = -normLightDirection;
    float3 dirToCamera = normalize(cameraPosition - input.worldPosition);
    float3 halfAngle = normalize(dirToCamera + dirToLight);
    
	// Get diffuse term (Lambert)
    //float3 diffuseTerm =
    //    saturate(dot(input.normal, dirToLight)) *
    //    light.color * light.intensity *
    //    surfaceColor.rgb;
    float diffuseTerm = DiffusePBR(input.normal, dirToLight);
    
    // Get specular term
    //float3 specularTerm = PhongSpecularTerm(light, input, surfaceColor, cameraPosition, normLightDirection);
    float3 specularTerm = MicrofacetBRDF(input.normal, halfAngle, dirToLight, dirToCamera, roughness, specColor);
    
    // Calculate diffuse with conservation
    float3 F = F_Schlick(dirToCamera, halfAngle, specColor);
    float3 balancedDiff = DiffuseEnergyConserve(diffuseTerm, F, metalness);
    
    float3 total = (balancedDiff * surfaceColor + specularTerm) * light.intensity * light.color;
    //float3 total = (balancedDiff * surfaceColor + specularTerm);
    
    return total;
};

float3 CalcPointLight(Light light, VertexToPixel input, float3 surfaceColor, float roughness, float metalness, float3 specColor, float3 cameraPosition)
{
    // Calculate and normalize light direction
    float3 normLightDirection = normalize(input.worldPosition - light.position);
    //float3 normLightDirection = normalize(light.direction);
    
    // Required terms
    float3 dirToLight = -normLightDirection;
    float3 dirToCamera = normalize(cameraPosition - input.worldPosition);
    float3 halfAngle = normalize(dirToCamera + dirToLight);
    
    // Get diffuse term (Lambert)
    float diffuseTerm = DiffusePBR(input.normal, dirToLight);
    
    // Get specular term
    float3 specularTerm = MicrofacetBRDF(input.normal, halfAngle, dirToLight, dirToCamera, roughness, specColor);
    
    // Calculate diffuse with conservation
    float3 F = F_Schlick(dirToCamera, halfAngle, specColor);
    float3 balancedDiff = DiffuseEnergyConserve(diffuseTerm, F, metalness);
    
    float3 total = ((balancedDiff * surfaceColor + specularTerm) * light.intensity * light.color);
    
    return total * Attenuate(light, input.worldPosition);
};

float3 CalcSpotLight(Light light, VertexToPixel input, float3 surfaceColor, float roughness, float metalness, float3 specColor, float3 cameraPosition)
{   
    // Normalize light direction
    float3 normLightDirection = normalize(light.direction);

    // Use spot light function
    float3 finalColor = CalcPointLight(light, input, surfaceColor, roughness, metalness, specColor, cameraPosition);
    
    // Angle between pixel and light
    float3 lightToPixel = normalize(input.worldPosition - light.position);
    
    // Get cos(angle) between pixel and light direction
    float pixelAngle = saturate(dot(lightToPixel, normLightDirection));
    
    // Get cosines of angles and calculate range
    float cosOuter = cos(light.spotOuterAngle);
    float cosInner = cos(light.spotInnerAngle);
    float falloffRange = cosOuter - cosInner;
    
    // Linear falloff over the range, clamp 0-1, apply to light calc
    float spotTerm = saturate((cosOuter - pixelAngle) / falloffRange);
    
    // Return calculated color with cone attenuation
    return finalColor * spotTerm;
};

// ** TEXTURE FUNCTIONS **


#endif