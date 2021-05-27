#pragma once

#include <string>

const std::string poissonfill_frag = R"(
uniform sampler2D   u_texUnf;
uniform sampler2D   u_texFil;
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
        vec4 acc = vec4(0.0,0.0,0.0,0.0);
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                int nx = x + dx;
                int ny = y + dy;
                // if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
                //   continue;
                // }
                vec4 col = texture2D(u_texUnf, vec2(float(nx)+1.0, float(ny)+1.0) * pixel);
                acc.r += h1(dx+2) * h1(dy+2) * col.r;
                acc.g += h1(dx+2) * h1(dy+2) * col.g;
                acc.b += h1(dx+2) * h1(dy+2) * col.b;
                acc.a += h1(dx+2) * h1(dy+2) * col.a;
            }
        }
        if (acc.a == 0.0) 
            gl_FragColor = acc;
        else
            gl_FragColor = vec4(acc.r/acc.a,acc.g/acc.a,acc.b/acc.a,1.0);
        
        
    }
    else {
        float h2 = 0.0270;
        vec4 acc = vec4(0.0,0.0,0.0,0.0);
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int nx = j + dx;
                int ny = i + dy;
                // if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
                //   continue;
                // }
                vec4 col = texture2D(u_texUnf, vec2(float(nx)+1.0, float(ny)+1.0) * pixel);
                acc.r += G(dx+1) * G(dy+1) * col.r;
                acc.g += G(dx+1) * G(dy+1) * col.g;
                acc.b += G(dx+1) * G(dy+1) * col.b;
                acc.a += G(dx+1) * G(dy+1) * col.a;
            }
        }
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                int nx = j + dx;
                int ny = i + dy;
                nx /= 2;
                ny /= 2;
                // if (nx < 0 || nx >= w/2 || ny < 0 || ny >= h/2) {
                //   continue;
                // }
                vec4 col = texture2D(u_texFil, vec2(float(nx), float(ny)) * pixel);
                acc.r += h2 * h1(dx+2) * h1(dy+2) * col.r;
                acc.g += h2 * h1(dx+2) * h1(dy+2) * col.g;
                acc.b += h2 * h1(dx+2) * h1(dy+2) * col.b;
                acc.a += h2 * h1(dx+2) * h1(dy+2) * col.a;
            }
        }
        if (acc.a == 0.0)
            gl_FragColor = acc;
        else
            gl_FragColor = vec4(acc.r/acc.a,acc.g/acc.a,acc.b/acc.a,1.0);
        
    }
}
    )";