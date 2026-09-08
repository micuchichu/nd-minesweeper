#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform float bubbleBlur;
uniform float crtEnabled;

vec2 curveUV(vec2 uv) {
    vec2 centered = uv - 0.5;
    float r2 = dot(centered, centered);
    vec2 curved = uv + centered * (r2 * 0.05);
    return curved;
}

void main() {
    vec2 baseUV = (crtEnabled > 0.5) ? curveUV(fragTexCoord) : fragTexCoord;
    
    if (baseUV.x < 0.0 || baseUV.x > 1.0 || baseUV.y < 0.0 || baseUV.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 uv = baseUV;
    
    // Bubble aquatic wobble distortion when bubble blur is active
    if (bubbleBlur > 0.005) {
        vec2 wobble = vec2(
            sin(uv.y * 22.0 + time * 5.0) * 0.012,
            cos(uv.x * 22.0 + time * 4.5) * 0.012
        ) * bubbleBlur;
        uv = clamp(uv + wobble, 0.0, 1.0);
    }

    vec2 fromCenter = uv - 0.5;
    float dist = length(fromCenter);
    float caAmount = (0.005 * dist + 0.012 * bubbleBlur) * (1.0 + sin(time * 1.5) * 0.1);
    
    float r = texture(texture0, uv - fromCenter * caAmount).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv + fromCenter * caAmount).b;
    vec3 color = vec3(r, g, b);
    
    vec2 texel = 1.0 / resolution;

    // Multi-tap blur kernel when bubbleBlur > 0.01
    if (bubbleBlur > 0.01) {
        vec3 blurAcc = vec3(0.0);
        float blurWeight = 0.0;
        float blurRad = 11.0 * bubbleBlur;

        for (int i = 0; i < 16; ++i) {
            float a = float(i) * 2.3999632;
            float rad = sqrt(float(i) + 0.5) / 4.0;
            vec2 offset = vec2(cos(a), sin(a)) * rad * blurRad * texel;
            vec2 sUV = clamp(uv + offset, 0.0, 1.0);

            float sR = texture(texture0, clamp(sUV - offset * 0.08, 0.0, 1.0)).r;
            float sG = texture(texture0, sUV).g;
            float sB = texture(texture0, clamp(sUV + offset * 0.08, 0.0, 1.0)).b;
            blurAcc += vec3(sR, sG, sB);
            blurWeight += 1.0;
        }

        vec3 blurred = blurAcc / blurWeight;
        blurred = mix(blurred, blurred * vec3(0.92, 1.04, 1.12) + vec3(0.01, 0.03, 0.05), bubbleBlur * 0.20);
        color = mix(color, blurred, clamp(bubbleBlur * 0.85, 0.0, 0.85));
    } else {
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
    }

    if (crtEnabled > 0.5) {
        float scanline = sin(uv.y * resolution.y * 3.14159) * 0.035;
        color -= scanline;

        float flicker = 1.0 + sin(time * 15.0) * 0.015;
        color *= flicker;

        float vignette = smoothstep(0.9, 0.3, dist);
        color *= vignette;
    }

    finalColor = vec4(color, 1.0);
}