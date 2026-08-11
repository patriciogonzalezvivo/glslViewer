# glslViewer / VERA — Optimization & Roadmap

Running list of things to improve in glslViewer and the VERA engine it sits on.

---

## Unified glTF loading: meshes + point clouds + gaussian splats

**Goal:** load a single glTF/GLB that mixes triangle meshes, point clouds, and
Gaussian splats — in any combination — and render them together correctly.

### Why it's mostly already possible

All three types are already representable by one abstraction, `vera::Model`, held
in the `Scene::models` map, and the renderer already composites them when mixed:

- **Mesh** — `Model` wraps a mesh VBO; define `MODEL_PRIMITIVE_TRIANGLES`.
- **Point cloud** — a `Mesh` with `POINTS` draw mode; define `MODEL_PRIMITIVE_POINTS`.
  `extractMode()` in `deps/vera/src/io/gltf.cpp` already maps glTF `mode==POINTS`,
  so point clouds from glTF already load.
- **Gaussian splat** — `Model` wraps a `Gsplat*` (under `SUPPORT_GSPLAT`); define
  `MODEL_PRIMITIVE_GSPLATS`. The two-pass render (opaque first, splats second),
  `renderDepth`/`renderNormal`, and depth ordering are already in place from the
  multi-geometry work.

So there is **no new top-level type to invent**. The work is entirely: (1) classify
each glTF *primitive* into one of the three, and (2) let `Gsplat` ingest splat data
that came from a glTF buffer instead of a `.splat`/`.ply` file.

### glTF-splat spec landscape (evolving — build a tolerant reader)

- **Khronos draft `KHR_gaussian_splatting`** — future-proof target. Splats as a
  primitive extension carrying position + scale + rotation (covariance) + opacity + SH.
- **Current exporter convention** (three.js / PlayCanvas / SuperSplat, etc.) — often a
  `POINTS` primitive with custom attributes (`_SCALE`, `_ROTATION`, `_OPACITY`, SH in
  `COLOR_0` / `_SH_*`).
- **`KHR_spz_gaussian_splats_compression` (SPZ)** — compression layer; needs a decoder
  dependency → later phase.

tinygltf does not understand these semantically — parse `primitive.extensions` / the
custom attributes **manually and defensively**, exactly like the existing
`KHR_lights_punctual` handling in `gltf.cpp`.

### Plan

1. **Refactor `Gsplat` to separate ingest from file parsing.** Today `loadSPLAT`/
   `loadPLY` fill `m_positions/m_scales/m_rotations/m_colors` inline. Extract a public
   ingest entry so `.splat`, `.ply`-3DGS, and glTF all feed the same path:
   ```cpp
   struct GsplatData {
       std::vector<glm::vec3>    positions, scales;
       std::vector<glm::quat>    rotations;
       std::vector<glm::u8vec4>  colors;      // + optional higher-order SH
   };
   bool Gsplat::set(const GsplatData&, const glm::mat4& xform, FrameConvention);
   ```
   This is the single highest-leverage change.

2. **Classify per primitive in `extractMesh`:**
   - has splat extension/attributes → decode accessors into `GsplatData` → `new Model(name, gsplat)`
   - `mode==POINTS` → point-cloud mesh (already works)
   - else → triangle mesh (already works)

   `Model(name, Gsplat*)` already exists, so the models map stays homogeneous and the
   `MODEL_PRIMITIVE_*` shader branching is automatic.

3. **Coordinate frame — fix the smell.** Splat positions **and rotations** must be
   transformed by the node's world matrix (`_matrix`, like mesh verts get `_matrix * pos`),
   and glTF is Y-up so the COLMAP 180°-about-X flip must **not** apply. Today that flip is a
   global static `Gsplat::s_useColmapFrame` baked at load — wrong home (frame is per-source:
   glTF-node vs COLMAP vs standalone `.splat`). Make the frame a **parameter of `set()`**,
   not a global.

4. **Graceful degradation.** If `SUPPORT_GSPLAT` is off, or SH/covariance can't be built,
   fall back to rendering splat **centers as a `POINTS` point cloud** (`COLOR_0`).

5. **Gate + tolerate.** Keep behind `SUPPORT_GSPLAT`; parse the extension manually and
   skip malformed primitives instead of crashing (same discipline as the `addCameras`
   bounds fix).

### Combined-file correctness (must-haves)

- **Unique per-primitive keys.** glTF keys models by mesh name; a single mesh with
  multiple primitives of different types would collide in the map (pre-existing sharp
  edge). Append a primitive index/type suffix so each primitive is its own `Model`.
