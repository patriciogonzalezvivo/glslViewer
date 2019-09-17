#ifndef FNC_TONEMAP
#define FNC_TONEMAP
vec3 tonemap(const vec3 x) { 
    return x / (x + vec3(1.0));
    // return x / (x + 0.155) * 1.019; 
}
#endif