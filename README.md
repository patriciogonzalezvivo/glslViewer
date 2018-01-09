# glslViewer [![Build Status](https://travis-ci.org/patriciogonzalezvivo/glslViewer.svg?branch=master)](https://travis-ci.org/patriciogonzalezvivo/glslViewer)

![](http://patriciogonzalezvivo.com/images/glslViewer.gif)

[![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=4BQMKQJDQ9XH6)

Live-coding console tool that renders GLSL Shaders. Every file you use (frag/vert shader, images and geometries) are watched for modification, so they can be updated on the fly.

## Install

### Installing on Ubuntu

Install the GLFW 3 library and other dependencies:

```bash
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install libglfw3-dev git-core
```

Download the glslViewer code, compile and install:

```bash
git clone http://github.com/patriciogonzalezvivo/glslViewer
cd glslViewer
make
sudo make install
```

This was tested with Ubuntu 16.04.

These instructions may not work for all users.
For example, it seems that libglfw3-dev conflicts with the older libglfw-dev.
The previous Ubuntu install instructions direct you to
[download and compile glfw3 manually](http://www.glfw.org/docs/latest/compile.html#compile_deps_x11):

```bash
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install git-core cmake xorg-dev libglu1-mesa-dev
cd ~
git clone https://github.com/glfw/glfw.git
cd glfw
cmake .
make
sudo make install
```

### Installing on Raspberry Pi

Get [Raspbian](https://www.raspberrypi.org/downloads/raspbian/), a Debian-based Linux distribution made for Raspberry Pi and then do:

```bash
sudo apt-get install glslviewer
```

Or, if you want to compile the code yourself:

```bash
cd ~
git clone http://github.com/patriciogonzalezvivo/glslViewer
cd glslViewer
make
sudo make install
```

### Installing on Arch Linux

```bash
sudo pacman -S glu glfw-x11
git clone http://github.com/patriciogonzalezvivo/glslViewer
cd glslViewer
make
sudo make install
```

Or simply install the AUR package [glslviewer-git](https://aur.archlinux.org/packages/glslviewer-git/) with an [AUR helper](https://wiki.archlinux.org/index.php/AUR_helpers).

### Installing on macOS

Use [Homebrew](https://brew.sh) to install glslViewer and its dependencies:

```bash
brew update
brew upgrade
brew install glslviewer
```

If you prefere to compile from source directly from this repository you need to [install GLFW](http://www.glfw.org), `pkg-config` first and then download the code, compile and install.

```bash
brew update
brew upgrade
brew tap homebrew/versions
brew install glfw3 pkg-config
cd ~
git clone http://github.com/patriciogonzalezvivo/glslViewer
cd glslViewer
make
make install
```

If `glfw3` was installed before, after running the code above, remove `glfw3` and try:

```bash
brew install glfw3 pkg-config
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
make
make install
```

## Use

In the most simple scenario you just want to load a fragment shader. For that you need to:

* Run the app passing the shader as an argument

```bash
cd examples
glslViewer test.frag
```

* Then edit the shader with your favorite text editor.

```bash
vim test.frag
```

**Note**: In RaspberryPi you can avoid taking over the screen by using the `-l` flags so you can see the console. Also you can edit the shader file through ssh/sftp.

**Note**: On Linux and macOS you may used to edit your shaders with Sublime Text 2, if thats your case you should try this [Sublime Text 2 plugin that lunch glslViewer every time you open a shader](https://packagecontrol.io/packages/glslViewer).

### Loading Vertex shaders and geometries

![](http://patriciogonzalezvivo.com/images/glslViewer-3D.gif)

You can also load both fragments and vertex shaders. Of course modifing a vertex shader makes no sense unless you load an interesting geometry. That's why `glslViewer` is can load `.ply` files. Try doing:

```bash
glslViewer bunny.frag bunny.vert bunny.ply
```

### Pre-Define `uniforms` and `varyings`

* `uniform float u_time;`: shader playback time (in seconds)

* `uniform float u_delta;`: delta time between frames (in seconds)

* `uniform vec4 u_date;`: year, month, day and seconds

* `uniform vec2 u_resolution;`: viewport resolution (in pixels)

* `uniform vec2 u_mouse;`: mouse pixel coords

* `varying vec2 v_texcoord`: UV of the billboard ( normalized )

* `uniform vec3 u_eye`: Position of the 3d camera when rendering 3d objects

* `u_view2d`: 2D position of viewport that can be changed by dragging

The following variables are used for fragment shaders that mimic a 3d model. See examples/menger.frag.

* `u_eye3d`: Position of the camera

* `u_centre3d`: Position of the center of the object

* `u_up3d`: Up-vector of the camera

### ShaderToy.com Image Shaders

ShaderToy.com image shaders are automatically detected and supported.
These conventions are also supported by other tools, such as Synthclipse.

To be recognized as a ShaderToy image shader, a fragment shader must define
```c++
void mainImage(out vec4 fragColor, in vec2 fragCoord)
```
It must not define `main()`, because this is automatically defined for you.

The following ShaderToy uniforms are automatically defined,
you don't declare them:
* `uniform vec3 iResolution;` <br>
  `iResolution.xy` is the viewport size in pixels, like `u_resolution`.
  `iResolution.z` is hard coded to 1.0, just like shadertoy.com and synthclipse,
  although it was originally supposed to be the pixel aspect ratio.
* `uniform float iGlobalTime;` <br>
  Shader playback time (in seconds), like `u_time`.
* `uniform float iTimeDelta;` <br>
  Render time for last frame (in seconds), like `u_delta`.
* `uniform vec4 iDate;` <br>
  [year, month (0-11), day of month (1-31), time of day (in seconds)],
  like `u_date`.
* `uniform vec4 iMouse;` <br>
  `iMouse` is initialized to 0, and only changes while the left mouse button
  (LMB) is being held down.
  * Mouse coordinates are integers in the range `[0,0]..iResolution.xy`.
  * `iMouse.xy` is the current mouse coordinates in pixels, while the LMB
    is being held down. When the LMB is released, `iMouse.xy` is set to the
    current coordinates, then stops changing.
  * `iMouse.zw` is set to the current mouse coordinates at the instant when
    the LMB is pressed, remains constant as long as the LMB is held down,
    and is set to `-iMouse.zw` when the LMB is released.
  * If the LMB is up, then `iMouse.xy` is the mouse location at the most recent
    mouseup event, and `iMouse.zw` is the negative of the mouse location at the
    most recent mousedown event.
    For example, after a mouse click, `iMouse` might be `[216,320,-216,-320]`.

Demo: `examples/numbers.frag`

### Textures

You can load PNGs and JPEGs images to a shader. They will be automatically loaded and asigned to an uniform name acording to the order they are pass as arguments: ex. `u_tex0`, `u_tex1`, etc. Also the resolution will be assigned to `vec2` uniform acording the texture uniforma name: ex. `u_tex0Resolution`, `u_tex1Resolution`, etc.

```bash
glslViewer test.frag test.png
```

In case you want to assign customs names to your textures uniforms you must specify the name with a flag before the texture file. For example to pass the following uniforms `uniform sampled2D imageExample;` and  `uniform vec2 imageExampleResolution;` is defined in this way:

```bash
glslViewer shader.frag -imageExample image.png
```

### Others arguments

Beside for texture uniforms other arguments can be add to `glslViewer`:

* `-x [pixels]` set the X position of the billboard on the screen

* `-y [pixels]` set the Y position of the billboard on the screen

* `-w [pixels]` or `--width [pixels]`  set the width of the billboard

* `-h [pixels]` or `--height [pixels]` set the height of the billboard

* `-s [seconds]` exit app after a specific amount of seconds

* `-o [image.png]` save the viewport to a image file before

* `-l` to draw a 500x500 billboard on the top right corner of the screen that let you see the code and the shader at the same time. (RaspberryPi only)

* `-c`or `--cursor` show cursor.

* `--headless` headless rendering. Very usefull for making images or benchmarking.

* `-I[include_folder]` add a include folder to default for `#include` files

* `-D[define]` add system `#define`s directly from the console argument

* `-[testure_uniform_name] [texture.png]`: add textures asociated with different `uniform sampler2D`names

* `-vFlip` all textures after will be fliped vertically

*  `-v` verbose outputs

* `--help` display the available command line options

### Inject other files

You can include other GLSL code using a traditional `#include "file.glsl"` macro. Note: included files are not under watch so changes will not take effect until the main file is save.

### Console IN commands

Once glslViewer is running the CIN is listening for some commands, so you can pass data trough regular *nix pipes.

* ```int```, ```floats```, ```vec2```, ```vec3``` and ```vec4``` uniforms can be pass as comma separated values, where the first column is for the name of the uniform and the rest for the numbers of values the uniform have. **Note** that there is a distintion between ```int```and ```float```so remember to put ```.``` (floating points) to your values.

* ```data```: return content of ```u_date```, return the current year, month, day and seconds

* ```time```: return content of ```u_time```, the elapsed time since the app start

* ```delta```: return content of ```u_delta```,return the last delta time between frames

* ```fps```: return content of ```u_fps```,return the number of frames per second

* ```frag```: return the source of the fragment shader

* ```vert```: return the source of the vertex shader

* ```window_width```, ```window_height```, ```screen_size``` and ```viewport``` : return the size of the windows, screen and viewport (content of ```u_resolution```)

* ```pixel_density```: return the pixel density

* `mouse_x`, `mouse_y` and `mouse`: return the position of the mouse (content of `u_mouse`)

* `view3d`:

* `screenshot [filename]`: save a screenshot of what's being render. If there is no filename as argument will default to what was define after the `-o` argument when glslViewer was lanched.

* `q`, `quit` or `exit`: close glslViewer

## glslLoader

```glslLoader``` is a python script that is install together with ```glslViewer``` binnary which let you download any shader made with [The book of shaders editor (editor.thebookofshaders.com) ](http://editor.thebookofshaders.com/). Just pass as argument the ***log number***

![](http://patriciogonzalezvivo.com/images/glslGallery/00.gif)

```bash
glslLoader 170208142327
```

It will also download a shader shared through the [ShaderToy's](https://shadertoy.com/) by passing the ID `glslLoader`. Ex:

```bash
glslLoader llVXRd
```

## Examples

* Open a Fragment shader:

```bash
glslViewer examples/test.frag
```

* Open a Fragment shader with an image:

```bash
glslViewer examples/test.frag examples/test.png
```

* Open a Fragment and Vertext shader with a geometry:

```bash
glslViewer examples/bunny.frag examples/bunny.vert examples/bunny.ply
```

* Open a Fragment that use the backbuffer to do a reaction diffusion example:

```bash
glslViewer examples/grayscott.frag
```

* Open a Fragment that use OS defines to know what platform is runnion on:

```bash
glslViewer examples/platform.frag
```

* Change a uniform value on the fly by passing CSV on the console IN:

```bash
glslViewer examples/temp.frag
u_temp,30
u_temp,40
u_temp,50
u_temp,60
u_temp,70
```

* Create a bash script to change uniform parameters on the fly:

```bash
examples/.temp.sh | glslViewer examples/temp.frag
```

* Run a headless instance of glslViewer that exits after 1 second outputing an PNG image:

```bash
glslViewer examples/cross.frag --headless -s 1 -o cross.png
```

* Make an SVG from a shader usin [potrace](http://potrace.sourceforge.net/) and [ImageMagic](https://www.imagemagick.org/script/index.php):

```bash
glslViewer examples/cross.frag --headless -s 1 -o cross.png
convert cross.png cross.pnm
potrace cross.pnm -s -o cross.svg
```

* Open a ShaderToy's image shader:

```bash
glslViewer examples/numbers.frag
```

* Download a shader shared through the [ShaderToy's](https://shadertoy.com/) by passing the ID `glslLoader`. Ex:

```bash
glslLoader llVXRd
```

* Download a shader shared through the [Book of Shader's](https://thebookofshaders.com/) [editor](http://editor.thebookofshaders.com/) by passing the LOG number to `glslLoader`. Ex:

```bash
glslLoader 170208142327
```

## Author

[Patricio Gonzalez Vivo](http://https://twitter.com/patriciogv): [github](https://github.com/patriciogonzalezvivo) | [twitter](http://https://twitter.com/patriciogv) | [website](http://patricio.io)

## Acknowledgements

Thanks to:

* [Karim’s Naaki](http://karim.naaji.fr/) lot of concept and code was inspired by his: [fragTool](https://github.com/karimnaaji/fragtool).

* [Doug Moen](https://github.com/doug-moen) he help to add the compatibility to ShaderToy shaders and some RayMarching features were added for his integration with his project: [curv](https://github.com/doug-moen/curv).

* [Yvan Sraka](https://github.com/yvan-sraka) for puting the code in shape and setting it for TravisCI.
