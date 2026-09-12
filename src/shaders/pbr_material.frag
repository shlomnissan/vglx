#version 410 core

#extension GL_GOOGLE_include_directive : enable

#pragma inject_attributes

layout (location = 0) out vec4 o_FragColor;

#ifdef USE_UV
    in vec2 v_TexCoords;
#endif

#ifdef USE_VERTEX_COLOR
    in vec3 v_Color;
#endif

#ifdef USE_INSTANCING
    in vec3 v_InstanceColor;
#endif

#ifdef USE_NORMAL_MAP
    in mat3 v_TBN;
#endif

#ifdef USE_FOG
    in float v_ViewDepth;
#endif

in vec4 v_Position;
in vec3 v_Normal;
in vec3 v_ViewDir;

uniform float u_Opacity;

#ifdef USE_ALPHA_TEST
    uniform float u_AlphaTest;
#endif

layout(std140) uniform ub_Camera {
    mat4 u_Projection;
    mat4 u_View;
};

#include "include/fog.incl.glsl"

struct PBRMaterial {
    vec3 Color;
    float Metallic;
    float Roughness;
};

uniform PBRMaterial u_Material;

uniform vec3 u_AmbientLight;
uniform vec3 u_EmissiveColor;
uniform float u_AOIntensity;
uniform float u_EmissiveIntensity;
uniform float u_NormalIntensity;

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_AlphaMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_EmissiveMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;

#ifdef USE_IBL
    uniform samplerCube u_IrradianceMap;
    uniform samplerCube u_PrefilteredMap;
    uniform sampler2D u_BrdfLut;
    uniform float u_PrefilteredMaxLod;
    uniform float u_EnvironmentIntensity;
#endif

const float PI = 3.14159265358979;

