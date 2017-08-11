// Smaller Number Printing - @P_Malin
// Creative Commons CC0 1.0 Universal (CC-0)
// Modified by @doug-moen (github) to display iMouse coordinates.

// Feel free to modify, distribute or use in commercial code, just don't hold me liable for anything bad that happens!
// If you use this code and want to give credit, that would be nice but you don't have to.

// I first made this number printing code in https://www.shadertoy.com/view/4sf3RN
// It started as a silly way of representing digits with rectangles.
// As people started actually using this in a number of places I thought I would try to condense the
// useful function a little so that it can be dropped into other shaders more easily,
// just snip between the perforations below.
// Also, the licence on the previous shader was a bit restrictive for utility code.
//
// Disclaimer: The values printed may not be accurate!
// Accuracy improvement for fractional values taken from TimoKinnunen https://www.shadertoy.com/view/lt3GRj

// ---- 8< ---- GLSL Number Printing - @P_Malin ---- 8< ----
// Creative Commons CC0 1.0 Universal (CC-0)
// https://www.shadertoy.com/view/4sBSWW

float DigitBin( const int x )
{
    return x==0?480599.0:x==1?139810.0:x==2?476951.0:x==3?476999.0:x==4?350020.0:x==5?464711.0:x==6?464727.0:x==7?476228.0:x==8?481111.0:x==9?481095.0:0.0;
}

float PrintValue( const vec2 vStringCoords, const float fValue, const float fMaxDigits, const float fDecimalPlaces )
{
    if ((vStringCoords.y < 0.0) || (vStringCoords.y >= 1.0)) return 0.0;
	float fLog10Value = log2(abs(fValue)) / log2(10.0);
	float fBiggestIndex = max(floor(fLog10Value), 0.0);
	float fDigitIndex = fMaxDigits - floor(vStringCoords.x);
	float fCharBin = 0.0;
	if(fDigitIndex > (-fDecimalPlaces - 1.01)) {
		if(fDigitIndex > fBiggestIndex) {
			if((fValue < 0.0) && (fDigitIndex < (fBiggestIndex+1.5))) fCharBin = 1792.0;
		} else {
			if(fDigitIndex == -1.0) {
				if(fDecimalPlaces > 0.0) fCharBin = 2.0;
			} else {
                float fReducedRangeValue = fValue;
                if(fDigitIndex < 0.0) { fReducedRangeValue = fract( fValue ); fDigitIndex += 1.0; }
				float fDigitValue = (abs(fReducedRangeValue / (pow(10.0, fDigitIndex))));
                fCharBin = DigitBin(int(floor(mod(fDigitValue, 10.0))));
			}
        }
	}
    return floor(mod((fCharBin / pow(2.0, floor(fract(vStringCoords.x) * 4.0) + (floor(vStringCoords.y * 5.0) * 4.0))), 2.0));
}

// ---- 8< -------- 8< -------- 8< -------- 8< ----


// Original interface

float PrintValue(const in vec2 fragCoord, const in vec2 vPixelCoords, const in vec2 vFontSize, const in float fValue, const in float fMaxDigits, const in float fDecimalPlaces)
{
    vec2 vStringCharCoords = (fragCoord.xy - vPixelCoords) / vFontSize;

    return PrintValue( vStringCharCoords, fValue, fMaxDigits, fDecimalPlaces );
}

float GetCurve(float x)
{
	return sin( x * 3.14159 * 4.0 );
}

