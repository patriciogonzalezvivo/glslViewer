# Uniforms

glslViewer injects these uniforms automatically. **They are only fed when your
shader actually declares/uses them** — glslViewer scans the shader source and
sets just the uniforms it references (some, like the scene G-buffers and the
shadow map, also enable extra render passes when detected). Declare the ones you
need with the exact name and type below.

Runtime values can be inspected/overridden with the `uniforms`, `time`, `mouse`,
`date`, `delta`, `light_*`, `camera_*` commands — see [COMMANDS.md](COMMANDS.md).

## Time & frame

| Uniform | Type | Description |
|---|---|---|
| `u_time` | `float` | Elapsed time in seconds. |
| `u_delta` | `float` | Seconds since the previous frame. |
| `u_frame` | `int` | Frame counter. |
| `u_date` | `vec4` | Year, month, day, seconds-into-day. |
| `u_sceneFps` | `float` | Current frames per second. |
| `u_sceneMs` | `float` | Frame time in milliseconds. |

## Viewport & input

| Uniform | Type | Description |
|---|---|---|
| `u_resolution` | `vec2` | Viewport size in pixels. |
| `u_mouse` | `vec2` | Mouse position in pixels. |
| `u_pixelDensity` | `float` | Display pixel density. |
| `u_view2d` | `mat3` | 2D pan/zoom transform (mouse-driven in 2D mode). |

## Input textures & streams

| Uniform | Type | Description |
|---|---|---|
| `u_tex0`, `u_tex1`, … | `sampler2D` | Textures passed positionally on the command line. |
| `u_<name>` | `sampler2D` | Texture loaded with a custom name (`-<name> file`). |
| `u_<name>Resolution` | `vec2` | Resolution companion for each texture. |
| `u_<name>CurrentFrame` | `float` | Current frame of a streaming texture (video/sequence). |
| `u_<name>TotalFrames` | `float` | Total frames of a streaming texture. |
| `u_<name>Time` | `float` | Playback time of a stream. |
| `u_<name>Duration` | `float` | Duration of a stream. |
| `u_<name>Fps` | `float` | FPS of a stream. |

## Camera

| Uniform | Type | Description |
|---|---|---|
| `u_camera` | `vec3` | Camera position (world). |
| `u_cameraTarget` | `vec3` | Camera target/look-at point. |
| `u_cameraDistance` | `float` | Distance from camera to target. |
| `u_cameraNearClip` | `float` | Near clip plane. |
| `u_cameraFarClip` | `float` | Far clip plane. |
| `u_cameraFov` | `float` | Field of view. |
| `u_cameraChange` | `bool` | True on frames where the camera moved. |
| `u_cameraEv100` | `float` | Exposure value at ISO 100. |
| `u_cameraExposure` | `float` | Exposure multiplier. |
| `u_cameraAperture` | `float` | Aperture (f-stop). |
| `u_cameraShutterSpeed` | `float` | Shutter speed. |
| `u_cameraSensitivity` | `float` | ISO sensitivity. |

## Matrices

| Uniform | Type | Description |
|---|---|---|
| `u_modelMatrix` | `mat4` | Model → world. |
| `u_viewMatrix` | `mat4` | World → view. |
| `u_inverseViewMatrix` | `mat4` | View → world. |
| `u_projectionMatrix` | `mat4` | View → clip. |
| `u_inverseProjectionMatrix` | `mat4` | Clip → view. |
| `u_modelViewProjectionMatrix` | `mat4` | Combined MVP (also used by the shadow/depth passes). |
| `u_normalMatrix` | `mat3` | Normal matrix (inverse-transpose of the model-view). |
| `u_model` | `vec3` | Model origin/offset (scene + floor origin). |

## Lights

Single-light scenes expose `u_light*`. With multiple lights, the primary light is
still `u_light*` and the rest follow `u_light1*`, `u_light2*`, … (the key of each
light in the scene becomes its uniform prefix).

| Uniform | Type | Description |
|---|---|---|
| `u_light` | `vec3` | Light position. |
| `u_lightColor` | `vec3` | Light color. |
| `u_lightDirection` | `vec3` | Light direction (directional/spot). |
| `u_lightIntensity` | `float` | Light intensity. |
| `u_lightFalloff` | `float` | Falloff distance (when > 0). |
| `u_lightMatrix` | `mat4` | Biased light-space MVP for shadow lookups. |
| `u_lightShadowMap` | `sampler2D` | Shadow-map depth texture. Declaring it enables the shadow pass. |

## Environment (image-based lighting)

| Uniform | Type | Description |
|---|---|---|
| `u_cubeMap` | `samplerCube` | Environment cubemap (`-c` / `-C`). |
| `u_SH` | `vec3[9]` | Spherical-harmonics coefficients of the environment (`-sh`). |
| `u_iblLuminance` | `float` | IBL luminance scale. |

## Scene G-buffers & render passes

Declaring one of these makes glslViewer render the corresponding extra pass.

| Uniform | Type | Description |
|---|---|---|
| `u_scene` | `sampler2D` | The rendered scene color (post-processing input). |
| `u_sceneDepth` | `sampler2D` | Scene depth buffer. |
| `u_sceneNormal` | `sampler2D` | Scene view-space normal G-buffer. |
| `u_scenePosition` | `sampler2D` | Scene position G-buffer. |
| `u_sceneBuffer0`, `u_sceneBuffer1`, … | `sampler2D` | Extra scene render targets (per-model multi-pass). |

## Ping-pong / multi-pass buffers

Authored via `#ifdef`/naming conventions in the fragment shader — see
[DEFINES.md](DEFINES.md).

| Uniform | Type | Description |
|---|---|---|
| `u_buffer0`, `u_buffer1`, … | `sampler2D` | Single-frame buffers (`BUFFER_N`). |
| `u_doubleBuffer0`, … | `sampler2D` | Ping-pong / feedback buffers (`DOUBLE_BUFFER_N`). |
| `u_pyramid0`, … | `sampler2D` | Convolution-pyramid buffers (`PYRAMID_N`). |
| `u_flood0`, … | `sampler2D` | Jump-flood buffers (`FLOOD_N`). |

## Notes

- The exact set of active uniforms is what `uniforms,active` reports at runtime.
- Model/geometry attributes (`a_position`, `a_normal`, `a_texcoord`, …) and the
  `MODEL_*`/`MATERIAL_*` branches that gate them are documented in
  [DEFINES.md](DEFINES.md).
