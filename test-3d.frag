#ifdef GL_ES
precision mediump float;
#endif

//	Default uniforms
uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

varying vec4 v_position;
// varying vec4 v_color;
varying vec3 v_normal;
// varying vec2 v_texcoord;


// LIGHT Functions and Structs
struct Light { vec3 ambient, diffuse, specular; };
struct DirectionalLight { Light emission; vec3 direction; };
struct PointLight { Light emission; vec3 position; };
struct Material { Light bounce; vec3 emission; float shininess;};

void computeLight(in DirectionalLight _light, in Material _material, in vec3 _pos, in vec3 _normal, inout Light _accumulator ){
    _accumulator.ambient += _light.emission.ambient;

    float diffuseFactor = clamp(dot(_normal,-_light.direction),0.0,1.0);
    _accumulator.diffuse += _light.emission.diffuse * diffuseFactor;

    if (diffuseFactor > 0.0) {
        vec3 reflectVector = reflect(_light.direction, _normal);
        float specularFactor = clamp(pow( dot(normalize(_pos), reflectVector), _material.shininess),0.0,1.0);
        _accumulator.specular += _light.emission.specular * specularFactor;
    }

}

void computeLight(in PointLight _light, in Material _material, in vec3 _pos, in vec3 _normal, inout Light _accumulator ){
    float dist = length(_light.position - _pos);
    vec3 lightDirection = (_light.position - _pos)/dist;

    _accumulator.ambient += _light.emission.ambient;

    float diffuseFactor = clamp(dot(lightDirection,_normal),0.0,1.0);
    _accumulator.diffuse += _light.emission.diffuse * diffuseFactor;

    if (diffuseFactor > 0.0) {
        vec3 reflectVector = reflect(-lightDirection, _normal);
        float specularFactor = clamp(pow( dot(-normalize(_pos), reflectVector), _material.shininess),0.0,1.0);
        _accumulator.specular += _light.emission.specular * specularFactor;
    }
}

vec3 calculate(in Material _material, in Light _light){
    vec3 color = vec3(0.0);
    color += _material.emission;
    color += _material.bounce.ambient * _light.ambient;
    color += _material.bounce.diffuse * _light.diffuse;
    color += _material.bounce.specular * _light.specular;
    return color;
}

vec3 rim (in vec3 _normal, in float _pct) {
    float cosTheta = abs( dot( vec3(0.0,0.0,-1.0) , _normal));
    return vec3( _pct * ( 1. - smoothstep( 0.0, 1., cosTheta ) ) );
}

// SCENE Definitions
//---------------------------------------------------

//  Light accumulator
Light l = Light(vec3(0.0),vec3(0.0),vec3(0.0)); 

//  Material
Material m = Material(Light(vec3(1.0),vec3(1.0),vec3(1.0)),vec3(0.0),200.0);

// Lights
DirectionalLight dLight = DirectionalLight(Light(vec3(0.1),vec3(0.3,0.6,0.6),vec3(1.0)),vec3(1.0));
PointLight pLight = PointLight(Light(vec3(0.1),vec3(0.6,0.6,0.3),vec3(0.5)),vec3(1.0));

void main (void) {
	vec2 st = gl_FragCoord.xy/u_resolution.xy;
	float aspect = u_resolution.x/u_resolution.y;
    st.x *= aspect;

    vec3 color = (v_normal-0.5)*2.;

    // dLight.direction = vec3(cos(u_time),0.0,sin(u_time));
    // computeLight(dLight,m,v_position.xyz,v_normal,l);
  
    pLight.position = vec3(cos(u_time),0.5,sin(u_time))*2.0;
    pLight.emission.ambient = vec3(0.129,0.180,0.196);
    pLight.emission.diffuse = vec3(1.000,0.945,0.655);
    pLight.emission.specular = vec3(0.788,1.000,0.655);
    computeLight(pLight,m,v_position.xyz,v_normal,l);
  
    color = calculate(m,l);

	//	Assign the resultant color
	gl_FragColor = vec4(color,1.0);
}
