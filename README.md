## glslViewer

Live-coding tool that renders GLSL Fragment shaders and update them every time they change and runs directly from the console. It handles for you the injection of sub-programs, textures and other uniforms such as time, resolution and mouse position.

### Installing in Ubuntu

[Install GLFW](http://www.glfw.org/docs/latest/compile.html#compile_deps_x11) then install FreeImage libraries, download the code, compile and install.

```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install git-core cmake xorg-dev libglu1-mesa-dev
cd ~ 
git clone git@github.com:glfw/glfw.git
cd glfw
cmake .
make
make install
sudo apt-get install libfreeimage-dev
git clone http://github.com/patriciogonzalezvivo/glslViewer
cd glslViewer
make
make install
```

### Installing in RaspberryPi

Install FreeImage libraries, download the code, compile and install.

```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install libfreeimage-dev
cd ~ 
git clone http://github.com/patriciogonzalezvivo/glslViewer
cd glslViewer
make
make install
```

### Installing in Mac OSX

```
brew update
brew upgrade
brew install freeimage 
brew tap homebrew/versions
brew install glfw3 pkg-config
cd ~ 
git clone http://github.com/patriciogonzalezvivo/glslViewer
cd glslViewer
make
make install
```

### Use

1. Run the app:

```
glslViewer test.frag
```

2. Edit the shader with your favorite text editor. 

```
vim test.frag
```

**Note**: In RaspberryPi you can avoid taking over the screen by using the ```-l``` or ```--live-coding``` flags so you can see the console. Also you can edit the shader file through ssh/sftp.

### Pre-Define Uniforms and Varyings

Shaders are cross compatible with the webGL shaders from [ShaderToy](http://www.shadertoy.com) for that the following uniforms are pre-define and can be add to the shader with the ```-u``` argument

* ```uniform float u_time;```: shader playback time (in seconds)

* ```uniform vec2 u_resolution;```: viewport resolution (in pixels)

* ```uniform vec2 u_mouse;```: mouse pixel coords (xy: pos, zw: buttons)

* ```varying vec2 v_texcoord```: UV of the billboard ( normalized )

### Dynamic uniforms: textures

You can load any image suported by FreeImage libraries, they will be automatically loaded and asigned to an uniform name acording to the order they are pass as arguments: ex. ```u_tex0```, ```u_tex1```, etc. Also the resolution will be assigned to ```vec2``` uniform acording the texture uniforma name: ex. ```u_tex0Resolution```, ```u_tex1Resolution```, etc. 

```
glslViewer test.frag test.png
```

In case you want to assign customs names to your textures uniforms you must specify the name with a flag before the texture file. For example to pass the following uniforms ```uniform sampled2D imageExample;``` and  ```uniform vec2 imageExampleResolution;``` is defined in this way:

```
glslViewer shader.frag -imageExample image.png
```

### Others arguments

Beside for texture uniforms other arguments can be add to ```glslViewer```:

* ```-x [pixels]``` set the X position of the billboard on the screen

* ```-y [pixels]``` set the Y position of the billboard on the screen

* ```-w [pixels]``` or ```--width [pixels]```  set the width of the billboard

* ```-h [pixels]``` or ```--height [pixels]``` set the height of the billboard

* ```-s [seconds]``` exit app after a specific amount of seconds

* ```-o [filename]``` save the viewport to a image file before exit

* ```-d``` or ```--dither``` dither the image before exit

* ```--squared``` to set a squared billboard

* ```-l``` or ```--live-coding``` to draw a 500x500 billboard on the top right corner of the screen that let you see the code and the shader at the same time

* ```-u``` inject the following header with the default uniforms to the loaded shader file.

```glsl
#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;
varying vec2 v_texcoord;
```

### Inject other files

You can include other GLSL code using a traditional ```#include “file.glsl”``` macro. Note: included files are not under watch so changes will not take effect until the main file is save.
