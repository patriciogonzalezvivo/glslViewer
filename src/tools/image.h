#pragma once

#include <string>

enum Channels {
	LUMINANCE = 1,
	LUMINANCE_ALPHA = 2,
	RGB = 3,
	RGB_ALPHA = 4
};

bool 	        savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height);
float*          loadFloatPixels(const std::string& _path, int *_width, int *_height, bool _vFlip = true);
unsigned char*  loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);