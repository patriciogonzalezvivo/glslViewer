## piFrag

PiFrag is a RaspberryPi console-based live-coding tool that renders GLSL Fragment shaders and updates them each time the file change. Let's you inject other shaders, textures and some basic mouse events. Using the opacity of the out put you can combine multiple instances of this program and be creative using layers.

### Install

Install FreeImage libraries, download the code, compile and install.

```
sudo apt-get update
sudo apt-get update
sudo apt-get install libfreeimage3-dev
cd ~ 
git clone http://github.com/patriciogonzalezvivo/piFrag
cd piFrag
make
sudo make install
```

### Use

Run the app:

```
piFrag ~/piFrag/examples/test.frag
```

If you set alpha value to 0.3 (```glFragColor.a = 0.3;```) you can se through the console and from another terminal edit your shader with you favorite editor:

```
vim ~/piFrag/examples/test.frag
```

In the test fragment shader we have hook it up to the X mouse position so you can take a look while you work on it.

Or you can login remotely using ssh to your raspberryPi and live-code with your favorite editor:

```
ssh pi@raspberrypi.local
vim ~/piFrag/examples/test.frag
```

### Pre-Define Uniforms

Shaders are cross compatible with the webGL shaders from [ShaderToy](http://www.shadertoy.com) for that the following uniforms are pre-define

* ```iGlobalTime``` (```float```) or ```u_time``` (```float```): shader playback time (in seconds)

* ```iResolution``` (```vec3```) or ```u_resolution``` (```vec2```): viewport resolution (in pixels)

* ```iMouse``` (```vec4```) or ```u_mouse``` (```vec2```): mouse pixel coords (xy: pos, zw: buttons)

* ```v_texcoord``` (```vec2```): UV of the billboard ( normalized )

### Loading texture uniforms

You can load any image formats suported by FreeImage libraries. Set the uniform name as an argument followed with the file name and the app will load it for you. For example, the ```uniform sampled2D tex0;``` is defined in this way:

```
cd ~/piFrag/examples/
piFrag test.frag --tex0 test.png
```

### Others arguments

Beside for texture uniforms other arguments can be add to ```piFrag``` to change it setup.

* ```-x [pixels]``` set the X position of the billboard on the screen

* ```-y [pixels]``` set yhe Y position of the billboard on the screen

* ```-w [pixels]``` or ```--width [pixels]```  set the width of the billboard

* ```-h [pixels]``` or ```--height [pixels]``` set the height of the billboard

* ```-s``` Squared Billboard

### Inject other files

You can include other GLSL code using a traditional ```#include “file.glsl”``` macro. Note: included files are not under watch so changes will not take effect until the main file is save.

## Authors

* [Patricio Gonzalez Vivo](http://patriciogonzalezvivo.com/)

* [Inspired on Karim’s fragTool](https://github.com/karimnaaji/fragtool). 


