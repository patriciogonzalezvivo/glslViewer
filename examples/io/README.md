# ORCA

Example made by Johan Ismael of how to connect [ORCA](https://100r.co/pages/orca.html), Ableton (MIDI) and glslViewer (OSC).Open `orca.orca` file on [ORCA](https://100r.co/pages/orca.html) and then:

```
make orca
```

# Chataigne 

Example on how to send animated values in [Chataigne](http://benjamin.kuperberg.fr/chataigne/en) to glslViewer through OSC. Open `chatainge.noisette` file on  [Chataigne](http://benjamin.kuperberg.fr/chataigne/en) and then:

```
make chataigne
```

# TouchOSC

Example on how to send values from [Touch OSC](https://hexler.net/products/touchosc) to glslViewer through OSC. Open Touch OSC and then:

```
make touchOSC
```

# Ossia Score

Example on how to send animated values from [Ossia Score](https://ossia.io/) to glslViewer through OSC. Open `ossia.score` file with [Ossia](https://ossia.io/) and then:

```
make ossia
```

# Scripts 

## Temp (Raspberry Pi only)

Bash script witch feed a single uniform `u_temp` to glslViewer with the CPU temperature over console IN in csv format

```bash
./temp.sh | glslViewer temp.frag -l
```


## TimeWarp

Python script that for every PNG in `frames/` folder creates an OpticalFlow image, then it use it to displace each single pixel.

Dependencies:

- OpenCV
- Numpy

Then run:

```bash
python3 timewarp.py
```

Then you can compile the final frames with ffmpeg:

```bash
ffmpeg -i frames_final/%05d.png -pix_fmt yuv420p final.mp4
```

[![Watch the video](https://img.youtube.com/vi/_REN7EdnES0/maxresdefault.jpg)](https://www.youtube.com/watch?v=_REN7EdnES0)