# Define Flags

`#define` flags gate `#ifdef` branches in the shaders. There are three sources:

1. **Automatic** — glslViewer adds them from the loaded geometry, material,
   environment and scene state (e.g. `MODEL_*`, `MATERIAL_*`, `SCENE_*`,
   `LIGHT_SHADOWMAP`, `FLOOR*`, `QUILT*`, `GLSLVIEWER`).
2. **Authored conventions** — you write the branch/uniform and glslViewer detects
   it in the source and wires up the pass (e.g. `BUFFER_N`, `DOUBLE_BUFFER_N`,
   `PYRAMID_N`, `FLOOD_N`, `POSTPROCESSING`, `BACKGROUND`).
3. **Manual** — via `-D<define>` on the command line, or the `define,<KEYWORD>[,<VALUE>]`
   / `undefine,<KEYWORD>` commands. `defines` lists the active set.

Inspect the active flags at runtime with the `defines` command.

## General (automatic)

| Define | Meaning |
|---|---|
| `GLSLVIEWER` | Set to the version (e.g. `340`); use to detect glslViewer. |
| `DEBUG` | Debug build/mode. |
| `PLATFORM_WEBXR` | Building/running for WebXR. |

## Model geometry (automatic, from the loaded mesh)

Set per model based on the attributes and primitive type present. The value of
the `MODEL_VERTEX_*` flags is the varying name to read in the shader.

| Define | Meaning |
|---|---|
| `MODEL_VERTEX_POSITION` | Position attribute present (→ `v_position`). |
| `MODEL_VERTEX_COLOR` | Vertex colors present (→ `v_color`). |
| `MODEL_VERTEX_NORMAL` | Normals present (→ `v_normal`). |
| `MODEL_VERTEX_TANGENT` | Tangents present (→ `v_tangent`). |
| `MODEL_VERTEX_TEXCOORD` | UVs present (→ `v_texcoord`). |
| `MODEL_PRIMITIVE_POINTS` | Point-cloud primitive. |
| `MODEL_PRIMITIVE_LINES` / `_LINE_LOOP` / `_LINE_STRIP` | Line primitives. |
| `MODEL_PRIMITIVE_TRIANGLES` / `_TRIANGLE_FAN` | Triangle primitives. |
| `MODEL_PRIMITIVE_GSPLATS` | Gaussian-splat primitive. |
| `MODEL_NAME_<NAME>` | One per model, from its (namespaced, uppercased) name — branch per geometry. |
| `MODEL_SDF_TEXTURE` | Name of a generated SDF texture (see `generate_sdf`). |
| `MODEL_SDF_TEXTURE_RESOLUTION` | SDF texture resolution. |
| `MODEL_SDF_VOXEL_RESOLUTION` | SDF voxel resolution. |
| `MODEL_SDF_SCALE` | SDF scale. |

> **Combining geometries:** loading several files (or a multi-primitive glTF)
> gives each model a distinct `MODEL_NAME_*` (all caps), so one shared shader can
> branch per object.

## Materials (automatic, from glTF/OBJ)

Each material sets `MATERIAL_NAME_<NAME>` plus a flag per channel it defines.
Channels follow a consistent pattern:

- `MATERIAL_<CHANNEL>` — a constant value is available.
- `MATERIAL_<CHANNEL>MAP` — a texture sampler is available.
- `MATERIAL_<CHANNEL>MAP_OFFSET` / `MATERIAL_<CHANNEL>MAP_SCALE` — UV transform for that map.

