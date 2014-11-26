## piFrag

A live GLSL Fragment render for live coding with the RaspberryPi inspired on [Karim’s fragTool](https://github.com/karimnaaji/fragtool)

### Install

```
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

Login remotely using ssh to your raspberryPi and live-code with your favorite editor:

```
ssh pi@raspberrypi.local
vim ~/piFrag/test.frag
```

## Authors

* [Patricio Gonzalez Vivo](http://patriciogonzalezvivo.com/)

* [Karim Naaji](http://karim.naaji.fr/)