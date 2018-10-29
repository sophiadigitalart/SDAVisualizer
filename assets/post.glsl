uniform vec3      	iResolution; 			// viewport resolution (in pixels)
uniform vec3        iChannelResolution[4];	// channel resolution (in pixels)
uniform float     	iGlobalTime; 			// shader playback time (in seconds)
uniform vec4      	iMouse; 				// mouse pixel coords. xy: current (if MLB down), zw: click
uniform sampler2D 	iChannel0; 				// input channel 0
uniform sampler2D 	iChannel1; 				// input channel 1
uniform float		iExposure;
uniform float		iSobel;
uniform float		iChromatic;

//out vec4 fragColor;
vec2  fragCoord = gl_FragCoord.xy; // keep the 2 spaces between vec2 and fragCoord

float intensity(in vec4 c){return sqrt((c.x*c.x)+(c.y*c.y)+(c.z*c.z));}

vec4 sobel(float stepx, float stepy, vec2 center) {
	float tleft = intensity(texture(iChannel0,center + vec2(-stepx,stepy)));
	float left = intensity(texture(iChannel0,center + vec2(-stepx,0)));
	float bleft = intensity(texture(iChannel0,center + vec2(-stepx,-stepy)));
	float top = intensity(texture(iChannel0,center + vec2(0,stepy)));
	float bottom = intensity(texture(iChannel0,center + vec2(0,-stepy)));
	float tright = intensity(texture(iChannel0,center + vec2(stepx,stepy)));
	float right = intensity(texture(iChannel0,center + vec2(stepx,0)));
	float bright = intensity(texture(iChannel0,center + vec2(stepx,-stepy)));
	float x = tleft + 2.0*left + bleft - tright - 2.0*right - bright;
	float y = -tleft - 2.0*top - tright + bleft + 2.0 * bottom + bright;
	return vec4(sqrt((x*x) + (y*y)));
}
vec4 chromatic( vec2 uv ) {	
	vec2 offset = vec2(iChromatic / 36., .0);
	return vec4(texture(iChannel0, uv + offset.xy).r,  texture(iChannel0, uv).g, texture(iChannel0, uv + offset.yx).b, 1.0);
}
void main() {
	vec2 uv = gl_FragCoord.xy / iResolution.xy;
	uv.y = 1.0 - uv.y;
	vec4 t0 = texture(iChannel0, uv );
	
	vec4 c = vec4(0.0);
	if (iSobel > 0.03) { t0 = sobel(iSobel * 3.0 /iResolution.x, iSobel * 3.0 /iResolution.y, uv); }
	if (iChromatic > 0.0) { t0 = chromatic(uv) * t0; }
	c = t0;
	c *= iExposure;
   	gl_FragColor = c;
}
