void main(){
    vec2 st = gl_FragCoord.xy/iResolution.xy;
    float aspect = iResolution.x/iResolution.y;
    st.x -= (iResolution.x-iResolution.y)/iResolution.x;
    st.x *= aspect;

    vec3 color = vec3(st.x,st.y,0.0);

    gl_FragColor = vec4(color,
                        (st.x < 0. || st.x >= 1.0)? 0.0:1.0);
}
