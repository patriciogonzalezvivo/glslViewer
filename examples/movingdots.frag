
vec2 movingTiles(vec2 _st, float _zoom, float _speed){
	_st *= _zoom;
	float time = u_time*_speed;
	if( fract(time)>0.5 ){
		if (fract( _st.y * 0.5) > 0.5){
			_st.x += fract(time)*2.0;
		} else {
			_st.x -= fract(time)*2.0;
		} 
	} else {
		if (fract( _st.x * 0.5) > 0.5){
			_st.y += fract(time)*2.0;
		} else {
			_st.y -= fract(time)*2.0;
		} 
	}
	return fract(_st);
}

float circle(vec2 _st, float _radius){
	vec2 pos = vec2(0.5)-_st;
	return smoothstep(1.0-_radius,1.0-_radius+_radius*0.2,1.-dot(pos,pos)*3.14);
}

void main() {
	vec2 st = v_texcoord;

	st = movingTiles(st,10.,0.5);
	
	vec3 color = vec3( circle(st, 0.5 ) );

	gl_FragColor = vec4(color,1.0);
}