#include "Texture.h"
#include <stdexcept>
#include "vkHelpers.h"
#include "Application.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(csmntVkApplication* pApp, VkCommandPool& cmdPool, const char* path, const int mode = STBI_rgb_alpha)
{
	createTextureImage(pApp, cmdPool, path, mode);
	createTextureImageView(pApp->getVkDevice());
}

void Texture::createTextureImage(csmntVkApplication* pApp, VkCommandPool& cmdPool, const char* path, const int mode = STBI_rgb_alpha)
{
	//TODO: map between stb & vk image formats?

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

	vkHelpers::createVkBuffer(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), imageSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, stagingBufferMemory);
	vkMapMemory(pApp->getVkDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(pApp->getVkDevice(), stagingBufferMemory);

	stbi_image_free(pixels);

	vkHelpers::createVkImage(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), texWidth, texHeight, 
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		m_textureImage, m_textureImageMemory);

	//Store the buffer
	vkHelpers::transitionVkImageLayout(pApp, cmdPool, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkHelpers::copyBufferToVkImage(pApp, cmdPool, stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	//transition to shader usage
	vkHelpers::transitionVkImageLayout(pApp, cmdPool, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//cleanup
	vkDestroyBuffer(pApp->getVkDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pApp->getVkDevice(), stagingBufferMemory, nullptr);
}

void Texture::createTextureImageView(VkDevice& device)
{
	m_textureImageView = vkHelpers::createVkImageView(device, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM);
}

void Texture::cleanupTexture(csmntVkApplication* pApp)
{
	vkDestroyImageView(pApp->getVkDevice(), m_textureImageView, nullptr);
	vkDestroyImage(pApp->getVkDevice(), m_textureImage, nullptr);
	vkFreeMemory(pApp->getVkDevice(), m_textureImageMemory, nullptr);
}
