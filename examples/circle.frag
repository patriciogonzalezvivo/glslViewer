
float circle(vec2 _st, float _radius){
	vec2 pos = vec2(0.5)-_st;
	_radius *= 0.75;
	return 1.-smoothstep(_radius-(_radius*0.01),_radius+(_radius*0.01),dot(pos,pos)*3.14);
}

void main(){
	vec2 st = v_texcoord;
	
	vec3 color = vec3(vec3(circle(st,0.9)));

	gl_FragColor = vec4( color, 1.0 );
}