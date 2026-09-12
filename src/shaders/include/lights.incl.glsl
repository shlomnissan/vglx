struct Light {
    int Type; // 1 = directional, 2 = point, 3 = spot
    vec3 Color;
    vec3 Position;
    vec3 Direction;
    float ConeCos;
    float PenumbraCos;
    float Range;
    int ShadowLayerIndex;
    float ShadowBias;
    float ShadowRadius;
    float ShadowNear;
    float ShadowFar;
    mat4 ShadowTransform;
};

#ifdef USE_SHADOW_MAPS
    uniform sampler2DArrayShadow u_ShadowMaps2D;
    uniform bool u_ReceiveShadow;
#endif

#ifdef USE_POINT_SHADOW_MAPS
    uniform samplerCubeArrayShadow u_PointShadowMaps;
#endif

layout(std140) uniform ub_Lights {
    Light u_Lights[NUM_LIGHTS];
};

float attenuation(const in float dist, const in Light light) {
    float atten = 1.0 / max(dist * dist, 1e-4);
    if (light.Range > 0.0) {
        float window = clamp(1.0 - pow(dist / light.Range, 4.0), 0.0, 1.0);
        atten *= window * window;
    }
    return atten;
}

#ifdef USE_SHADOW_MAPS
float shadowFactor(const in Light light, const in vec4 position) {
    if (!u_ReceiveShadow || light.ShadowLayerIndex < 0) {
        return 1.0;
    }

    #ifdef USE_POINT_SHADOW_MAPS
    if (light.Type == 2 /* point light */) {
        vec3 dir = position.xyz - light.Position;
        dir = transpose(mat3(u_View)) * dir;

        float axis = max(abs(dir.x), max(abs(dir.y), abs(dir.z)));
        if (axis >= light.ShadowFar) {
            return 1.0;
        }

        float ndc = (light.ShadowFar + light.ShadowNear) / (light.ShadowFar - light.ShadowNear)
            - (2.0 * light.ShadowFar * light.ShadowNear) / ((light.ShadowFar - light.ShadowNear) * axis);

        float ref = (ndc * 0.5 + 0.5) - light.ShadowBias;

        #ifdef USE_PCF_SHADOWS
            float layer = float(light.ShadowLayerIndex);
            float texel_step = 2.0 * light.ShadowRadius / float(textureSize(u_PointShadowMaps, 0).x);
            vec2 offset = vec2(-1.0, 1.0) * texel_step;
            vec3 norm_dir = normalize(dir);

            float sum =
                  texture(u_PointShadowMaps, vec4(norm_dir + offset.xxx, layer), ref)
                + texture(u_PointShadowMaps, vec4(norm_dir + offset.xxy, layer), ref)
                + texture(u_PointShadowMaps, vec4(norm_dir + offset.xyx, layer), ref)
                + texture(u_PointShadowMaps, vec4(norm_dir + offset.xyy, layer), ref)
                + texture(u_PointShadowMaps, vec4(norm_dir,              layer), ref)
                + texture(u_PointShadowMaps, vec4(norm_dir + offset.yxx, layer), ref)
                + texture(u_PointShadowMaps, vec4(norm_dir + offset.yxy, layer), ref)
                + texture(u_PointShadowMaps, vec4(norm_dir + offset.yyx, layer), ref)
                + texture(u_PointShadowMaps, vec4(norm_dir + offset.yyy, layer), ref);

            return sum / 9.0;
        #endif

        return texture(u_PointShadowMaps, vec4(dir, float(light.ShadowLayerIndex)), ref);
    }
    #endif

    vec4 coord = light.ShadowTransform * position;
    vec3 proj = coord.xyz / coord.w;
    if (proj.z > 1.0) {
        return 1.0;
    }

    float ref = proj.z - light.ShadowBias;

    #ifdef USE_PCF_SHADOWS
        float texel_step = light.ShadowRadius / float(textureSize(u_ShadowMaps2D, 0).x);
        float sum = 0.0;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                vec2 coords = proj.xy + vec2(i, j) * texel_step;
                sum += texture(u_ShadowMaps2D, vec4(coords, float(light.ShadowLayerIndex), ref));
            }
        }
        return sum / 9.0;
    #endif

    return texture(u_ShadowMaps2D, vec4(proj.xy, float(light.ShadowLayerIndex), ref));
}
#else
float shadowFactor(const in Light light, const in vec4 position) {
    return 1.0;
}
#endif
