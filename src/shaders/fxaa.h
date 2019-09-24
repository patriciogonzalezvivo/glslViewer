#pragma once

#include <string>

const std::string fxaa_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform sampler2D   u_scene;\n\
uniform vec2        u_resolution;\n\
\n\
#ifndef FXAA_PATCHES\n\
#define FXAA_PATCHES 0\n\
#endif\n\
\n\
#ifndef MEDIUMP_FLT_MIN\n\
#define MEDIUMP_FLT_MIN     0.00006103515625\n\
#endif\n\
\n\
#ifndef FXAA_SPAN_MAX\n\
#define FXAA_SPAN_MAX 8.0\n\
#endif\n\
\n\
#ifndef FXAA_REDUCE_MUL\n\
#define FXAA_REDUCE_MUL   (1.0/FXAA_SPAN_MAX)\n\
#endif\n\
\n\
#ifndef FXAA_REDUCE_MIN\n\
#define FXAA_REDUCE_MIN   (1.0/128.0)\n\
#endif\n\
\n\
#ifndef FXAA_SUBPIX_SHIFT\n\
#define FXAA_SUBPIX_SHIFT (1.0/4.0)\n\
#endif\n\
\n\
vec4 fxaa(\n\
    vec2 pos,\n\
    vec4 fxaaConsolePosPos,\n\
    sampler2D tex,\n\
    vec2 fxaaConsoleRcpFrameOpt,\n\
    vec2 fxaaConsoleRcpFrameOpt2,\n\
    float fxaaConsoleEdgeSharpness,\n\
    float fxaaConsoleEdgeThreshold,\n\
    float fxaaConsoleEdgeThresholdMin\n\
) {\n\
    float lumaNw = texture2D(tex, fxaaConsolePosPos.xy).y;\n\
    float lumaSw = texture2D(tex, fxaaConsolePosPos.xw).y;\n\
    float lumaNe = texture2D(tex, fxaaConsolePosPos.zy).y;\n\
    float lumaSe = texture2D(tex, fxaaConsolePosPos.zw).y;\n\
\n\
    vec4 rgbyM = texture2D(tex, pos.xy);\n\
    float lumaM = rgbyM.y;\n\
    float lumaMaxNwSw = max(lumaNw, lumaSw);\n\
#   if FXAA_PATCHES == 0\n\
        lumaNe += 1.0 / 384.0;\n\
#   endif\n\
    float lumaMinNwSw = min(lumaNw, lumaSw);\n\
    float lumaMaxNeSe = max(lumaNe, lumaSe);\n\
    float lumaMinNeSe = min(lumaNe, lumaSe);\n\
    float lumaMax = max(lumaMaxNeSe, lumaMaxNwSw);\n\
    float lumaMin = min(lumaMinNeSe, lumaMinNwSw);\n\
\n\
    float lumaMaxScaled = lumaMax * fxaaConsoleEdgeThreshold;\n\
\n\
    float lumaMinM = min(lumaMin, lumaM);\n\
    float lumaMaxScaledClamped = max(fxaaConsoleEdgeThresholdMin, lumaMaxScaled);\n\
\n\
    float lumaMaxM = max(lumaMax, lumaM);\n\
\n\
    float lumaMaxSubMinM = lumaMaxM - lumaMinM;\n\
\n\
    if (lumaMaxSubMinM < lumaMaxScaledClamped) { return rgbyM; }\n\
\n\
    float dirSwMinusNe = lumaSw - lumaNe;\n\
\n\
    float dirSeMinusNw = lumaSe - lumaNw;\n\
\n\
    vec2 dir = vec2(\n\
        dirSwMinusNe + dirSeMinusNw, // == (SW + SE) - (NE + NW) ~= S - N\n\
        dirSwMinusNe - dirSeMinusNw  // == (SW + NW) - (SE + NE) ~= W - E\n\
    );\n\
\n\
    float dirLength = length(dir.xy);\n\
    if (dirLength < MEDIUMP_FLT_MIN) { return rgbyM; }\n\
\n\
    vec2 dir1 = dir.xy / dirLength;\n\
\n\
    vec4 rgbyN1 = texture2D(tex, pos.xy - dir1 * fxaaConsoleRcpFrameOpt.xy);\n\
    vec4 rgbyP1 = texture2D(tex, pos.xy + dir1 * fxaaConsoleRcpFrameOpt.xy);\n\
\n\
#   if FXAA_PATCHES\n\
        float dirAbsMinTimesC = max(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness * 0.015;\n\
        vec2 dir2 = dir1.xy * min(lumaMaxSubMinM / dirAbsMinTimesC, 3.0);\n\
#   else\n\
        float dirAbsMinTimesC = min(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness;\n\
        vec2 dir2 = clamp(dir1.xy / dirAbsMinTimesC, vec2(-2.0), vec2(2.0));\n\
#   endif\n\
    vec4 rgbyN2 = texture2D(tex, pos.xy - dir2 * fxaaConsoleRcpFrameOpt2.xy);\n\
    vec4 rgbyP2 = texture2D(tex, pos.xy + dir2 * fxaaConsoleRcpFrameOpt2.xy);\n\
\n\
    vec4 rgbyA = rgbyN1 + rgbyP1;\n\
    vec4 rgbyB = ((rgbyN2 + rgbyP2) * 0.25) + (rgbyA * 0.25);\n\
    bool twoTap = (rgbyB.y < lumaMin || rgbyB.y > lumaMax);\n\
    if (twoTap) { rgbyB.xyz = rgbyA.xyz * 0.5; }\n\
\n\
#   if FXAA_PATCHES\n\
        rgbyB = mix(rgbyB, rgbyM, 0.25); // Faster\n\
#   endif\n\
    return rgbyB;\n\
}\n\
\n\
vec4 fxaa(sampler2D tex, vec2 st) {\n\
    vec2 fboSize = u_resolution;\n\
    vec2 invSize = 1.0 / fboSize;\n\
    vec2 halfTexel = 0.5 * invSize;\n\
    vec2 viewportSize = u_resolution.xy;\n\
\n\
    vec2 excessSize = 0.5 + fboSize - viewportSize;\n\
    vec2 upperBound = 1.0 - excessSize * invSize;\n\
\n\
    vec2 texelCenter = min(st, upperBound);\n\
    vec2 texelMaxCorner = min(st + halfTexel, upperBound);\n\
    vec2 texelMinCorner = st - halfTexel;\n\
\n\
    return fxaa(\n\
            texelCenter,\n\
            vec4(texelMinCorner, texelMaxCorner),\n\
            tex,\n\
            invSize,             // vec4 fxaaConsoleRcpFrameOpt,\n\
            2.0 * invSize,       // vec4 fxaaConsoleRcpFrameOpt2,\n\
            8.0,                 // float fxaaConsoleEdgeSharpness,\n\
#if defined(FXAA_PATCHES) && FXAA_PATCHES == 1\n\
            0.08,                // float fxaaConsoleEdgeThreshold,\n\
#else\n\
            0.125,               // float fxaaConsoleEdgeThreshold,\n\
#endif\n\
            0.04                 // float fxaaConsoleEdgeThresholdMin\n\
    );\n\
}\n\
\n\
void main(void) {\n\
    vec2 uv = gl_FragCoord.xy/u_resolution;\n\
    gl_FragColor = fxaa(u_scene, uv);\n\
}\n";
