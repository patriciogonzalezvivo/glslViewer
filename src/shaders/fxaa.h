#pragma once

#include <string>

const std::string fxaa_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;

#ifndef FXAA_PATCHES
#define FXAA_PATCHES 0
#endif

#ifndef MEDIUMP_FLT_MIN
#define MEDIUMP_FLT_MIN     0.00006103515625
#endif

#ifndef FXAA_SPAN_MAX
#define FXAA_SPAN_MAX 8.0
#endif

#ifndef FXAA_REDUCE_MUL
#define FXAA_REDUCE_MUL   (1.0/FXAA_SPAN_MAX)
#endif

#ifndef FXAA_REDUCE_MIN
#define FXAA_REDUCE_MIN   (1.0/128.0)
#endif

#ifndef FXAA_SUBPIX_SHIFT
#define FXAA_SUBPIX_SHIFT (1.0/4.0)
#endif

vec4 fxaa(
    vec2 pos,
    vec4 fxaaConsolePosPos,
    sampler2D tex,
    vec2 fxaaConsoleRcpFrameOpt,
    vec2 fxaaConsoleRcpFrameOpt2,
    float fxaaConsoleEdgeSharpness,
    float fxaaConsoleEdgeThreshold,
    float fxaaConsoleEdgeThresholdMin
) {
    float lumaNw = texture2D(tex, fxaaConsolePosPos.xy).y;
    float lumaSw = texture2D(tex, fxaaConsolePosPos.xw).y;
    float lumaNe = texture2D(tex, fxaaConsolePosPos.zy).y;
    float lumaSe = texture2D(tex, fxaaConsolePosPos.zw).y;

    vec4 rgbyM = texture2D(tex, pos.xy);
    float lumaM = rgbyM.y;
    float lumaMaxNwSw = max(lumaNw, lumaSw);
#   if FXAA_PATCHES == 0
        lumaNe += 1.0 / 384.0;
#   endif
    float lumaMinNwSw = min(lumaNw, lumaSw);
    float lumaMaxNeSe = max(lumaNe, lumaSe);
    float lumaMinNeSe = min(lumaNe, lumaSe);
    float lumaMax = max(lumaMaxNeSe, lumaMaxNwSw);
    float lumaMin = min(lumaMinNeSe, lumaMinNwSw);

    float lumaMaxScaled = lumaMax * fxaaConsoleEdgeThreshold;

    float lumaMinM = min(lumaMin, lumaM);
    float lumaMaxScaledClamped = max(fxaaConsoleEdgeThresholdMin, lumaMaxScaled);

    float lumaMaxM = max(lumaMax, lumaM);

    float lumaMaxSubMinM = lumaMaxM - lumaMinM;

    if (lumaMaxSubMinM < lumaMaxScaledClamped) { return rgbyM; }

    float dirSwMinusNe = lumaSw - lumaNe;

    float dirSeMinusNw = lumaSe - lumaNw;

    vec2 dir = vec2(
        dirSwMinusNe + dirSeMinusNw, // == (SW + SE) - (NE + NW) ~= S - N
        dirSwMinusNe - dirSeMinusNw  // == (SW + NW) - (SE + NE) ~= W - E
    );

    float dirLength = length(dir.xy);
    if (dirLength < MEDIUMP_FLT_MIN) { return rgbyM; }

    vec2 dir1 = dir.xy / dirLength;

    vec4 rgbyN1 = texture2D(tex, pos.xy - dir1 * fxaaConsoleRcpFrameOpt.xy);
    vec4 rgbyP1 = texture2D(tex, pos.xy + dir1 * fxaaConsoleRcpFrameOpt.xy);

#   if FXAA_PATCHES
        float dirAbsMinTimesC = max(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness * 0.015;
        vec2 dir2 = dir1.xy * min(lumaMaxSubMinM / dirAbsMinTimesC, 3.0);
#   else
        float dirAbsMinTimesC = min(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness;
        vec2 dir2 = clamp(dir1.xy / dirAbsMinTimesC, vec2(-2.0), vec2(2.0));
#   endif
    vec4 rgbyN2 = texture2D(tex, pos.xy - dir2 * fxaaConsoleRcpFrameOpt2.xy);
    vec4 rgbyP2 = texture2D(tex, pos.xy + dir2 * fxaaConsoleRcpFrameOpt2.xy);

    vec4 rgbyA = rgbyN1 + rgbyP1;
    vec4 rgbyB = ((rgbyN2 + rgbyP2) * 0.25) + (rgbyA * 0.25);
    bool twoTap = (rgbyB.y < lumaMin || rgbyB.y > lumaMax);
    if (twoTap) { rgbyB.xyz = rgbyA.xyz * 0.5; }

#   if FXAA_PATCHES
        rgbyB = mix(rgbyB, rgbyM, 0.25); // Faster
#   endif
    return rgbyB;
}

vec4 fxaa(sampler2D tex, vec2 st) {
    vec2 fboSize = u_resolution;
    vec2 invSize = 1.0 / fboSize;
    vec2 halfTexel = 0.5 * invSize;
    vec2 viewportSize = u_resolution.xy;

    vec2 excessSize = 0.5 + fboSize - viewportSize;
    vec2 upperBound = 1.0 - excessSize * invSize;

    vec2 texelCenter = min(st, upperBound);
    vec2 texelMaxCorner = min(st + halfTexel, upperBound);
    vec2 texelMinCorner = st - halfTexel;

    return fxaa(
            texelCenter,
            vec4(texelMinCorner, texelMaxCorner),
            tex,
            invSize,             // vec4 fxaaConsoleRcpFrameOpt,
            2.0 * invSize,       // vec4 fxaaConsoleRcpFrameOpt2,
            8.0,                 // float fxaaConsoleEdgeSharpness,
#if defined(FXAA_PATCHES) && FXAA_PATCHES == 1
            0.08,                // float fxaaConsoleEdgeThreshold,
#else
            0.125,               // float fxaaConsoleEdgeThreshold,
#endif
            0.04                 // float fxaaConsoleEdgeThresholdMin
    );
}

void main(void) {
    vec2 uv = gl_FragCoord.xy/u_resolution;
    gl_FragColor = fxaa(u_scene, uv);
})";



const std::string fxaa_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;
out     vec4        fragColor;

#ifndef FXAA_PATCHES
#define FXAA_PATCHES 0
#endif

#ifndef MEDIUMP_FLT_MIN
#define MEDIUMP_FLT_MIN     0.00006103515625
#endif

#ifndef FXAA_SPAN_MAX
#define FXAA_SPAN_MAX 8.0
#endif

#ifndef FXAA_REDUCE_MUL
#define FXAA_REDUCE_MUL   (1.0/FXAA_SPAN_MAX)
#endif

#ifndef FXAA_REDUCE_MIN
#define FXAA_REDUCE_MIN   (1.0/128.0)
#endif

#ifndef FXAA_SUBPIX_SHIFT
#define FXAA_SUBPIX_SHIFT (1.0/4.0)
#endif

vec4 fxaa(
    vec2 pos,
    vec4 fxaaConsolePosPos,
    sampler2D tex,
    vec2 fxaaConsoleRcpFrameOpt,
    vec2 fxaaConsoleRcpFrameOpt2,
    float fxaaConsoleEdgeSharpness,
    float fxaaConsoleEdgeThreshold,
    float fxaaConsoleEdgeThresholdMin
) {
    float lumaNw = texture(tex, fxaaConsolePosPos.xy).y;
    float lumaSw = texture(tex, fxaaConsolePosPos.xw).y;
    float lumaNe = texture(tex, fxaaConsolePosPos.zy).y;
    float lumaSe = texture(tex, fxaaConsolePosPos.zw).y;

    vec4 rgbyM = texture(tex, pos.xy);
    float lumaM = rgbyM.y;
    float lumaMaxNwSw = max(lumaNw, lumaSw);
#   if FXAA_PATCHES == 0
        lumaNe += 1.0 / 384.0;
#   endif
    float lumaMinNwSw = min(lumaNw, lumaSw);
    float lumaMaxNeSe = max(lumaNe, lumaSe);
    float lumaMinNeSe = min(lumaNe, lumaSe);
    float lumaMax = max(lumaMaxNeSe, lumaMaxNwSw);
    float lumaMin = min(lumaMinNeSe, lumaMinNwSw);

    float lumaMaxScaled = lumaMax * fxaaConsoleEdgeThreshold;

    float lumaMinM = min(lumaMin, lumaM);
    float lumaMaxScaledClamped = max(fxaaConsoleEdgeThresholdMin, lumaMaxScaled);

    float lumaMaxM = max(lumaMax, lumaM);

    float lumaMaxSubMinM = lumaMaxM - lumaMinM;

    if (lumaMaxSubMinM < lumaMaxScaledClamped) { return rgbyM; }

    float dirSwMinusNe = lumaSw - lumaNe;

    float dirSeMinusNw = lumaSe - lumaNw;

    vec2 dir = vec2(
        dirSwMinusNe + dirSeMinusNw, // == (SW + SE) - (NE + NW) ~= S - N
        dirSwMinusNe - dirSeMinusNw  // == (SW + NW) - (SE + NE) ~= W - E
    );

    float dirLength = length(dir.xy);
    if (dirLength < MEDIUMP_FLT_MIN) { return rgbyM; }

    vec2 dir1 = dir.xy / dirLength;

    vec4 rgbyN1 = texture(tex, pos.xy - dir1 * fxaaConsoleRcpFrameOpt.xy);
    vec4 rgbyP1 = texture(tex, pos.xy + dir1 * fxaaConsoleRcpFrameOpt.xy);

#   if FXAA_PATCHES
        float dirAbsMinTimesC = max(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness * 0.015;
        vec2 dir2 = dir1.xy * min(lumaMaxSubMinM / dirAbsMinTimesC, 3.0);
#   else
        float dirAbsMinTimesC = min(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness;
        vec2 dir2 = clamp(dir1.xy / dirAbsMinTimesC, vec2(-2.0), vec2(2.0));
#   endif
    vec4 rgbyN2 = texture(tex, pos.xy - dir2 * fxaaConsoleRcpFrameOpt2.xy);
    vec4 rgbyP2 = texture(tex, pos.xy + dir2 * fxaaConsoleRcpFrameOpt2.xy);

    vec4 rgbyA = rgbyN1 + rgbyP1;
    vec4 rgbyB = ((rgbyN2 + rgbyP2) * 0.25) + (rgbyA * 0.25);
    bool twoTap = (rgbyB.y < lumaMin || rgbyB.y > lumaMax);
    if (twoTap) { rgbyB.xyz = rgbyA.xyz * 0.5; }

#   if FXAA_PATCHES
        rgbyB = mix(rgbyB, rgbyM, 0.25); // Faster
#   endif
    return rgbyB;
}

vec4 fxaa(sampler2D tex, vec2 st) {
    vec2 fboSize = u_resolution;
    vec2 invSize = 1.0 / fboSize;
    vec2 halfTexel = 0.5 * invSize;
    vec2 viewportSize = u_resolution.xy;

    vec2 excessSize = 0.5 + fboSize - viewportSize;
    vec2 upperBound = 1.0 - excessSize * invSize;

    vec2 texelCenter = min(st, upperBound);
    vec2 texelMaxCorner = min(st + halfTexel, upperBound);
    vec2 texelMinCorner = st - halfTexel;

    return fxaa(
            texelCenter,
            vec4(texelMinCorner, texelMaxCorner),
            tex,
            invSize,             // vec4 fxaaConsoleRcpFrameOpt,
            2.0 * invSize,       // vec4 fxaaConsoleRcpFrameOpt2,
            8.0,                 // float fxaaConsoleEdgeSharpness,
#if defined(FXAA_PATCHES) && FXAA_PATCHES == 1
            0.08,                // float fxaaConsoleEdgeThreshold,
#else
            0.125,               // float fxaaConsoleEdgeThreshold,
#endif
            0.04                 // float fxaaConsoleEdgeThresholdMin
    );
}

void main(void) {
    vec2 uv = gl_FragCoord.xy/u_resolution;
    fragColor = fxaa(u_scene, uv);
})";
