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

#ifdef USE_ENVIRONMENT_MAP
    uniform samplerCube u_EnvironmentMap;
    uniform float u_Reflectivity;
#endif

struct PhongMaterial {
    vec3 DiffuseColor;
    vec3 SpecularColor;
    float Shininess;
};

uniform PhongMaterial u_Material;

uniform vec3 u_AmbientLight;
uniform vec3 u_EmissiveColor;
uniform float u_AOIntensity;
uniform float u_EmissiveIntensity;
uniform float u_NormalIntensity;

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_AlphaMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_SpecularMap;
uniform sampler2D u_EmissiveMap;

vec3 phongShading(
    const in vec3 light_dir,
    const in vec3 light_color,
    const in vec3 normal,
    const in vec3 view_dir,
    const in vec3 diffuse_color,
    const in float specular_factor
) {
    float diffuse_factor = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = light_color * diffuse_color * diffuse_factor;

    // If the diffuse factor is zero, the light is facing away from the surface
    // and no light contribution should be calculated, so we skip specular calculation.
    vec3 specular = vec3(0.0);
    if (diffuse_factor > 0.0) {
        vec3 halfway = normalize(light_dir + view_dir);
        specular = light_color * (u_Material.SpecularColor * specular_factor) *
                   pow(max(dot(halfway, normal), 0.0), max(u_Material.Shininess, 1.0));
    }

    return diffuse + specular;
}

#if NUM_LIGHTS > 0

#include "include/lights.incl.glsl"

vec3 processLights(
    const in vec3 normal,
    const in vec3 view_dir,
    const in vec3 diffuse_color,
    const in float specular_factor
) {
    vec3 output_color = vec3(0.0);
    for (int i = 0; i < NUM_LIGHTS; i++) {
        Light light = u_Lights[i];

        if (light.Type == 1 /* directional light */) {
            output_color += shadowFactor(light, v_Position) * phongShading(
                light.Direction,
                light.Color,
                normal,
                view_dir,
                diffuse_color,
                specular_factor
            );
        }

        if (light.Type == 2 /* point light */) {
            vec3 light_dir = normalize(light.Position - v_Position.xyz);
            float dist = length(light.Position - v_Position.xyz);
            output_color += shadowFactor(light, v_Position) * attenuation(dist, light) * phongShading(
                light_dir,
                light.Color,
                normal,
                view_dir,
                diffuse_color,
                specular_factor
            );
        }

        if (light.Type == 3 /* spot light */) {
            vec3 light_dir = normalize(light.Position - v_Position.xyz);
            float dist = length(light.Position - v_Position.xyz);
            float angle_cos = dot(light_dir, light.Direction);
            if (angle_cos > light.ConeCos) {
                vec3 spot_color = light.Color * smoothstep(light.ConeCos, light.PenumbraCos, angle_cos);
                output_color += shadowFactor(light, v_Position) * attenuation(dist, light) * phongShading(
                    light_dir,
                    spot_color,
                    normal,
                    view_dir,
                    diffuse_color,
                    specular_factor
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

    vec3 diffuse_color = u_Material.DiffuseColor;
    float opacity = u_Opacity;

    #ifdef USE_INSTANCING
        diffuse_color *= v_InstanceColor;
    #endif

    #ifdef USE_VERTEX_COLOR
        diffuse_color *= v_Color;
    #endif

    #ifdef USE_ALBEDO_MAP
        vec4 texture_sample = texture(u_AlbedoMap, v_TexCoords);
        diffuse_color *= texture_sample.rgb;
        opacity *= texture_sample.a;
    #endif

    #ifdef USE_ALPHA_MAP
        vec4 alpha_sample = texture(u_AlphaMap, v_TexCoords);
        opacity *= alpha_sample.r;
    #endif

    #ifdef USE_ALPHA_TEST
        if (opacity < u_AlphaTest) discard;
    #endif

    float specular_factor = 1.0;
    #ifdef USE_SPECULAR_MAP
        specular_factor = texture(u_SpecularMap, v_TexCoords).r;
    #endif

    float ao = 1.0;
    #ifdef USE_AO_MAP
        float ao_sample = texture(u_AOMap, v_TexCoords).r;
        ao = mix(1.0, ao_sample, u_AOIntensity);
    #endif

    vec3 output_color = diffuse_color * u_AmbientLight * ao;
    #if NUM_LIGHTS > 0
        output_color += processLights(normal, view_dir, diffuse_color, specular_factor);
    #endif

    #ifdef USE_ENVIRONMENT_MAP
        vec3 view_reflect = reflect(-view_dir, normal);
        vec3 world_reflect = transpose(mat3(u_View)) * view_reflect;
        vec3 environment_color = texture(u_EnvironmentMap, world_reflect).rgb;
        output_color = mix(output_color, environment_color, u_Reflectivity);
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
