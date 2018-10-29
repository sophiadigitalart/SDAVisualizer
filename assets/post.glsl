uniform vec3      	iResolution; 			// viewport resolution (in pixels)
uniform vec3        iChannelResolution[4];	// channel resolution (in pixels)
uniform float     	iGlobalTime; 			// shader playback time (in seconds)
uniform vec4      	iMouse; 				// mouse pixel coords. xy: current (if MLB down), zw: click
uniform sampler2D 	iChannel0; 				// input channel 0
uniform sampler2D 	iChannel1; 				// input channel 1


//out vec4 fragColor;
vec2  fragCoord = gl_FragCoord.xy; // keep the 2 spaces between vec2 and fragCoord

void main() {
	vec2 uv = gl_FragCoord.xy / iResolution.xy;

	vec3 t0 = texture(iChannel0, uv ).xyz;
	
	vec3 c = vec3(0.0);
	c = texture(iChannel0, uv).yxz;// + texture(iChannel1, uv).xyz;     

   	gl_FragColor = vec4(c,1.0);
}
