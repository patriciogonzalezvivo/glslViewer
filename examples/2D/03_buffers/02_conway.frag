#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;

uniform vec2        u_resolution;
uniform vec2        u_mouse;
uniform float       u_time;

vec3 live = vec3(0.5,1.0,0.7);
vec3 dead = vec3(0.,0.,0.);
vec3 blue = vec3(0.,0.,1.);

void main( void ) {
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    vec2 pixel = 1./u_resolution;
    vec2 mouse = u_mouse/u_resolution;

#ifdef BUFFER_0
    // PING BUFFER
    //
    //  Note: Here is where most of the action happens. But need's to read
    //  te content of the previous pass, for that we are making another buffer
    //  BUFFER_1 (u_buffer1)
    vec3 color = vec3(0.0);
    if (length(st - mouse) < 0.01) {
        float rnd1 = fract(sin(dot(st + u_time * 0.001, vec2(14.9898,78.233))) * 43758.5453);
        color = mix(live, blue, step(0.5, rnd1));
    } 
    else {
        float sum = 0.;
        sum += texture2D(u_buffer1, st + pixel * vec2(-1., -1.)).g;
        sum += texture2D(u_buffer1, st + pixel * vec2(-1., 0.)).g;
        sum += texture2D(u_buffer1, st + pixel * vec2(-1., 1.)).g;
        sum += texture2D(u_buffer1, st + pixel * vec2(1., -1.)).g;
        sum += texture2D(u_buffer1, st + pixel * vec2(1., 0.)).g;
        sum += texture2D(u_buffer1, st + pixel * vec2(1., 1.)).g;
        sum += texture2D(u_buffer1, st + pixel * vec2(0., -1.)).g;
        sum += texture2D(u_buffer1, st + pixel * vec2(0., 1.)).g;
        color = texture2D(u_buffer1, st).rgb;

        if (color.g <= 0.1) {
            if ((sum >= 2.9) && (sum <= 3.1)) {
                color = live;
            } else if (color.b > 0.004) {
                color = vec3(0., 0., max(color.b - 0.004, 0.25));
            } else {
                color = dead;
            }
        } else {
            if ((sum >= 1.9) && (sum <= 3.1)) {
                color = live;
            } else {
                color = blue;
            }
        }
    }
     
    gl_FragColor = vec4(color, 1.0);
#elif defined( BUFFER_1 )
    // PONG BUFFER
    //
    //  Note: Just copy the content of the BUFFER0 so it can be 
    //  read by it in the next frame
    //
    gl_FragColor = texture2D(u_buffer0, st);
#else

    // Main Buffer
    gl_FragColor = texture2D(u_buffer1, st);
#endif
}