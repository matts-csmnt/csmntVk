#pragma once
#include <vulkan/vulkan.h>
class csmntVkApplication;

namespace vkBuffers {

	uint32_t findMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	//Buffer Helpers
	void createVkBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, VkDeviceSize& size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void copyVkBuffer(csmntVkApplication* pApp, VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize& size, VkCommandPool& cmdPool);
}