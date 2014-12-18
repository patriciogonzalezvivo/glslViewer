## piFrag

A live GLSL Fragment render for live coding with the RaspberryPi inspired on [Karim’s fragTool](https://github.com/karimnaaji/fragtool)

### Install

Install FreeImage libraries, download the code, compile and install

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
piFrag ~/piFrag/test.frag
```

If you set alpha value to 0.3 (```glFragColor.a = 0.3;```) you can se through the console and from another terminal edit your shader with you favorite editor:

```
vim ~/piFrag/test.frag
```

In the test fragment shader we have hook it up to the X mouse position so you can take a look while you work on it.

Or you can login remotely using ssh to your raspberryPi and live-code with your favorite editor:

```
ssh pi@raspberrypi.local
vim ~/piFrag/test.frag
```

### Loading texture uniforms

You can load any image formats suported by FreeImage libraries. Set the uniform name as an argument followed with the file name and the app will load it for you. For example, the ```uniform sampled2D tex0;``` is defined in this way:

```
piFrag test.frag -tex0 test.png
```

### Inject other files

You can include other GLSL code using a traditional ```#include “file.glsl”``` macro. Check the example.

## Authors

* [Patricio Gonzalez Vivo](http://patriciogonzalezvivo.com/)

* [Karim Naaji](http://karim.naaji.fr/)