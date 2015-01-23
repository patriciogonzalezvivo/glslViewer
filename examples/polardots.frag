#define PI 3.14159265358979323846
#define TWO_PI 6.28318530717958647693
#define PHI 1.618033988749894848204586834

vec2 radialTile(vec2 _st, vec2 _vel, float _zoom){
	vec2 toExtreme = vec2(0.5)-_st;
	vec2 polar = vec2( (PI+atan(toExtreme.y,toExtreme.x))/TWO_PI,	// Angle
						log(length(toExtreme))*PHI*0.1);			// Radius
	polar *= _zoom;
	polar += _vel;

	return fract(polar);
}

float circle(vec2 _st, float _radius){
	vec2 pos = vec2(0.5)-_st;
	return smoothstep(1.0-_radius,1.0-_radius+_radius*0.2,1.-dot(pos,pos)*3.14);
}

void main() {
	vec2 st = v_texcoord;
	st = radialTile(st,vec2(u_time*-0.1),5.0);

	// vec3 color = vec3(st,0.0);
	vec3 color = vec3(circle(st , (1.+sin(u_time)*0.4)*0.5) );

	gl_FragColor = vec4(color,1.0);
}