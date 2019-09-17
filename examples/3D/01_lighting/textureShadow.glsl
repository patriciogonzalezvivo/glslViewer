#if defined(LIGHT_SHADOWMAP) && defined(LIGHT_SHADOWMAP_SIZE)
uniform sampler2D   u_ligthShadowMap;
varying vec4        v_lightcoord;

float textureShadow(const sampler2D depths, vec2 uv, float compare){
    return step(compare, texture2D(depths, uv).r);
}

float textureShadowLerp(sampler2D depths, vec2 size, vec2 uv, float compare){
    vec2 texelSize = vec2(1.0)/size;
    vec2 f = fract(uv*size+0.5);
    vec2 centroidUV = floor(uv*size+0.5)/size;

    float lb = textureShadow(depths, centroidUV+texelSize*vec2(0.0, 0.0), compare);
    float lt = textureShadow(depths, centroidUV+texelSize*vec2(0.0, 1.0), compare);
    float rb = textureShadow(depths, centroidUV+texelSize*vec2(1.0, 0.0), compare);
    float rt = textureShadow(depths, centroidUV+texelSize*vec2(1.0, 1.0), compare);
    float a = mix(lb, lt, f.y);
    float b = mix(rb, rt, f.y);
    float c = mix(a, b, f.x);
    return c;
}

float textureShadowPCF(sampler2D depths, vec2 size, vec2 uv, float compare) {
    float result = 0.0;
    for(int x=-2; x<=2; x++){
        for(int y=-2; y<=2; y++){
            vec2 off = vec2(x,y)/size;
            result += textureShadowLerp(depths, size, uv+off, compare);
        }
    }
    return result/25.0;
}
#endif
