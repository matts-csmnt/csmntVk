#include "Texture.h"
#include <vulkan/vulkan.h>
#include <stdexcept>
#include "vkBuffers.h"
#include "Application.h"

Texture::Texture(csmntVkApplication* pApp, const char* path, const int mode = STBI_rgb_alpha)
{
	createTextureImage(pApp, path, mode);
}

void Texture::createTextureImage(csmntVkApplication* pApp, const char* path, const int mode = STBI_rgb_alpha)
{
	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, mode);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	//copy memory
	VkBuffer		stagingBuffer;
	VkDeviceMemory	stagingBufferMemory;
	void*			data;

	vkBuffers::createVkBuffer(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	vkMapMemory(pApp->getVkDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(pApp->getVkDevice(), stagingBufferMemory);

	stbi_image_free(pixels);
}
