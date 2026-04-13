#ifndef __GGP_SHADER_INCLUDES__
#define __GGP_SHADER_INCLUDES__

#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2

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
};

// Used in sky vertex shader
struct VertexToPixelSky
{
    float4 screenPosition : SV_POSITION;
    float3 sampleDir : DIRECTION;
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

float3 CalcDirectionalLight(Light light, VertexToPixel input, float4 surfaceColor, float3 cameraPosition)
{
    // Normalize light direction
    float3 normLightDirection = normalize(light.direction);
    
	// Get diffuse term
    float3 dirToLight = -normLightDirection;
    float3 diffuseTerm =
        saturate(dot(input.normal, dirToLight)) *
        light.color * light.intensity *
        surfaceColor.rgb;
    
    // Get specular term
    float shininess = 128.0;
    float specScale = 0.8; // TODO: get from a specular map
    float3 refl = reflect(normLightDirection, input.normal); // Calculate the reflection of incoming light using surface normal
    float RdotV = saturate(dot(refl, normalize(cameraPosition - input.worldPosition))); // Calculate difference between perfect reflection and camera angle
    float3 specularTerm =
        pow(RdotV, shininess) * specScale *
        light.color * light.intensity *
        surfaceColor.rgb;
    
    // Cut specular term if diffuse contribution is zero
    specularTerm *= any(diffuseTerm);
    
    return diffuseTerm + specularTerm;
};

float3 CalcPointLight(Light light, VertexToPixel input, float4 surfaceColor, float3 cameraPosition)
{
    // Calculate and normalize light direction
    float3 normLightDirection = normalize(input.worldPosition - light.position);
    
    // Get diffuse term
    float3 dirToLight = -normLightDirection;
    float3 diffuseTerm =
        saturate(dot(input.normal, dirToLight)) *
        light.color * light.intensity *
        surfaceColor.rgb;
    
    // Get specular term
    float shininess = 128.0;
    float specScale = 0.8; // TODO: get from a specular map
    float3 refl = reflect(normLightDirection, input.normal); // Calculate the reflection of incoming light using surface normal
    float RdotV = saturate(dot(refl, normalize(cameraPosition - input.worldPosition))); // Calculate difference between perfect reflection and camera angle
    float3 specularTerm =
        pow(RdotV, shininess) * specScale *
        light.color * light.intensity *
        surfaceColor.rgb;
    
    // Cut specular term if diffuse contribution is zero
    specularTerm *= any(diffuseTerm);
    
    return (diffuseTerm + specularTerm) * Attenuate(light, input.worldPosition);
};

float3 CalcSpotLight(Light light, VertexToPixel input, float4 surfaceColor, float3 cameraPosition)
{   
    // Normalize light direction
    float3 normLightDirection = normalize(light.direction);

    // Use spot light function
    float3 finalColor = CalcPointLight(light, input, surfaceColor, cameraPosition);
    
    // Angle between pixel and light
    float3 lightToPixel = normalize(input.worldPosition - light.position);
    
    // Get cos(angle) between pixel and light direction
    float pixelAngle = saturate(dot(lightToPixel, normLightDirection));
    
    // Get cosines of angles and calculate range
    float cosOuter = cos(light.spotOuterAngle);
    float cosInner = cos(light.spotInnerAngle);
    float falloffRange = cosOuter - cosInner;
    
    // Linear falloff over the range, clamp 0-1, apply to light calc
    float spotTerm =  saturate((cosOuter - pixelAngle) / falloffRange);
    
    // Return calculated color with cone attenuation
    return finalColor * spotTerm;
};

// ** TEXTURE FUNCTIONS **


#endif