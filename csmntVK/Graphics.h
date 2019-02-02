#pragma once
#ifndef _VK_GRAPHICS_CLASS_
#define _VK_GRAPHICS_CLASS_

#include <vulkan/vulkan.h>
#include <vector>

#include "vkDetailsStructs.h"

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
	void initGraphicsModule(VkDevice&,VkPhysicalDevice&, VkSurfaceKHR&, const int, const int, SwapChainSupportDetails&);
	void drawFrame(VkDevice&, VkQueue&, VkQueue&);

private:
	//How many frames should be processed concurrently?
	const int m_MAX_FRAMES_IN_FLIGHT = 2;
	size_t currentFrame = 0;

	VkSwapchainKHR				m_vkSwapChain;
	std::vector<VkImage>		m_vkSwapChainImages;
	VkFormat					m_vkSwapChainImageFormat;
	VkExtent2D					m_vkSwapChainExtent;
	std::vector<VkImageView>    m_vkSwapChainImageViews;

	VkPipelineLayout			m_vkPipelineLayout;
	VkRenderPass				m_vkRenderPass;
	VkPipeline					m_vkGraphicsPipeline;
	std::vector<VkFramebuffer>	m_vkSwapChainFramebuffers;
	VkCommandPool				m_vkCommandPool;
	std::vector<VkCommandBuffer> m_vkCommandBuffers;

	std::vector<VkSemaphore> m_vkImageAvailableSemaphores;
	std::vector<VkSemaphore> m_vkRenderFinishedSemaphores;
	std::vector<VkFence> m_vkInFlightFences;

	void createSwapChain(VkDevice&, VkPhysicalDevice&, VkSurfaceKHR&, const int, const int, SwapChainSupportDetails&);
	void createImageViews(VkDevice&);

	void createPipeline(VkDevice&);
	void createRenderPass(VkDevice&);
	void createFramebuffers(VkDevice&);
	void createCommandPool(VkDevice&, VkPhysicalDevice&, VkSurfaceKHR&);
	void createCommandBuffers(VkDevice&);
	void createSemaphoresAndFences(VkDevice&);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&, const int, const int);

	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice*);
};

#endif