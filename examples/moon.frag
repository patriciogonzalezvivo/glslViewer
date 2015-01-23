float speed = 0.1;

vec3 sphere(vec2 uv) {
	uv = fract(uv)*2.0-1.0; 
	vec3 ret;
	ret.xy = sqrt(uv * uv) * sign(uv);
	ret.z = sqrt(abs(1.0 - dot(ret.xy,ret.xy)));
	ret = ret * 0.5 + 0.5;    
	return mix(vec3(0.0), ret, smoothstep(1.0,0.98,dot(uv,uv)) );
}

void main(){
	vec2 st = gl_FragCoord.xy/iResolution.xy;
    float aspect = iResolution.x/iResolution.y;
    st.x -= (iResolution.x-iResolution.y)/iResolution.x;
    st.x *= aspect;

    vec3 color = vec3(0.0);

    float time = speed*u_time;
    vec3 light = normalize(vec3(cos(time),0.0,sin(time)));
    vec3 surface = normalize(sphere(st)*2.0-1.0);
    float border = 1.0-length(st-vec2(0.5))*0.5;
   
	float radius = 1.0-length( vec2(0.5)-st )*2.0;
	radius = smoothstep(0.001,0.05,radius);
	color = vec3( smoothstep(0.1,0.15,dot(light,surface)*radius) );

    gl_FragColor = vec4(color, 1.0);
}