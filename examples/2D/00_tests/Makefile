test:
	glslViewer test.frag

test_in_png:
	glslViewer test.frag in.png

test_in_tga:
	glslViewer test.frag in.tga

test_in_gif:
	glslViewer test.frag in.gif

test_in_psd:
	glslViewer test.frag in.psd

test_in_mp4:
	glslViewer test.frag -w 960 -h 540 in.mp4

test_in_depth:
	glslViewer test_depth.frag iphone_depth.jpeg -w 576 -h 768 -e textures,on

test_out_png:
	glslViewer test.frag in.png -E screenshot,out.png

test_out_tga:
	glslViewer test.frag in.png -E screenshot,out.tga

test_out_jpg:
	glslViewer test.frag in.png -E screenshot,out.jpg

test_out_hdr:
	glslViewer test.frag in.png -E screenshot,out.hdr

test_audio:
	glslViewer test_audio.frag --audio

test_video:
	glslViewer test_video.frag in.mp4 -w 960 -h 540

test_video_prevs:
	glslViewer test_video.frag in.mp4 -w 960 -h 540 -e streams,prevs,10 -e streams,speed,0.5
