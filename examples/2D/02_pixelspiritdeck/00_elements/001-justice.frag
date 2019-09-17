// Number: XI
// Title: Justice
// Author: Patricio Gonzalez Vivo

#ifdef GL_ES
precision mediump float;
#endif

//GLOBAL_START
uniform vec2 u_resolution;

void main() {
    vec3 color = vec3(0.);
    vec2 st = gl_FragCoord.xy/u_resolution;
//GLOBAL_END
    st = (st-.5)*1.1912+.5;
    if (u_resolution.y > u_resolution.x ) {
        st.y *= u_resolution.y/u_resolution.x;
        st.y -= (u_resolution.y*.5-u_resolution.x*.5)/u_resolution.x;
    } else {
        st.x *= u_resolution.x/u_resolution.y;
        st.x -= (u_resolution.x*.5-u_resolution.y*.5)/u_resolution.y;
    }
//GLOBAL_START

    color += step(.5,st.x);
    
    gl_FragColor = vec4(color,1.);
}
//GLOBAL_END