#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform vec2 cameraVelocity;
uniform float enableCRT;
uniform float enableDoppler;

vec2 curveUV(vec2 uv) {
    vec2 centered = uv - 0.5;
    float r2 = dot(centered, centered);
    vec2 curved = uv + centered * (r2 * 0.05);
    return curved;
}

void main() {
    vec2 uv = (enableCRT > 0.5) ? curveUV(fragTexCoord) : fragTexCoord;
    
    if (enableCRT > 0.5 && (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 color = texture(texture0, clamp(uv, 0.0, 1.0)).rgb;

    // 1. Relativistic Doppler Effect
    if (enableDoppler > 0.5) {
        float speed = length(cameraVelocity);
        if (speed > 0.005) {
            vec2 dir = cameraVelocity / speed;
            vec2 fromCenter = uv - 0.5;
            float alignment = dot(fromCenter, dir);
            
            float dopplerFactor = alignment * speed;
            float dispersion = dopplerFactor * 0.007;
            
            float rSample = texture(texture0, clamp(uv - dir * dispersion, 0.0, 1.0)).r;
            float bSample = texture(texture0, clamp(uv + dir * dispersion, 0.0, 1.0)).b;
            color.r = mix(color.r, rSample, 0.65);
            color.b = mix(color.b, bSample, 0.65);
            
            if (dopplerFactor > 0.0) {
                // Blueshift towards direction of motion
                float b = clamp(dopplerFactor * 1.8, 0.0, 0.75);
                color.r *= (1.0 - b * 0.35);
                color.g *= (1.0 + b * 0.12);
                color.b *= (1.0 + b * 0.55);
                color += vec3(0.0, 0.03, 0.10) * b;
            } else {
                // Redshift away from direction of motion
                float r = clamp(-dopplerFactor * 1.8, 0.0, 0.75);
                color.r *= (1.0 + r * 0.55);
                color.g *= (1.0 + r * 0.08);
                color.b *= (1.0 - r * 0.40);
                color += vec3(0.10, 0.02, 0.0) * r;
            }
        }
    }

    // 2. Retro CRT Shader Effects
    if (enableCRT > 0.5) {
        vec2 fromCenter = uv - 0.5;
        float dist = length(fromCenter);
        float caAmount = 0.005 * dist * (1.0 + sin(time * 1.5) * 0.1);
        
        float r = texture(texture0, clamp(uv - fromCenter * caAmount, 0.0, 1.0)).r;
        float g = color.g;
        float b = texture(texture0, clamp(uv + fromCenter * caAmount, 0.0, 1.0)).b;
        color = vec3(mix(color.r, r, 0.5), g, mix(color.b, b, 0.5));
        
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
    }

    finalColor = vec4(color, 1.0);
}