- **Per-node transform + frame for every type**, threaded through `Gsplat::set()` rather
  than the global static, so mixed content in one file stays in a consistent frame.

### Phasing

- **Phase 1 (core):** ingest refactor + per-primitive classification + raw-attribute
  reader (SH degree 0 / RGBA) + node-matrix transform + point-cloud fallback. Gets
  meshes + point clouds + degree-0 splats from a single glTF working.
- **Phase 2:** Khronos `KHR_gaussian_splatting` extension proper + higher-order SH in the
  splat shader (view-dependent color — real shader work).
- **Phase 3:** SPZ (`KHR_spz_...`) compression decode.

### Gotchas

- **SH support:** `.splat` and `Gsplat::loadPLY` are degree 0 (`f_dc_*`); higher-degree SH
  is genuine splat-shader work → Phase 2.
- **Covariance representation:** glTF may give scale+rotation *or* packed covariance; the
  ingest struct should accept scale+quat (what `Gsplat` already uses) and convert if needed.
- **Frame consistency** across mixed content — solved by threading matrix + frame policy
  through `set()`.

### Validation

Hand-author a `.glb` containing all three primitive types (a triangle mesh + a `POINTS`
primitive + a splat primitive) and confirm it loads as N coexisting `Model`s and renders
with correct depth ordering (headless screenshot).

### Relevant code

- `deps/vera/src/io/gltf.cpp` — `extractMesh` / `extractNodes` (per-primitive site);
  `extractMode` (POINTS handling); `KHR_lights_punctual` (manual-extension pattern).
- `deps/vera/src/types/gsplat.cpp` / `include/vera/types/gsplat.h` — `load`/`loadSPLAT`/
  `loadPLY`, the `m_positions/…` arrays, `s_useColmapFrame`.
- `deps/vera/src/types/model.cpp` — `Model(name, Gsplat*)` vs `Model(name, Mesh)`.
- `src/core/sceneRender.cpp` — two-pass render, shadow/normal/position passes.

---

## Camera animation commands (play until a mouse gesture)

**Goal:** add commands that continuously animate the camera until the user does a
mouse gesture (click / drag / scroll), at which point manual interaction takes over.

### Reuse — almost all the plumbing already exists

- **Per-frame hook + easing pattern:** `GlslViewer::updateCameraTransition()` (called
  once per frame from `renderPrep()`) already advances an animation with
  `vera::getDelta()`, eases it, calls `applyCameraMatrixUniforms()` + `vera::flagChange()`,
  and stops on a flag. The new animator is a sibling of it.
- **Gesture cancellation is already the pattern:** `onMousePress/onMouseDrag/onScroll`
  set `m_camera_transitioning = false` to "take over." The animator just needs the same
  handlers to also clear its mode. → "animate until a gesture" falls out for free.
- **Camera move primitives already in `vera::Node`:**
  `truck(x)` (X translate), `boom(y)` (Y translate = *pedestal*), `dolly(z)` (Z translate),
  `pan(deg)` (yaw / Y), `tilt(deg)` (pitch / X), `roll(deg)` (roll / Z), plus `translate`,
  `rotate`, `lookAt`. And `Camera::orbit(az, el, dist)` / `getOrbitAngles()` for the
  target-anchored moves. So no new math primitives are needed.
- **Continuous rendering:** the animator must `vera::flagChange()` every active frame so
  the app keeps redrawing (same as `updateCameraTransition`).

### Command set & semantics

Two behavior families:

- **Continuous** (monotonic, wraps):
  - `camera,orbit` — spin around the target: hold current elevation + distance, advance
    azimuth from the *current* azimuth. Always anchored to the target (uses `orbit()`).

- **Ping-pong** (oscillate around the starting pose, back and forth; a splat/COLMAP-style
  smooth ease at the ends). Amplitude is split ± around the start (e.g. arc 180° → −90°
  first, then +90°):
  - `camera,arc,<deg=180>` — like orbit but oscillate azimuth within ±deg/2, anchored to target.
  - `camera,dolly,<min>,<max>` — oscillate distance between min/max, anchored to target.
  - `camera,truck,<dist>` — translate along camera **X** by ±dist/2 (position **and** target move → look direction unchanged; not re-anchored).
  - `camera,pedestal,<dist>` — same along camera **Y** (`Node::boom`).
  - `camera,pan,<angle>` — rotate view about local **Y** by ±angle/2 (position fixed; look direction pivots).
  - `camera,tilt,<angle>` — rotate view about local **X** by ±angle/2.
  - `camera,roll,<angle>` — rotate view about local **Z** by ±angle/2.

