#ifdef GL_ES
precision mediump float;
#endif

uniform float u_temp;

void main (void) {
    float temp_min = 20.;
    float temp_max = 80.;
    float temp = (u_temp-temp_min)/(temp_max-temp_min);

    vec3 cold = vec3(0.,0.,1.);
    vec3 hot = vec3(1.,0.,0.);

	gl_FragColor = vec4(mix(cold, hot, temp),1.0);
}
