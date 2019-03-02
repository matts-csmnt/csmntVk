#pragma once
#include <vulkan/vulkan.h>

class csmntVkApplication;

class Texture {
public:
	Texture() {};
	Texture(csmntVkApplication*, VkCommandPool&, const char*, const int);
	~Texture() {};

	void cleanupTexture(csmntVkApplication*);
	void createTextureImage(csmntVkApplication*, VkCommandPool&, const char*, const int);
	void createTextureImageView();

private:
	VkImage			m_textureImage;
	VkDeviceMemory	m_textureImageMemory;
	VkImageView		m_textureImageView;
};