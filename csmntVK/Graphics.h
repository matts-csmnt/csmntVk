#pragma once
#ifndef _VK_GRAPHICS_CLASS_
#define _VK_GRAPHICS_CLASS_

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>

#include "vkDetailsStructs.h"
#include "Model.h"
#include "Texture.h"

//Graphics knows about Application, for passing params easier
class csmntVkApplication;

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

	void shutdown(csmntVkApplication*);

	void initGraphicsModule(csmntVkApplication*, SwapChainSupportDetails&);

	void drawFrame(csmntVkApplication*, SwapChainSupportDetails&);

	void recreateSwapChain(csmntVkApplication*, SwapChainSupportDetails&);

private:
	//How many frames should be processed concurrently?
	const int					m_MAX_FRAMES_IN_FLIGHT = 2;
	size_t						m_currentFrame = 0;

	VkSwapchainKHR				m_vkSwapChain;
	std::vector<VkImage>		m_vkSwapChainImages;
	VkFormat					m_vkSwapChainImageFormat;
	VkExtent2D					m_vkSwapChainExtent;
	std::vector<VkImageView>    m_vkSwapChainImageViews;

	VkDescriptorSetLayout		m_vkDescriptorSetLayout;
	VkPipelineLayout			m_vkPipelineLayout;
	VkRenderPass				m_vkRenderPass;
	VkPipeline					m_vkGraphicsPipeline;
	std::vector<VkFramebuffer>	m_vkSwapChainFramebuffers;
	VkCommandPool				m_vkCommandPool;
	std::vector<VkCommandBuffer> m_vkCommandBuffers;

	std::vector<VkSemaphore>	m_vkImageAvailableSemaphores;
	std::vector<VkSemaphore>	m_vkRenderFinishedSemaphores;
	std::vector<VkFence>		m_vkInFlightFences;

	VkBuffer					m_vkVertexBuffer;
	VkDeviceMemory				m_vkVertexBufferMemory;
	VkBuffer					m_vkIndexBuffer;
	VkDeviceMemory				m_vkIndexBufferMemory;
	uint16_t					m_vkIndexCount;

	std::vector<VkBuffer>		m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;

	VkDescriptorPool			m_vkDescriptorPool;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;

	//Models etc... for testing
	Model*						m_pModel;
	Texture*					m_pTexture;

	VkSampler					m_linearTexSampler;

	//Depth Buffer
	VkImage						m_vkDepthImage;
	VkDeviceMemory				m_vkDepthImageMemory;
	VkImageView					m_vkDepthImageView;

	void createSwapChain(csmntVkApplication*, SwapChainSupportDetails&);

	void createImageViews(VkDevice&);

	void cleanupSwapChain(VkDevice&);

	void createDescriptorSetLayout(VkDevice&);

	void createPipeline(VkDevice&);
	void createRenderPass(csmntVkApplication*);
	void createFramebuffers(VkDevice&);
	void createCommandPool(VkDevice&, VkPhysicalDevice&, VkSurfaceKHR&);
	
	void createVertexBuffer(csmntVkApplication*);
	void createIndexBuffer(csmntVkApplication*);
	void createUniformBuffers(csmntVkApplication*);
	void updateUniformBuffer(uint32_t, VkDevice&);

	void createTextureSampler(csmntVkApplication*);

	//Depth Buffer
	void createDepthResources(csmntVkApplication*);
	void cleanupDepthResources(csmntVkApplication*);
	VkFormat findDepthFormat(VkPhysicalDevice&);

	void createTexture(csmntVkApplication*);
	void cleanupTexture(csmntVkApplication*);

	void createDescriptorPool(VkDevice&);
	void createDescriptorSets(VkDevice&);

	void createCommandBuffers(VkDevice&);
	void createSemaphoresAndFences(VkDevice&);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&, GLFWwindow*);

	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice*);
};

#endif