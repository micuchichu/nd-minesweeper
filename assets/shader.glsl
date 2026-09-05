#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;

vec2 curveUV(vec2 uv) {
    vec2 centered = uv - 0.5;
    float r2 = dot(centered, centered);
    vec2 curved = uv + centered * (r2 * 0.05);
    return curved;
}

void main() {
    vec2 uv = curveUV(fragTexCoord);
    
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 fromCenter = uv - 0.5;
    float dist = length(fromCenter);
    float caAmount = 0.005 * dist * (1.0 + sin(time * 1.5) * 0.1);
    
    float r = texture(texture0, uv - fromCenter * caAmount).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv + fromCenter * caAmount).b;
    vec3 color = vec3(r, g, b);
    
    vec2 texel = 1.0 / resolution;
    vec3 bloom = vec3(0.0);
    float weight = 0.0;
    
    for(float x = -2.0; x <= 2.0; x+=2.0) {
        for(float y = -2.0; y <= 2.0; y+=2.0) {
            vec3 sampleColor = texture(texture0, clamp(uv + vec2(x, y) * texel, 0.0, 1.0)).rgb;
            float brightness = dot(sampleColor, vec3(0.299, 0.587, 0.114));
            
            if(brightness > 0.3) {
                bloom += sampleColor;
                weight += 1.0;
            }
        }
    }
    if (weight > 0.0) {
        color += (bloom / weight) * 0.35;
    }

    float scanline = sin(uv.y * resolution.y * 3.14159) * 0.035;
    color -= scanline;

    float flicker = 1.0 + sin(time * 15.0) * 0.015;
    color *= flicker;

    float vignette = smoothstep(0.9, 0.3, dist);
    color *= vignette;

    finalColor = vec4(color, 1.0);
}