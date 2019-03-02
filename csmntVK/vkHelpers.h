#pragma once
#include <vulkan/vulkan.h>
#include <vector>
class csmntVkApplication;

namespace vkHelpers {

	uint32_t findMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	//Buffer Helpers
	void createVkBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, VkDeviceSize& size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyVkBuffer(csmntVkApplication* pApp, VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize& size, VkCommandPool& cmdPool);

	//Command Buffers
	VkCommandBuffer beginSingleTimeCommands(VkCommandPool& cmdPool, VkDevice& device);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue& queue, VkDevice& device, VkCommandPool& cmdPool);

	//Image Creation
	void createVkImage(VkDevice& device, VkPhysicalDevice& physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionVkImageLayout(csmntVkApplication* pApp, VkCommandPool& cmdPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToVkImage(csmntVkApplication* pApp, VkCommandPool& cmdPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	//File Reading
	std::vector<char> readFile(const std::string & filename);
}