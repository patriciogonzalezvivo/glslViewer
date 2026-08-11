# Command-line Arguments

```
glslViewer <frag_shader>.frag [<vert_shader>.vert <geometry>.obj|ply|stl|glb|gltf] [options...]
```

glslViewer parses positional files by extension and options by flag. Multiple
geometry files can be combined (meshes, point clouds and splats), and each is
namespaced by its filename.

## Positional inputs (detected by extension)

| Input | Extensions | Notes |
|---|---|---|
| Fragment shader | `.frag` `.fs` | If missing, a default is created / used. |
| Vertex shader | `.vert` `.vs` | Optional; a default scene vertex shader is used with geometry. |
| Geometry | `.ply` `.obj` `.stl` `.glb` `.gltf` `.splat` | Meshes, point clouds and gaussian splats. Multiple allowed; they coexist. |
| Textures | `.png .jpg .jpeg .tga .bmp .psd .gif .hdr .exr` and video `.mov .mp4 .rtsp .rtmp` | Assigned to `u_tex0`, `u_tex1`, … in order. Wildcards create an image-sequence stream. |
| Camera set | `.csv` | COLMAP camera poses (`camera.csv`); switches the scene into the COLMAP frame. |
| Command list | `.lst` | A list of files/commands. |

## Window & display

| Flag | Argument | Description |
|---|---|---|
| `-x` | `<pixels>` | X position of the window on screen. |
| `-y` | `<pixels>` | Y position of the window on screen. |
| `-s`, `--size` | `<w> <h>` | Set width and height of the window. |
| `-w`, `--width` | `<pixels>` | Set the window width. |
| `-h`, `--height` | `<pixels>` | Set the window height. |
| `-f`, `--fullscreen` | | Open in fullscreen. |
| `--undecorated` | | Borderless window. |
| `-l`, `--life-coding` | | Live-code mode: the billboard is always visible. |
| `-ss`, `--screensaver` | | Screensaver mode: any key press exits. |
| `--headless` | | Headless (offscreen) rendering. |
| `--nocursor` | | Hide the cursor. |
| `--nofloor` | | Hide the default floor. |
| `-d`, `--display` | `<display>` | Open a specific display port (GBM driver only). Ex: `/dev/dri/card1`. |
| `-m`, `--mouse` | `<mouse>` | Open a specific mouse device (non-GLFW drivers). Ex: `/dev/input/mice`. |
| `-msaa` | | Enable 4× MSAA. |
| `-major` / `-minor` | `<n>` | Request a specific OpenGL major/minor version. |

## Textures & environment

| Flag | Argument | Description |
|---|---|---|
| `-<name>` | `<texture>` | Load a texture under a **custom** uniform name `u_<name>` (e.g. `-diffuse img.png` → `u_diffuse`). |
| `--video` | `<device#>` | Open a video capture device as a texture. |
| `--audio`, `-a` | `[<device_id>]` | Open an audio capture device as a `sampler2D` texture. |
| `-C` | `<envmap>` | Load an environment map as a cubemap **and display it**. |
| `-c` | `<envmap>` | Load an environment map as a cubemap (hidden). |
| `-sh` | `<envmap>` | Load an environment map as a spherical-harmonics array. |
| `-vFlip` | | Vertically flip all textures loaded after this flag. |

## Rendering options

| Flag | Argument | Description |
|---|---|---|
| `--fxaa` | | Use FXAA as a post-process filter. |
| `-r`, `--fps` | `<fps>` | Cap the maximum FPS. |
| `-fullFps` | | Render at full FPS (don't throttle to on-change). |
| `--verbose` | | Verbose output. |

## Holographic (Looking Glass / HoloPlay)

| Flag | Argument | Description |
|---|---|---|
| `--quilt` | `<0-15>` | Render a quilt at the given preset resolution. |
| `--quilt_tile` | `<N>` | Render a single tile of the quilt. |
| `--lenticular` | `<visual.json>` | Looking Glass lenticular calibration file. |

## Shader system

| Flag | Argument | Description |
|---|---|---|
| `-D<define>` | | Add a `#define` from the command line (e.g. `-DMY_FLAG` or `-DFOO=1`). |
| `-I<folder>` | | Add an include folder for `#include` resolution. |

## Runtime / control

| Flag | Argument | Description |
|---|---|---|
| `-e` | `<command>` | Execute a command on start (stackable — multiple `-e` allowed). |
| `-E` | `<command>` | Same as `-e` but **exit** after the commands run (useful for headless/CI). |
| `-p`, `--port` | `<OSC_port>` | Open an OSC listening port for commands. |
| `--noncurses` | | Disable the ncurses console UI (plain stdin/stdout). |

## Info

| Flag | Description |
|---|---|
| `-v`, `--version` | Print the glslViewer version. |
| `--help` | Print help for one or all commands. |

## Notes

- `-e`/`-E` take any command from [COMMANDS.md](COMMANDS.md); commands can also be
  sent at runtime via the console (or OSC when `-p` is set).
- Custom-named textures (`-<name>`) become the uniform `u_<name>` plus a
  `u_<name>Resolution` companion; see [UNIFORMS.md](UNIFORMS.md).
- `-D` defines and the geometry/material of a loaded model drive `#ifdef`
  branches in the shader; see [DEFINES.md](DEFINES.md).
