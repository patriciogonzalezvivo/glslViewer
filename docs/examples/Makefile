test:
	glslViewer test.frag

test_image_png:
	glslViewer test.frag image.png

test_image_tga:
	glslViewer test.frag image.tga

test_image_gif:
	glslViewer test.frag image.gif

test_image_psd:
	glslViewer test.frag image.psd

test_image_depth:
	glslViewer test_depth.frag image_depth.jpeg -w 576 -h 768 -e textures,on

test_screenshot_png:
	glslViewer test.frag image.png -E screenshot,out.png

test_screenshot_tga:
	glslViewer test.frag image.png -E screenshot,out.tga

test_screenshot_jpg:
	glslViewer test.frag image.png -E screenshot,out.jpg

test_screenshot_hdr:
	glslViewer test.frag image.png -E screenshot,out.hdr

test_audio:
	glslViewer test_audio.frag --audio

test_audio_fft:
	glslViewer test_audio_fft.frag -l --audio

test_audio_wave:
	glslViewer test_audio_wave.frag -l --audio

test_audio_spectrogram:
	glslViewer test_audio_spectogram.frag -l --audio

test_audio_amp:
	glslViewer test_audio_amp.frag -l --audio

test_video:
	glslViewer test_video.frag in.mp4 -w 960 -h 540

test_video_prevs:
	glslViewer test_video.frag in.mp4 -w 960 -h 540 -e streams,prevs,10 -e streams,speed,0.5

test_buffers:
	glslViewer test_buffers.frag -l

test_buffers_scale:
	glslViewer test_buffers_scale.frag image.png -l

test_doubleBuffer:
	glslViewer test_doubleBuffer.frag -l

test_poisson_fill:
	glslViewer test_poisson_fill.frag image.png -l

test_background:
	glslViewer test_background.frag head.ply -l

test_background_quilt:
	glslViewer test_background.frag head.ply --quilt 0

test_background_lenticular:
	glslViewer test_background.frag head.ply --lenticular
	
test_postprocessing:
	glslViewer test_postprocessing.frag head.ply -l

test_sceneDepth:
	glslViewer test_sceneDepth.frag head.ply -e debug,on -l