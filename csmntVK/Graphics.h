#pragma once
#ifndef _VK_GRAPHICS_CLASS_
#define _VK_GRAPHICS_CLASS_

#include <vulkan/vulkan.h>
#include <vector>

/////////////////////////////////////////////////////
//---csmntVkGraphics:
//---Handles Graphics Pipeline (Shaders...)
/////////////////////////////////////////////////////

class csmntVkGraphics {
public:
	csmntVkGraphics();
	~csmntVkGraphics();
	csmntVkGraphics(csmntVkGraphics&) = delete;
	csmntVkGraphics& operator=(const csmntVkGraphics&) = delete;

	void shutdown(VkDevice&);
	void createGraphicsPipeline(VkDevice&, VkExtent2D&, VkFormat&, 
								VkPhysicalDevice&, VkSurfaceKHR&, 
								std::vector<VkImageView>&);

	void drawFrame(VkDevice&, VkSwapchainKHR&, VkQueue&, VkQueue&);

private:
	//How many frames should be processed concurrently?
	const int m_MAX_FRAMES_IN_FLIGHT = 2;
	size_t currentFrame = 0;

	VkPipelineLayout m_vkPipelineLayout;
	VkRenderPass     m_vkRenderPass;
	VkPipeline		 m_vkGraphicsPipeline;
	std::vector<VkFramebuffer> m_vkSwapChainFramebuffers;
	VkCommandPool	 m_vkCommandPool;
	std::vector<VkCommandBuffer> m_vkCommandBuffers;

	std::vector<VkSemaphore> m_vkImageAvailableSemaphores;
	std::vector<VkSemaphore> m_vkRenderFinishedSemaphores;

	std::vector<VkFence> m_vkInFlightFences;

	void createPipeline(VkDevice&, VkExtent2D&);
	void createRenderPass(VkDevice&, VkFormat&);
	void createFramebuffers(VkDevice&, std::vector<VkImageView>&, VkExtent2D&);
	void createCommandPool(VkDevice&, VkPhysicalDevice&, VkSurfaceKHR&);
	void createCommandBuffers(VkDevice&, VkExtent2D&);
	void createSemaphoresAndFences(VkDevice&);

	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice*);
};

#endif