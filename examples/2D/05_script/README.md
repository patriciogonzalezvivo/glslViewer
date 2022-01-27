## 2D Scripts 

### Temp (Raspberry Pi only)

Bash script witch feed a single uniform `u_temp` to glslViewer with the CPU temperature over console IN in csv format

```bash
./temp.sh | glslViewer temp.frag -l
```


### TimeWarp

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