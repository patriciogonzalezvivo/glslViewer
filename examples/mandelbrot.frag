#ifdef GL_ES
precision mediump float;
#endif

// Demo of pan and zoom using left-mouse-button drag and scroll-wheel.

uniform mat3 u_view2d;
uniform vec2 u_resolution;

// bounding box of initial view: [xmin,ymin,xmax,ymax]
const vec4 bbox = vec4(-2.0, -1.0, 1.0, 1.0);

// A larger value of maxiter gives finer detail.
const int maxiter = 100;

void main() {
    // (scale,offset) is a transformation
    // that centers the bounding box in the window.
    vec2 size = bbox.zw - bbox.xy;
    vec2 scale2 = size / u_resolution.xy;
    vec2 offset = bbox.xy;
    float scale;
    if (scale2.x > scale2.y) {
        scale = scale2.x;
        offset.y -= (u_resolution.y*scale - size.y)/2.0;
    } else {
        scale = scale2.y;
        offset.x -= (u_resolution.x*scale - size.x)/2.0;
    }

    // Transform the fragment coordinates into model coordinates.
    vec2 p = (u_view2d * vec3(gl_FragCoord.xy, 1.0)).xy;
    p = p*scale + offset;

    vec2 c = p;

    // Set default color to HSV value for black
    vec3 color = vec3(0.0,0.0,0.0);

    for (int i=0; i<maxiter; i++) {
        //Perform complex number arithmetic
        p= vec2(p.x*p.x-p.y*p.y,2.0*p.x*p.y)+c;

        if (dot(p,p)>4.0){
            // The point, c, is not part of the set, so smoothly color it.
            // colorRegulator increases linearly by 1 for every extra step it
            // takes to break free.
            // http://iquilezles.org/www/articles/mset_smooth/mset_smooth.htm
            float colorRegulator = float(i-1)-log(((log(dot(p,p)))/log(2.0)))/log(2.0);
            // An arbitrary HSV color scheme that looks nice.
            color = vec3(0.95 + .012*colorRegulator , 1.0, .2+.4*(1.0+sin(.3*colorRegulator)));
            break;
        }
    }

    // Change color from HSV to RGB.
    // https://gist.github.com/patriciogonzalezvivo/114c1653de9e3da6e1e3
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 m = abs(fract(color.xxx + K.xyz) * 6.0 - K.www);
    gl_FragColor.rgb = color.z * mix(K.xxx, clamp(m - K.xxx, 0.0, 1.0), color.y);

    gl_FragColor.a=1.0;
}
