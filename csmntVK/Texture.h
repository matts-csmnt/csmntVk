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
	void createTextureImageView(VkDevice&);

	const VkImage& getVkImage() const { return m_textureImage; };
	const VkDeviceMemory& getVkImageMem() const { return m_textureImageMemory; };
	const VkImageView& getVkImageView() const { return m_textureImageView; };

private:
	VkImage			m_textureImage;
	VkDeviceMemory	m_textureImageMemory;
	VkImageView		m_textureImageView;
};