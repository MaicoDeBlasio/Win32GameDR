// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561

#pragma once

//static const float PI = 3.14159265f;
static const float EPSILON = 1e-6f;

static const uint  NumRadianceMipLevels = 11; // Added this.

// Shlick's approximation of Fresnel
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
float3 Fresnel_Shlick(in float3 f0, in float3 f90, in float x)
{
    return f0 + (f90 - f0) * pow(1 - x, 5);
}

// Burley B. "Physically Based Shading at Disney"
// SIGGRAPH 2012 Course: Practical Physically Based Shading in Film and Game Production, 2012.
float Diffuse_Burley(in float NdotL, in float NdotV, in float LdotH, in float roughness)
{
    float fd90 = 0.5f + 2 * roughness * LdotH * LdotH;
    return Fresnel_Shlick(1, fd90, NdotL).x * Fresnel_Shlick(1, fd90, NdotV).x;
}

// GGX specular D (normal distribution)
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float Specular_D_GGX(in float alpha, in float NdotH)
{
    const float alpha2 = alpha * alpha;
    const float lower = (NdotH * NdotH * (alpha2 - 1)) + 1;
    return alpha2 / max(EPSILON, PI * lower * lower);
}

// Schlick-Smith specular G (visibility) with Hable's LdotH optimization
// http://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
// http://graphicrants.blogspot.se/2013/08/specular-brdf-reference.html
float G_Shlick_Smith_Hable(float alpha, float LdotH)
{
    return rcp(lerp(LdotH * LdotH, 1, alpha * alpha * 0.25f));
}

// A microfacet based BRDF.
//
// alpha:           This is roughness * roughness as in the "Disney" PBR model by Burley et al.
//
// specularColor:   The F0 reflectance value - 0.04 for non-metals, or RGB for metals. This follows model 
//                  used by Unreal Engine 4.
//
// NdotV, NdotL, LdotH, NdotH: vector relationships between,
//      N - surface normal
//      V - eye normal
//      L - light normal
//      H - half vector between L & V.
float3 Specular_BRDF(in float alpha, in float3 specularColor, in float NdotV, in float NdotL, in float LdotH, in float NdotH)
{
    // Specular D (microfacet normal distribution) component
    float specular_D = Specular_D_GGX(alpha, NdotH);

    // Specular Fresnel
    float3 specular_F = Fresnel_Shlick(specularColor, 1, LdotH);

    // Specular G (visibility) component
    float specular_G = G_Shlick_Smith_Hable(alpha, LdotH);

    return specular_D * specular_F * specular_G;
}

/*
// Diffuse irradiance
float3 Diffuse_IBL(in float3 N)
{
    return IrradianceTexture.Sample(IBLSampler, N);
}

// Approximate specular image based lighting by sampling radiance map at lower mips 
// according to roughness, then modulating by Fresnel term. 
float3 Specular_IBL(in float3 N, in float3 V, in float lodBias)
{
    float mip = lodBias * NumRadianceMipLevels;
    float3 dir = reflect(-V, N);
    return RadianceTexture.SampleLevel(IBLSampler, dir, mip);
}
*/

// Apply Disney-style physically based rendering to a surface with:
//
// V, N:             Eye and surface normals
//
// numLights:        Number of directional lights.
//
// lightColor[]:     Color and intensity of directional light.
//
// lightDirection[]: Light direction.
float3 LightSurface(
    in float3 V, in float3 N, in float3 L,
    in uint numLights, in float3 lightColor, //in float3 lightDirection,
    //in int numLights, in float3 lightColor[3], in float3 lightDirection[3],
    in float3 albedo, in float roughness, in float metallic, in float ambientOcclusion,
    // Added these parameters.
    in float  specularMask, in float3 ambientLight, in float3 reflectColor, in float shadowMask //in bool isInShadow
    //in TextureCube<float3> DiffIBLTex,
    //in TextureCube<float3> SpecIBLTex
)
{
    // Specular coefficiant - fixed reflectance value for non-metals.
    static const float kSpecularCoefficient = 0.04f;

    const float NdotV = saturate(dot(N, V));

    // Burley roughness bias.
    const float alpha = roughness * roughness;

    // Blend base colors.
    const float3 c_diff = lerp(albedo, float3(0, 0, 0), metallic)      * ambientOcclusion;
    const float3 c_spec = lerp(kSpecularCoefficient, albedo, metallic) * ambientOcclusion;

    // Output color.
    float3 acc_color = 0;

    // Accumulate light values.
    for (uint i = 0; i < numLights; i++)
    {
        // Light vector (to light).
        //const float3 L = -lightDirection;
        //const float3 L = normalize(-lightDirection[i]);

        // Half vector.
        const float3 H = normalize(L + V);

        // Products.
        const float NdotL = saturate(dot(N, L));
        const float LdotH = saturate(dot(L, H));
        const float NdotH = saturate(dot(N, H));

        // Diffuse & specular factors.
        float diffuse_factor = Diffuse_Burley(NdotL, NdotV, LdotH, roughness);

        float3 specular = Specular_BRDF(alpha, c_spec, NdotV, NdotL, LdotH, NdotH) * specularMask;

        //float shadowFactor = isInShadow ? 0 : 1;
        //float shadowFactor = isInShadow ? InShadowRadiance : 1;

        // Directional light.
        acc_color += NdotL * lightColor * shadowMask * (c_diff * diffuse_factor + specular); // Modified for a single light only.
        //acc_color += NdotL * lightColor * shadowFactor * (c_diff * diffuse_factor + specular); // Modified for a single light only.
        //acc_color += NdotL * lightColor * shadowFactor * (c_diff * diffuse_factor * reflectColor + specular);
        //acc_color += NdotL * lightColor[i] * (((c_diff * diffuse_factor) + specular));
    }

    // IBL replaces the constant ambient lighting term.
    // Add diffuse irradiance.
    //float3 diffuse_env_hdr_srgb = DiffIBLTex.SampleLevel(LinearClamp, N, 0);
    //float3 diffuse_env_hdr = RemoveSRGBCurve(diffuse_env_hdr_srgb);
    //float3 diffuse_env = Diffuse_IBL(N);

    acc_color += c_diff * ambientLight;
    //acc_color += c_diff;
    //acc_color += c_diff * float3(0.25f, 0.25f, 0.25f); // diffuse_env_hdr;
    acc_color += c_spec * reflectColor * shadowMask;

    ///*
    //if (!isInShadow)
    //{
        // Add specular radiance.
        // Approximate specular image based lighting by sampling radiance map at lower mips according to roughness,
        // then modulating by Fresnel term. 
        //float  mip = roughness * NumRadianceMipLevels;
        //float3 dir = reflect(-V, N);
        //float3 specular_env_hdr_srgb = SpecIBLTex.SampleLevel(LinearClamp, dir, mip);
        //float3 specular_env_hdr = RemoveSRGBCurve(specular_env_hdr_srgb);
        //float3 specular_env = Specular_IBL(N, V, roughness);
        // acc_color += c_spec * reflectColor;
        //acc_color += c_spec * specular_env_hdr;
    //}
    //*/

    return acc_color;
}
