#pragma once

#include <string>

const std::string poissonfill_frag = R"(
uniform sampler2D   u_tex0;
uniform sampler2D   u_tex1;
uniform vec2        u_resolution;
uniform bool        u_isup;
                        
float h1(int i) {
    if (i == 0 || i == 4)
        return 0.1507;
    
    if (i == 1 || i == 3)
        return 0.6836;

    return 1.0334;
}

float G(int i) {
    if (i == 0 || i == 2)
        return 0.0312;

    return 0.7753;
}

void main() {
    vec2 pixel = 1.0/u_resolution;

    int i = int(gl_FragCoord.y-0.5);
    int j = int(gl_FragCoord.x-0.5);

    if (!u_isup) {
        int x = j * 2;
        int y = i * 2;
        vec4 acc = vec4(0.0);
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                float nx = float(x + dx + 1);
                float ny = float(y + dy + 1);
                if (nx < 0.0 || nx >= u_resolution.x || ny < 0.0 || ny >= u_resolution.y)
                    continue;
                
                vec4 col = texture2D(u_tex0, vec2(nx, ny) * pixel);
                acc += col * h1(dx+2) * h1(dy+2);
            }
        }

        if (acc.a == 0.0) 
            gl_FragColor = acc;
        else
            gl_FragColor = vec4(acc.rgb/acc.a,1.0);
    }
    else {
        float h2 = 0.0270;
        vec4 acc = vec4(0.0);
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                float nx = float(j + dx + 1);
                float ny = float(i + dy + 1);
                if (nx < 0.0 || nx >= u_resolution.x || ny < 0.0 || ny >= u_resolution.y)
                    continue;

                vec4 col = texture2D(u_tex0, vec2(nx, ny) * pixel);
                acc += col * G(dx+1) * G(dy+1);
            }
        }
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                float nx = float(j + dx);
                float ny = float(i + dy);
                if (nx < 0.0 || nx >= u_resolution.x || ny < 0.0 || ny >= u_resolution.y)
                    continue;
                
                vec4 col = texture2D(u_tex1, vec2(nx, ny) * 0.5 * pixel);
                acc += col * h2 * h1(dx+2) * h1(dy+2);
            }
        }

        if (acc.a == 0.0)
            gl_FragColor = acc;
        else
            gl_FragColor = vec4(acc.rgb/acc.a,1.0);
    }
}
    )";