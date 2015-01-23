
vec2 tile(vec2 _st, float _zoom){
	_st *= _zoom;
	if (fract(_st.y * 0.5) > 0.5){
		_st.x += 0.5;
	}
	return fract(_st);
}

float circle(vec2 _st, float _radius){
	vec2 pos = vec2(0.5)-_st;
	return smoothstep(1.0-_radius,1.0-_radius+_radius*0.2,1.-dot(pos,pos)*3.14);
}

void main() {

	vec2 st = v_texcoord;
	st = tile(st,10.0);
	

	vec3 color = vec3( circle(st, (1.+sin(u_time)*0.4)*0.5) );

	gl_FragColor = vec4(color,1.0);
}