float distributionGGX(float NoH, float alpha) {
    float a2 = alpha * alpha;
    float denom = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Height-correlated Smith G2 with the 1/(4 NoL NoV) divisor folded in.
float visibilityGGX(float NoL, float NoV, float alpha) {
    float a2 = alpha * alpha;
    float gv = NoL * sqrt(a2 + (1.0 - a2) * NoV * NoV);
    float gl = NoV * sqrt(a2 + (1.0 - a2) * NoL * NoL);
    return 0.5 / max(gv + gl, 1e-6);
}

vec3 fresnelSchlick(float cos_theta, vec3 F0) {
    return F0 + (vec3(1.0) - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

#ifdef USE_IBL
vec3 fresnelSchlickRoughness(float cos_theta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}
#endif

vec3 cookTorranceShading(
    const in vec3 light_dir,
    const in vec3 light_color,
    const in vec3 normal,
    const in vec3 view_dir,
    const in vec3 base_color,
    const in float metallic,
    const in float roughness
) {
    float NoL = max(dot(normal, light_dir), 0.0);
    if (NoL <= 0.0) return vec3(0.0);

    vec3 halfway = normalize(light_dir + view_dir);
    float NoV = max(dot(normal, view_dir), 0.0);
    float NoH = max(dot(normal, halfway), 0.0);
    float VoH = max(dot(view_dir, halfway), 0.0);

    float alpha = roughness * roughness;
    vec3 F0 = mix(vec3(0.04), base_color, metallic);

    float D = distributionGGX(NoH, alpha);
    float V = visibilityGGX(NoL, NoV, alpha);
    vec3 F = fresnelSchlick(VoH, F0);

    vec3 specular = D * V * F;

    vec3 diffuse = (1.0 - metallic) * base_color / PI;

    return (diffuse + specular) * light_color * NoL;
}

#if NUM_LIGHTS > 0

#include "include/lights.incl.glsl"

vec3 processLights(
    const in vec3 normal,
    const in vec3 view_dir,
    const in vec3 base_color,
    const in float metallic,
    const in float roughness
) {
    vec3 output_color = vec3(0.0);
    for (int i = 0; i < NUM_LIGHTS; i++) {
        Light light = u_Lights[i];

        if (light.Type == 1 /* directional light */) {
            output_color += shadowFactor(light, v_Position) * cookTorranceShading(
                light.Direction,
                light.Color,
                normal,
                view_dir,
                base_color,
                metallic,
                roughness
            );
        }

        if (light.Type == 2 /* point light */) {
            vec3 light_dir = normalize(light.Position - v_Position.xyz);
            float dist = length(light.Position - v_Position.xyz);
            output_color += shadowFactor(light, v_Position) * attenuation(dist, light) * cookTorranceShading(
                light_dir,
                light.Color,
                normal,
                view_dir,
                base_color,
                metallic,
                roughness
            );
        }

        if (light.Type == 3 /* spot light */) {
            vec3 light_dir = normalize(light.Position - v_Position.xyz);
            float dist = length(light.Position - v_Position.xyz);
            float angle_cos = dot(light_dir, light.Direction);
            if (angle_cos > light.ConeCos) {
                vec3 spot_color = light.Color * smoothstep(light.ConeCos, light.PenumbraCos, angle_cos);
                output_color += shadowFactor(light, v_Position) * attenuation(dist, light) * cookTorranceShading(
                    light_dir,
                    spot_color,
                    normal,
                    view_dir,
                    base_color,
                    metallic,
                    roughness
                );
            }
        }
    }
    return output_color;
}

#endif

void main() {
    vec3 view_dir = normalize(v_ViewDir);

    #ifdef USE_FLAT_SHADED
        vec3 fdx = dFdx(v_Position.xyz);
        vec3 fdy = dFdy(v_Position.xyz);
        vec3 normal = normalize(cross(fdx, fdy));
    #else
        vec3 normal = normalize(v_Normal);

        #ifdef USE_NORMAL_MAP
            vec3 normal_tan = (texture(u_NormalMap, v_TexCoords).rgb * 2.0 - 1.0);
            normal_tan.xy *= u_NormalIntensity;
            normal = normalize(v_TBN * normal_tan);
        #endif

        #ifdef USE_FLIP_NORMALS
            normal *= gl_FrontFacing ? 1.0 : -1.0;
        #endif
    #endif

    vec3 base_color = u_Material.Color;
    float opacity = u_Opacity;

    #ifdef USE_INSTANCING
        base_color *= v_InstanceColor;
    #endif

    #ifdef USE_VERTEX_COLOR
        base_color *= v_Color;
    #endif

    #ifdef USE_ALBEDO_MAP
        vec4 texture_sample = texture(u_AlbedoMap, v_TexCoords);
        base_color *= texture_sample.rgb;
        opacity *= texture_sample.a;
    #endif

    #ifdef USE_ALPHA_MAP
        vec4 alpha_sample = texture(u_AlphaMap, v_TexCoords);
        opacity *= alpha_sample.r;
    #endif

    #ifdef USE_ALPHA_TEST
        if (opacity < u_AlphaTest) discard;
    #endif

    float metallic = u_Material.Metallic;
    #ifdef USE_METALLIC_MAP
        metallic *= texture(u_MetallicMap, v_TexCoords).b;
    #endif
    metallic = clamp(metallic, 0.0, 1.0);

    float roughness = u_Material.Roughness;
    #ifdef USE_ROUGHNESS_MAP
        roughness *= texture(u_RoughnessMap, v_TexCoords).g;
    #endif
    roughness = clamp(roughness, 0.04, 1.0);

    float ao = 1.0;
    #ifdef USE_AO_MAP
        float ao_sample = texture(u_AOMap, v_TexCoords).r;
        ao = mix(1.0, ao_sample, u_AOIntensity);
    #endif

    #ifdef USE_IBL
        float NoV = max(dot(normal, view_dir), 0.0);
        vec3 F0 = mix(vec3(0.04), base_color, metallic);
        vec3 F = fresnelSchlickRoughness(NoV, F0, roughness);
        vec3 kD = (1.0 - F) * (1.0 - metallic);

        // Cubes are world space but the varyings are view space, so rotate the
        // lookup directions back to world (u_View is rigid, so transpose = inverse).
        mat3 view_to_world = transpose(mat3(u_View));
        vec3 N_world = normalize(view_to_world * normal);
        vec3 R_world = normalize(view_to_world * reflect(-view_dir, normal));

        vec3 diffuse = texture(u_IrradianceMap, N_world).rgb * base_color;

        vec3 prefiltered = textureLod(u_PrefilteredMap, R_world, roughness * u_PrefilteredMaxLod).rgb;
        vec2 ab = texture(u_BrdfLut, vec2(NoV, roughness)).rg;
        vec3 specular = prefiltered * (F0 * ab.x + ab.y);

        vec3 output_color = (kD * diffuse + specular) * ao * u_EnvironmentIntensity;
    #else
        vec3 output_color = base_color * (1.0 - metallic) * u_AmbientLight * ao / PI;
    #endif
    #if NUM_LIGHTS > 0
        output_color += processLights(normal, view_dir, base_color, metallic, roughness);
    #endif

    vec3 emissive = u_EmissiveColor;
    #ifdef USE_EMISSIVE_MAP
        emissive *= texture(u_EmissiveMap, v_TexCoords).rgb;
    #endif

    emissive *= u_EmissiveIntensity;
    output_color += emissive;

    #ifdef USE_FOG
        applyFog(output_color, v_ViewDepth);
    #endif

    o_FragColor = vec4(output_color, opacity);
}
