#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class csmntVkApplication;

class Texture {
public:
	Texture() {};
	Texture(csmntVkApplication*, const char*, const int);
	~Texture() {};

private:
	void createTextureImage(csmntVkApplication*, const char*, const int);
};