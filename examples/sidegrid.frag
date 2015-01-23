vec2 tile(vec2 _st, float _zoom){
    _st *= _zoom;
    return fract(_st);
}

float X(vec2 _st, float _width){
    float pct0 = smoothstep(_st.x-_width,_st.x,_st.y);
    pct0 *= 1.-smoothstep(_st.x,_st.x+_width,_st.y);

    float pct1 = smoothstep(_st.x-_width,_st.x,1.0-_st.y);
    pct1 *= 1.-smoothstep(_st.x,_st.x+_width,1.0-_st.y);

    return pct0+pct1;
}

void main(void){
    vec2 st = v_texcoord;
    st = tile(st,10.0);
    
    vec3 color = vec3(X(st,0.03));

    gl_FragColor = vec4(color,1.0);    
}