float GetCurveDeriv(float x)
{
	return 3.14159 * 4.0 * cos( x * 3.14159 * 4.0 );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec3 vColour = vec3(0.0);

	// Multiples of 4x5 work best
	vec2 vFontSize = vec2(8.0, 15.0);

	// Draw Horizontal Line
	if(abs(fragCoord.y - iResolution.y * 0.5) < 1.0)
	{
		vColour = vec3(0.25);
	}

	// Draw Sin Wave
	// See the comment from iq or this page
	// http://www.iquilezles.org/www/articles/distance/distance.htm
	float fCurveX = fragCoord.x / iResolution.x;
	float fSinY = (GetCurve(fCurveX) * 0.25 + 0.5) * iResolution.y;
	float fSinYdX = (GetCurveDeriv(fCurveX) * 0.25) * iResolution.y / iResolution.x;
	float fDistanceToCurve = abs(fSinY - fragCoord.y) / sqrt(1.0+fSinYdX*fSinYdX);
	float fSetPixel = fDistanceToCurve - 1.0; // Add more thickness
	vColour = mix(vec3(1.0, 0.0, 0.0), vColour, clamp(fSetPixel, 0.0, 1.0));

	// Draw Sin Value
	float fValue4 = GetCurve(iMouse.x / iResolution.x);
	float fPixelYCoord = (fValue4 * 0.25 + 0.5) * iResolution.y;

	// Plot Point on Sin Wave
	float fDistToPointA = length( vec2(iMouse.x, fPixelYCoord) - fragCoord.xy) - 4.0;
	vColour = mix(vColour, vec3(0.0, 0.0, 1.0), (1.0 - clamp(fDistToPointA, 0.0, 1.0)));

	// Plot Mouse Pos
	float fDistToPointB = length( vec2(iMouse.x, iMouse.y) - fragCoord.xy) - 4.0;
	vColour = mix(vColour, vec3(0.0, 1.0, 0.0), (1.0 - clamp(fDistToPointB, 0.0, 1.0)));

	// Print Sin Value
	vec2 vPixelCoord4 = vec2(iMouse.x, fPixelYCoord) + vec2(4.0, 4.0);
	float fDigits = 1.0;
	float fDecimalPlaces = 2.0;
	float fIsDigit4 = PrintValue( (fragCoord - vPixelCoord4) / vFontSize, fValue4, fDigits, fDecimalPlaces);
	vColour = mix( vColour, vec3(0.0, 0.0, 1.0), fIsDigit4);

	// Print Shader Time
	vec2 vPixelCoord1 = vec2(96.0, 5.0);
	float fValue1 = iGlobalTime;
	fDigits = 6.0;
	float fIsDigit1 = PrintValue( (fragCoord - vPixelCoord1) / vFontSize, fValue1, fDigits, fDecimalPlaces);
	vColour = mix( vColour, vec3(0.0, 1.0, 1.0), fIsDigit1);

	// Print Date
	vColour = mix( vColour, vec3(1.0, 1.0, 0.0), PrintValue( (fragCoord - vec2(0.0, 5.0)) / vFontSize, iDate.x, 4.0, 0.0));
	vColour = mix( vColour, vec3(1.0, 1.0, 0.0), PrintValue( (fragCoord - vec2(0.0 + 48.0, 5.0)) / vFontSize, iDate.y + 1.0, 2.0, 0.0));
	vColour = mix( vColour, vec3(1.0, 1.0, 0.0), PrintValue( (fragCoord - vec2(0.0 + 72.0, 5.0)) / vFontSize, iDate.z, 2.0, 0.0));

	// Draw Time
	vColour = mix( vColour, vec3(1.0, 0.0, 1.0), PrintValue( (fragCoord - vec2(184.0, 5.0)) / vFontSize, mod(iDate.w / (60.0 * 60.0), 12.0), 2.0, 0.0));
	vColour = mix( vColour, vec3(1.0, 0.0, 1.0), PrintValue( (fragCoord - vec2(184.0 + 24.0, 5.0)) / vFontSize, mod(iDate.w / 60.0, 60.0), 2.0, 0.0));
	vColour = mix( vColour, vec3(1.0, 0.0, 1.0), PrintValue( (fragCoord - vec2(184.0 + 48.0, 5.0)) / vFontSize, mod(iDate.w, 60.0), 2.0, 0.0));

	if(iMouse.x > 0.0)
	{
		// Print Mouse X
		vec2 vPixelCoord2 = iMouse.xy + vec2(-52.0, 6.0);
		float fValue2 = iMouse.x / iResolution.x;
		fDigits = 1.0;
		fDecimalPlaces = 3.0;
		float fIsDigit2 = PrintValue( (fragCoord - vPixelCoord2) / vFontSize, fValue2, fDigits, fDecimalPlaces);
		vColour = mix( vColour, vec3(0.0, 1.0, 0.0), fIsDigit2);

		// Print Mouse Y
		vec2 vPixelCoord3 = iMouse.xy + vec2(0.0, 6.0);
		float fValue3 = iMouse.y / iResolution.y;
		fDigits = 1.0;
		float fIsDigit3 = PrintValue( (fragCoord - vPixelCoord3) / vFontSize, fValue3, fDigits, fDecimalPlaces);
		vColour = mix( vColour, vec3(0.0, 1.0, 0.0), fIsDigit3);
	}
    // Draw Mouse
    vColour = mix( vColour, vec3(0.0, 1.0, 0.0), PrintValue(
      (fragCoord-vec2(300.0, 5.0))/vFontSize, iMouse.x, 2.0, 0.0));
    vColour = mix( vColour, vec3(0.0, 1.0, 0.0), PrintValue(
      (fragCoord-vec2(333.0, 5.0))/vFontSize, iMouse.y, 2.0, 0.0));
    vColour = mix( vColour, vec3(0.0, 1.0, 0.0), PrintValue(
      (fragCoord-vec2(366.0, 5.0))/vFontSize, iMouse.z, 2.0, 0.0));
    vColour = mix( vColour, vec3(0.0, 1.0, 0.0), PrintValue(
      (fragCoord-vec2(399.0, 5.0))/vFontSize, iMouse.w, 2.0, 0.0));


	fragColor = vec4(vColour,1.0);
}
