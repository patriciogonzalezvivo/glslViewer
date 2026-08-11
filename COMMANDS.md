# Commands

Commands are comma-separated (`command,arg1,arg2`). They can be sent:

- at startup with `-e <command>` / `-E <command>` (see [ARGUMENTS.md](ARGUMENTS.md)),
- interactively in the console (unless `--noncurses`),
- over OSC when a port is open (`-p <port>`).

Most getters/setters follow the pattern **"no args → print current value, with args → set it."** In the tables, `[ ]` marks optional arguments and `|` marks a choice.

## Info & meta

| Command | Description |
|---|---|
| `help[,<command>]` | Print help for one or all commands. |
| `version` | Return the glslViewer version. |
| `about` | About glslViewer. |
| `glsl_version` | Return the GLSL version. |
| `defines` | List active `#define` flags (see [DEFINES.md](DEFINES.md)). |
| `uniforms[,all\|active\|defined\|textures\|buffers\|cubemaps\|lights\|cameras\|on\|off]` | List uniforms (see [UNIFORMS.md](UNIFORMS.md)); `on/off` toggles the on-screen panel. |
| `files` | List loaded/watched files. |
| `dependencies[,vert\|frag]` | List `#include` dependencies of the vertex/fragment shader (or both). |
| `pixel_density` | Return the pixel density. |

## Time, playback & capture

| Command | Description |
|---|---|
| `time` | Return `u_time` (elapsed seconds). |
| `delta` | Return `u_delta` (seconds between frames). |
| `date` | Return `u_date` as `YYYY, M, D, secs`. |
| `reset` | Reset the timestamp back to zero. |
| `fps` | Return or set the frames-per-second cap. |
| `fullFps[,on\|off]` | Render at full FPS (vs. on-change). |
| `vsync[,on\|off]` | Enable/disable VSync (on by default). |
| `wait,<seconds>` | Wait N seconds before running the next command. |
| `update` | Force all uniforms to be updated. |
| `screenshot[,<filename>]` | Save a screenshot. |
| `sequence,<from_sec>,<to_sec>[,<fps>]` | Save a PNG sequence between two seconds (default 24 fps). |
| `secs,<A>,<B>[,<fps>]` | Save images from second A to B (default 24 fps). |
| `frames,<A>,<B>[,<fps>]` | Save images from frame A to B (default 24 fps). |
| `record,<file>,<A>,<B>[,<fps>]` | Record a video from second A to B (default 24 fps). |
| `max_mem_in_queue,<bytes>` | Max memory used by the image-save queue. |

## Window & viewport

| Command | Description |
|---|---|
| `window_width` | Return the window width. |
| `window_height` | Return the window height. |
| `screen_size` | Return the screen size. |
| `viewport` | Return the viewport size. |
| `mouse` | Return the mouse position. |
| `cursor[,on\|off]` | Show/hide the cursor. |

## Shaders

| Command | Description |
|---|---|
| `frag[,<filename>]` | Return, or save to file, the fragment shader source. |
| `vert[,<filename>]` | Return, or save to file, the vertex shader source. |
| `reload[,<filename>]` | Reload one or all files. |
| `define,<KEYWORD>[,<VALUE>]` | Add a `#define` to the shader (see [DEFINES.md](DEFINES.md)). |
| `undefine,<KEYWORD>` | Remove a `#define`. |
| `error_screen,on\|off` | Enable/disable the magenta error screen on shader errors. |
| `debug[,on\|off]` | Show/hide debug elements, or return their status. |
| `track[,on\|off\|average\|samples]` | Start/stop render-time tracking. |
| `plot[,off\|luma\|red\|green\|blue\|rgb\|fps\|ms]` | Show/hide an on-screen histogram or FPS/ms plot. |

## Scene, models & materials

| Command | Description |
|---|---|
| `models[,clear]` | Print all model names, or clear all loaded models. |
| `model,<name>[,<x>,<y>,<z>]` | Print a model's info/defines, or set its position. |
| `materials` / `material,<name>` | List materials, or print a material's defines. |
| `origin[,<x>,<y>,<z>]` | Get or set the scene origin. |
| `generate_sdf[,<padding>[,<resolution>]]` | Build a 3D SDF texture of loaded models (default padding 0.01, resolution 6). |
| `bboxes[,on\|off]` | Show/hide model bounding boxes. |
| `axis[,on\|off]` | Show/hide the axis gizmo. |
| `grid[,on\|off]` | Show/hide the grid. |

### Procedural geometry (replace the loaded geometry)