| Channel flags | Meaning |
|---|---|
| `MATERIAL_BASECOLOR` / `MATERIAL_BASECOLORMAP` | Base color (albedo). |
| `MATERIAL_EMISSIVE` / `MATERIAL_EMISSIVEMAP` | Emission. |
| `MATERIAL_SPECULAR` / `MATERIAL_SPECULARMAP` | Specular. |
| `MATERIAL_METALLIC` / `MATERIAL_METALLICMAP` | Metalness. |
| `MATERIAL_ROUGHNESS` / `MATERIAL_ROUGHNESSMAP` | Roughness. |
| `MATERIAL_NORMALMAP` | Normal map. |
| `MATERIAL_OCCLUSIONMAP` / `MATERIAL_OCCLUSIONMAP_STRENGTH` | Ambient occlusion. |
| `MATERIAL_ROUGHNESSMETALLICMAP` / `MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP` | Packed ORM maps. |
| `MATERIAL_AMBIENT` / `MATERIAL_AMBIENTMAP` | Ambient. |
| `MATERIAL_ALPHAMAP` | Alpha/opacity map. |
| `MATERIAL_BUMPMAP` / `MATERIAL_BUMPMAP_NORMALMAP` | Bump map. |
| `MATERIAL_DISPLACEMENTMAP` | Displacement map. |
| `MATERIAL_SHEEN` / `MATERIAL_SHEENMAP` | Sheen. |
| `MATERIAL_REFLECTIONMAP` | Reflection map. |
| `MATERIAL_IOR` | Index of refraction. |
| `MATERIAL_SHININESS` | Shininess (Phong). |
| `MATERIAL_DISSOLVE` | Dissolve / transparency. |
| `MATERIAL_ANISOTROPY` / `MATERIAL_ANISOTROPY_ROTATION` | Anisotropy. |
| `MATERIAL_CLEARCOAT_THICKNESS` / `MATERIAL_CLEARCOAT_ROUGHNESS` | Clear coat. |
| `MATERIAL_TRANSMITTANCE` | Transmittance. |
| `MATERIAL_ILLUM` | OBJ illumination model. |

## Lighting & shadows (automatic)

| Define | Meaning |
|---|---|
| `LIGHT_SHADOWMAP` | Value is the shadow-map sampler name (`u_lightShadowMap`); enables the shadow pass. |
| `LIGHT_SHADOWMAP_SIZE` | Shadow-map resolution. |
| `SUN` | A sun/directional light is present. |

## Scene & environment (automatic)

| Define | Meaning |
|---|---|
| `SCENE_CUBEMAP` | Value is the environment cubemap sampler (`u_cubeMap`). |
| `SCENE_SH_ARRAY` | Value is the SH array (`u_SH`). |
| `SCENE_BUFFER_0`, `SCENE_BUFFER_1`, … | Extra per-model scene render targets. |
| `FLOOR` | The floor is being rendered. |
| `FLOOR_AREA` | Floor area. |
| `FLOOR_HEIGHT` | Floor Y height. |
| `FLOOR_SUBD` | Floor subdivision level. |
| `FLOOR_COLOR` | Floor/ground color. |
| `BACKGROUND` | The fragment shader has a background pass (drawn behind the scene). |
| `POSTPROCESSING` | The fragment shader has a post-processing pass (consumes `u_scene`). |

## Multi-pass buffers (authored conventions)

Declare these in the fragment shader; glslViewer detects them and allocates the
matching FBO + `sampler2D` uniform (see [UNIFORMS.md](UNIFORMS.md)).

| Define | Buffer uniform | Meaning |
|---|---|---|
| `BUFFER_0`, `BUFFER_1`, … | `u_buffer0`, … | Single-frame render buffers. |
| `DOUBLE_BUFFER_0`, … | `u_doubleBuffer0`, … | Ping-pong / feedback buffers. |
| `PYRAMID_0`, … (`PYRAMID_ALGORITHM`) | `u_pyramid0`, … | Convolution-pyramid buffers. |
| `FLOOD_0`, … (`FLOOD_ALGORITHM`) | `u_flood0`, … | Jump-flood buffers. |

## DevLook debug widgets (automatic)

| Define | Meaning |
|---|---|
| `DEVLOOK_SPHERE_0`, … | Material-preview spheres. |
| `DEVLOOK_BILLBOARD_0`, … | Material-preview billboards. |
| `DEVLOOK_Y_OFFSET` | Vertical offset for the devlook widgets. |

## Holographic (automatic, quilt / Looking Glass)

| Define | Meaning |
|---|---|
| `QUILT` | Quilt rendering enabled (value = preset). |
| `QUILT_WIDTH` / `QUILT_HEIGHT` | Quilt dimensions. |
| `QUILT_COLUMNS` / `QUILT_ROWS` | Quilt tiling. |
| `QUILT_TOTALVIEWS` | Total views in the quilt. |

## Streams (automatic)

| Define | Meaning |
|---|---|
| `STREAMS_PREVS` | Number of previous frames kept per stream. |
| `STREAMS_FRAME` | Stream frame indexing. |

## Plot (automatic)

| Define | Meaning |
|---|---|
| `PLOT_VALUE` | Active on-screen plot mode (see the `plot` command). |
