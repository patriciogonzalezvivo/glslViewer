//	CONSTANT
//
#define PHI 1.618033988749894848204586834
#define HALF_PI 1.57079632679489661923
#define PI 3.14159265358979323846
#define TWO_PI 6.28318530717958647693
#define DEG_TO_RAD (PI/180.0)
#define RAD_TO_DEG (180.0/PI)

//	MACRO OPERATIONS
//
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define CLAMP(val,min,max) ((val) < (min) ? (min) : ((val > max) ? (max) : (val)))
#define ABS(x) (((x) < 0) ? -(x) : (x))

//	IQuiles USEFUL FUNCTIONS http://iquilezles.org/www/articles/functions/functions.htm
//

float cubicPulse( float c, float w, float x ){
	x = abs(x - c);
	if( x>w ) return 0.0;
	x /= w;
	return 1.0 - x*x*(3.0-2.0*x);
}
