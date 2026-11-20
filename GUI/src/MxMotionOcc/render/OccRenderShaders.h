#pragma once

namespace MotionOcc::RenderShaders {

// Adapted from E:/codex/Motion/src/ui/cad/CadMeshRenderer.cpp:110–204, 214–298, 362–396.
// Only the two-sided CAD lighting and screen-space edge/outline math are
// reused; no donor document, animation, sectioning or UI dependencies.
inline constexpr char SurfaceVertex[] = R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uPivot;
uniform mat3 uNormal;
uniform bool uOutline;
uniform vec2 uViewportPixels;
uniform float uOutlinePixels;
out vec3 vNormal;
out vec3 vFlatPosition;
void main() {
    vec3 local = aPosition - uPivot;
    vec4 clip = uProjection * uView * vec4(local, 1.0);
    vNormal = uNormal * aNormal;
    // A common scale preserves the flat normal while keeping derivatives in
    // a useful range when the model's dimensions are large or small.
    vFlatPosition = (uNormal * local) * uProjection[1][1];
    if (uOutline) {
        vec2 direction = (uProjection * vec4(vNormal, 0.0)).xy * uViewportPixels;
        float span = length(direction);
        if (span > 0.0) clip.xy += (direction / span) * (2.0 * uOutlinePixels / uViewportPixels) * clip.w;
    }
    gl_Position = clip;
}
)GLSL";

inline constexpr char SurfaceFragment[] = R"GLSL(#version 330 core
in vec3 vNormal;
in vec3 vFlatPosition;
uniform vec3 uColor;
uniform bool uOutline;
uniform bool uFlat;
out vec4 fragColor;
void main() {
    if (uOutline) {
        fragColor = vec4(uColor, 1.0);
        return;
    }
    vec3 n;
    if (uFlat) {
        n = normalize(cross(normalize(dFdx(vFlatPosition)), normalize(dFdy(vFlatPosition))));
    } else {
        n = vNormal * inversesqrt(max(dot(vNormal, vNormal), 1e-12));
        if (!gl_FrontFacing) n = -n;
    }
    vec3 light = normalize(vec3(-0.35, 0.65, 0.9));
    float diffuse = max(dot(n, light), 0.0);
    vec3 halfVector = normalize(light + vec3(0.0, 0.0, 1.0));
    float specular = pow(max(dot(n, halfVector), 0.0), 32.0) * 0.16;
    fragColor = vec4(uColor * (0.38 + 0.62 * diffuse) + vec3(specular), 1.0);
}
)GLSL";

inline constexpr char EdgeVertex[] = R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uPivot;
void main() {
    gl_Position = uProjection * uView * vec4(aPosition - uPivot, 1.0);
}
)GLSL";

inline constexpr char EdgeGeometry[] = R"GLSL(#version 330 core
layout(lines) in;
layout(triangle_strip, max_vertices=4) out;
uniform vec2 uViewportPixels;
uniform float uWidthPixels;
uniform float uDepthBiasNdc;
noperspective out float vSide;
bool clipPlane(float da, float db, inout float enter, inout float leave) {
    if (da < 0.0 && db < 0.0) return false;
    if (da >= 0.0 && db >= 0.0) return true;
    float crossing = da / (da - db);
    if (da < 0.0) enter = max(enter, crossing); else leave = min(leave, crossing);
    return enter <= leave;
}
void emitCorner(vec4 p, vec2 offset, float halfWidth) {
    p.xy += offset * p.w;
    p.z = max(-p.w, p.z - uDepthBiasNdc * p.w);
    gl_Position = p;
    vSide = halfWidth;
    EmitVertex();
}
void main() {
    vec4 a = gl_in[0].gl_Position, b = gl_in[1].gl_Position;
    float enter = 0.0, leave = 1.0;
    // Clip the depth interval before biasing, and keep only strokes near the
    // frame. The small XY margin lets wide strokes touch the viewport edge.
    vec2 margin = vec2(1.0) + (uWidthPixels + 1.0) / uViewportPixels;
    if (!clipPlane(a.w * margin.x + a.x, b.w * margin.x + b.x, enter, leave) ||
        !clipPlane(a.w * margin.x - a.x, b.w * margin.x - b.x, enter, leave) ||
        !clipPlane(a.w * margin.y + a.y, b.w * margin.y + b.y, enter, leave) ||
        !clipPlane(a.w * margin.y - a.y, b.w * margin.y - b.y, enter, leave) ||
        !clipPlane(a.w + a.z, b.w + b.z, enter, leave) ||
        !clipPlane(a.w - a.z, b.w - b.z, enter, leave) ||
        !clipPlane(a.w - 1e-6, b.w - 1e-6, enter, leave)) return;
    vec4 first = mix(a, b, enter), second = mix(a, b, leave);
    vec2 d = (second.xy / second.w - first.xy / first.w) * uViewportPixels * 0.5;
    float span = length(d);
    if (span < 1e-5) return;
    float extent = uWidthPixels * 0.5 + 0.5;
    vec2 side = vec2(-d.y, d.x) * (2.0 * extent / span) / uViewportPixels;
    emitCorner(first, -side, -extent);
    emitCorner(first,  side,  extent);
    emitCorner(second, -side, -extent);
    emitCorner(second,  side,  extent);
    EndPrimitive();
}
)GLSL";

inline constexpr char EdgeFragment[] = R"GLSL(#version 330 core
uniform vec4 uColor;
uniform float uWidthPixels;
noperspective in float vSide;
out vec4 fragColor;
void main() {
    float coverage = clamp(uWidthPixels * 0.5 + 0.5 - abs(vSide), 0.0, 1.0);
    if (coverage <= 0.0) discard;
    fragColor = vec4(uColor.rgb, uColor.a * coverage);
}
)GLSL";

inline constexpr char BackdropVertex[] = R"GLSL(#version 330 core
out float vHeight;
void main() {
    const vec2 corners[3] = vec2[3](vec2(-1.0,-1.0), vec2(3.0,-1.0), vec2(-1.0,3.0));
    vec2 p = corners[gl_VertexID];
    gl_Position = vec4(p, 0.0, 1.0);
    vHeight = 0.5 * p.y + 0.5;
}
)GLSL";

inline constexpr char BackdropFragment[] = R"GLSL(#version 330 core
in float vHeight;
uniform vec3 uBottom;
uniform vec3 uTop;
out vec4 fragColor;
void main() {
    fragColor = vec4(mix(uBottom, uTop, clamp(vHeight, 0.0, 1.0)), 1.0);
}
)GLSL";

} // namespace MotionOcc::RenderShaders
