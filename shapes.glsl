
vec4 grid(in float _lineWidth,in float _size, in vec2 _st){
	vec4 rta = vec4(0.0);
	vec2 grid = fract(_st*_size);
	if (grid.x < _lineWidth || grid.y < _lineWidth){
		rta.x = smoothstep(0.0,_lineWidth,grid.x);
		rta.y = smoothstep(0.0,_lineWidth,grid.y);
		rta.a = 1.0;
	}
	return rta;
}

vec4 cross(in float _lineWidth, in float _size, in vec2 _st){
	vec4 rta = vec4(0.0);
	if( length(_st) < _size){
		if(abs(_st.x) < _lineWidth || abs(_st.y) < _lineWidth+0.00005 ){
			rta = vec4(1.0);
		}
	}
	return rta;
}

