#version 410 core

// Input from the vertex shader
in vec2 fragTexCoords;
in vec3 fragPosition;
in mat3 TBN;

// Output color
out vec4 fragColor;

// Uniforms for lighting and material
uniform vec3 uLightDirection; // Change from position to direction
uniform vec3 uLightColor;
uniform vec3 uViewPosition;
uniform float uEnvIntensity;
uniform int uRenderMode;

uniform vec4 uDiffuseColor;
uniform float uMetalnessFactor;
uniform float uRoughnessFactor;
uniform int uUseNormalMap;

// Textures
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uMetalnessRoughnessMap;
uniform sampler2D uSkyMap;
uniform sampler2D uBrdfLut;
uniform samplerCube uPrefilterMap;
uniform samplerCube uIrradianceMap;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------
void main()
{       
    vec3 normalMap = texture(uNormalMap, fragTexCoords).rgb;
    vec3 N = normalize(TBN * (normalMap * 2.0 - 1.0));
    vec3 V = normalize(uViewPosition - fragPosition);
    if (uUseNormalMap == 0) N = TBN[2];
    vec3 R = reflect(-V, N);
    float alpha = uDiffuseColor.a;

    vec3 albedo = uDiffuseColor.rgb;
    float metallic = uMetalnessFactor;
    float roughness = uRoughnessFactor;

    // use maps if set
    if (albedo.r > 1) {
        albedo = texture(uAlbedoMap, fragTexCoords).rgb;
        alpha = 1;
    }
    if (metallic > 1) metallic = texture(uMetalnessRoughnessMap, fragTexCoords).b;
    if (roughness > 1) roughness = texture(uMetalnessRoughnessMap, fragTexCoords).g;
    

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    // calculate per-light radiance
    vec3 L = normalize(-uLightDirection);
    vec3 H = normalize(V + L);
    vec3 radiance = uLightColor;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);    
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;
    
     // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;                   
        
    // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);        

    // add to outgoing radiance Lo
    Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again 

    // ambient lighting (we now use IBL as the ambient term)
    F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    kD = 1.0 - F;
    kD *= 1.0 - metallic;     
    
    vec3 irradiance = texture(uIrradianceMap, N).rgb * uEnvIntensity;
    vec3 diffuse = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 6.0;
    vec3 prefilteredColor = textureLod(uPrefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb * uEnvIntensity;    
    vec2 brdf  = texture(uBrdfLut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular);

    vec3 color = Lo + ambient;

    // switch render mode
    if (uRenderMode != 0) {
        if (uRenderMode == 1) color = albedo;
        else if (uRenderMode == 2) color = normalMap;
        else if (uRenderMode == 3) color = vec3(metallic);
        else if (uRenderMode == 4) color = vec3(roughness);
        else if (uRenderMode == 5) color = vec3(F);
        else if (uRenderMode == 6) color = vec3(kD);
        else if (uRenderMode == 7) color = diffuse;
        else if (uRenderMode == 8) color = ambient;
        else if (uRenderMode == 9) color = irradiance;
        else if (uRenderMode == 10) color = prefilteredColor;
        else if (uRenderMode == 11) color = vec3(brdf.x);
        else if (uRenderMode == 12) color = vec3(brdf.y);
        else if (uRenderMode == 13) color = specular;
        else if (uRenderMode == 14) color = Lo;
    }
    
    fragColor = vec4(color, alpha);
}