- **Control:**
  - `camera,speed,<number>` — global multiplier for **all** camera animations **and** the
    COLMAP named-camera transition (default 1.0).
  - `camera,stop` (or `camera,off`) — cancel any running camera animation.

### Design

- **State** (new members on `GlslViewer`):
  ```
  enum CameraAnim { CAM_NONE, CAM_ORBIT, CAM_ARC, CAM_DOLLY,
                    CAM_TRUCK, CAM_PEDESTAL, CAM_PAN, CAM_TILT, CAM_ROLL };
  CameraAnim m_cam_anim = CAM_NONE;
  float m_cam_anim_phase = 0.0f;          // advances by getDelta()*speed
  float m_cam_anim_amp = 0.0f, m_cam_anim_min = 0.0f, m_cam_anim_max = 0.0f;
  float m_cam_anim_speed = 1.0f;          // global; also scales COLMAP transition
  // captured base pose at start (drift-free):
  glm::vec3 m_cam_base_pos, m_cam_base_target;
  glm::quat m_cam_base_rot;
  float m_cam_base_az, m_cam_base_el, m_cam_base_dist;
  ```
- **Start:** on a `camera,<move>` command, capture the base pose from the *current*
  `activeCamera` (switch to `default` first, like the mouse handlers do, so we animate the
  interactive camera and don't mutate a loaded COLMAP camera), set mode + params, reset phase.
- **`updateCameraAnimation()`** (new; call from `renderPrep()` right after
  `updateCameraTransition()`): advance `m_cam_anim_phase += getDelta() * m_cam_anim_speed`,
  compute the offset **relative to the captured base pose** (reset-to-base then apply each
  frame → no float drift, exact ping-pong, clean stop-anywhere), then
  `applyCameraMatrixUniforms()` + `flagChange()`.
  - Oscillator: `s = sin(phase)` (smooth ease at extremes); start negative so arc goes −half
    first. Continuous orbit uses `az = baseAz + phase*rate` (wrap).
  - Anchored moves (orbit/arc/dolly): `activeCamera->orbit(az, el, dist)` from base + offset.
  - truck/pedestal: `pos = basePos + baseAxis * (amp*s)` and move target by the same delta.
  - pan/tilt/roll: `setOrientation(baseRot)` then `pan/tilt/roll(angle_offset)` (or compose a
    quaternion), position = basePos.
  - Keep `m_camera_azimuth/elevation` in sync (`getOrbitAngles`) so a subsequent gesture
    resumes seamlessly.
- **Cancellation:** in `onMousePress/onMouseDrag/onScroll`, set `m_cam_anim = CAM_NONE`
  (next to the existing `m_camera_transitioning = false`). Leaves the camera wherever it is.
- **Global speed coupling:** `updateCameraTransition()` currently does
  `m_camera_transition_time += vera::getDelta();` → change to `* m_cam_anim_speed` so
  `camera,speed` also affects the COLMAP transition.

### Integration points

- `src/core/glslViewer.cpp`: new `updateCameraAnimation()`; call it in `renderPrep()` (~2158,
  next to `updateCameraTransition()`); extend the `camera` command handler (~1137) to parse the
  new subcommands + `speed`/`stop`; add cancel in `onMousePress/onMouseDrag/onScroll`
  (~2992/3031/2936); couple speed into `updateCameraTransition()` (~319).
- `src/core/glslViewer.h`: the state members above (near the existing
  `m_camera_transition_*` block, ~186).
- vera: no changes needed — `Node::truck/boom/dolly/pan/tilt/roll` and `Camera::orbit`
  already exist.

### Phasing

- **Phase 1:** state + `updateCameraAnimation()` + `camera,orbit` + `camera,arc` + the
  gesture-cancel + `camera,speed`/`camera,stop`, with speed coupled into the COLMAP transition.
- **Phase 2:** `dolly`, `truck`, `pedestal` (translation family).
- **Phase 3:** `pan`, `tilt`, `roll` (orientation family).

### Gotchas

- **Drift:** accumulate against a **captured base pose**, not frame-to-frame incremental
  `truck()/pan()` calls, or the oscillation slowly walks off over many cycles.
- **Continuous render:** must `flagChange()` each active frame or the animation stalls when
  the app is in its render-on-change idle mode.
- **Which camera:** animate `default` (switch like the mouse handlers) so a loaded COLMAP
  camera's pristine pose isn't mutated.
- **Resume after gesture:** keep `m_camera_azimuth/elevation` updated during anchored moves
  so a drag doesn't jump.
- **`camera,<id>` vs `camera,<move>`:** the existing `camera` command also selects named
  cameras — dispatch by keyword so a move verb isn't mistaken for a camera id.