| Command | Description |
|---|---|
| `plane[,<subd>]` | Load a plane. |
| `pcl_plane[,<subd>]` | Load a plane as a point cloud. |
| `sphere[,<resolution>]` | Load a sphere. |
| `pcl_sphere[,<resolution>]` | Load a sphere as a point cloud. |
| `icosphere[,<resolution>]` | Load an icosphere. |
| `cylinder[,<radius>,<height>,...]` | Load a cylinder. |

## Floor & rendering state

| Command | Description |
|---|---|
| `floor[,on\|off\|toggle\|res\|<subD_level>]` | Show/hide the floor or set its subdivision level. |
| `floor_color[,<r>,<g>,<b>]` | Get or set the skybox ground color. |
| `blend[,alpha\|add\|multiply\|screen\|substract]` | Get or set the blend mode. |
| `depth_test[,on\|off]` | Turn depth testing on/off. |
| `culling[,none\|front\|back\|both]` | Get or set the face-culling mode. |
| `dynamic_shadows[,on\|off]` | Get or set dynamic shadows. |

## Textures & buffers

| Command | Description |
|---|---|
| `textures[,on\|off\|list]` | Show/hide the input-textures debug column, or list textures. |
| `buffers[,show\|hide\|list]` | Show/hide the render-pass debug column, or list buffers. |

## Environment & sky

| Command | Description |
|---|---|
| `cubemaps` | List all cubemaps. |
| `cubemap[,on\|off\|toggle\|sh]` | Show/hide the cubemap, or return its spherical harmonics. |
| `sky[,on\|off]` | Show/hide the procedural skybox. |
| `sun_elevation[,<degrees>]` | Get or set the sun elevation (requires `sky,on`). |
| `sun_azimuth[,<degrees>]` | Get or set the sun azimuth (requires `sky,on`). |
| `sky_turbidity[,<value>]` | Get or set the sky turbidity. |

## Lights

| Command | Description |
|---|---|
| `lights` | Print all light-related uniforms. |
| `light_position[[,<index>],<x>,<y>,<z>]` | Get or set a light's position. |
| `light_color[,<r>,<g>,<b>]` | Get or set the light color. |
| `light_falloff[,<value>]` | Get or set the light falloff distance. |
| `light_intensity[,<value>]` | Get or set the light intensity. |

## Camera

| Command | Description |
|---|---|
| `camera[,<name>\|default\|list]` | Select the active camera, or list named cameras. |
| `camera_distance[,<dist>]` | Get or set the camera distance to the target. |
| `camera_type[,ortho\|perspective]` | Get or set the projection type. |
| `camera_fov[,<fov>]` | Get or set the field of view. |
| `camera_position[,<x>,<y>,<z>]` | Get or set the camera position. |
| `camera_move,<x>,<y>,<z>` | Move the camera position. |
| `camera_look_at[,<x>,<y>,<z>]` | Point the camera toward a position. |
| `camera_exposure[,<aperture>,<shutter>,<sensitivity>]` | Get or set the physical camera exposure. |

### Camera animations

These play continuously **until the user does a mouse gesture** (click/drag/scroll),
which cancels the animation and hands control back to manual interaction. Use
`camera,stop` to cancel programmatically. `camera,speed` also scales the animated
transition when selecting a COLMAP named camera.

| Command | Description |
|---|---|
| `camera,orbit` | Spin continuously around the target (elevation & distance held, departs from the current azimuth). |
| `camera,arc[,<deg=180>]` | Ping-pong the azimuth within ±deg/2 around the start (−90° then +90° for 180°). Anchored to the target. |
| `camera,dolly[,<min>,<max>]` | Oscillate the distance to the target between min and max. |
| `camera,truck,<dist>` | Slide along the camera's local X by ±dist/2 (look direction unchanged). |
| `camera,pedestal,<dist>` | Slide along the camera's local Y by ±dist/2. |
| `camera,pan,<angle>` | Pivot the view about the local Y axis by ±angle/2. |
| `camera,tilt,<angle>` | Pivot the view about the local X axis by ±angle/2. |
| `camera,roll,<angle>` | Pivot the view about the local Z axis by ±angle/2. |
| `camera,speed,<number>` | Global multiplier for all camera animations **and** the COLMAP camera transition. |
| `camera,stop` (or `camera,off`) | Cancel any running camera animation. |

## Streams (video / camera / image sequences / audio)

| Command | Description |
|---|---|
| `streams[,stop\|play\|restart\|frame\|speed\|prevs[,<value>]]` | List streams, or get/set global stream speed/previous-frames. |
| `stream,<name>,stop\|play\|speed\|time[,<value>]` | Play/stop, or change speed/time of a specific stream. |

## Control

| Command | Description |
|---|---|
| `exit`, `quit`, `q` | Close glslViewer. |
