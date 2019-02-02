#include "Graphics.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <set>
#include <algorithm>

#ifndef _vk_details_h
#define _vk_details_h
#include "vkDetailsStructs.h"
#endif

csmntVkGraphics::csmntVkGraphics()
{
#if _DEBUG
	std::cout << "HEY! csmntVK Graphics Module Created" << std::endl;
#endif
}

csmntVkGraphics::~csmntVkGraphics()
{
#if _DEBUG
	std::cout << "HEY! csmntVK Graphics Module Destroyed" << std::endl;
#endif
}

void csmntVkGraphics::shutdown(VkDevice& device)
{
	cleanupSwapChain(device);

	for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, m_vkRenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, m_vkImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, m_vkInFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, m_vkCommandPool, nullptr);
}

void csmntVkGraphics::initGraphicsModule(VkDevice& device, VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface, 
										 SwapChainSupportDetails& scSupportDetails, GLFWwindow* window)
{
	//Create all required functionality for graphics pipeline
	createSwapChain(device, physicalDevice, surface, scSupportDetails, window);
	createImageViews(device);

	createRenderPass(device);
	createPipeline(device);
	createFramebuffers(device);

	createCommandPool(device, physicalDevice, surface);
	createCommandBuffers(device);

	createSemaphoresAndFences(device);
}

void csmntVkGraphics::createPipeline(VkDevice& device)
{
	auto vertShaderCode = readFile("../Shaders/vert.spv");
	auto fragShaderCode = readFile("../Shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, &device);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, &device);

	//Vertex Shader
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	//pSpecializationInfo:  You can use a single shader module where its behavior can be 
	//configured at pipeline creation by specifying different values for the constants 
	//used in it. This is more efficient than configuring the shader using variables at 
	//render time, because the compiler can do optimizations like eliminating if statements 
	//that depend on these values.

	//Frag Shader
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	//Store Stages
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//Define a Viewport
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_vkSwapChainExtent.width;
	viewport.height = (float)m_vkSwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//Scissor rect
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = m_vkSwapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	//Rasteriser
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;

	//If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near
	//and far planes are clamped to them as opposed to discarding them.
	rasterizer.rasterizerDiscardEnable = VK_FALSE;

	//The polygonMode determines how fragments are generated for geometry. The following modes are available:
	//VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
	//VK_POLYGON_MODE_LINE : polygon edges are drawn as lines
	//VK_POLYGON_MODE_POINT : polygon vertices are drawn as points
	//Using any mode other than fill requires enabling a GPU feature.
	//(https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions)
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	//any line thicker than 1.0f requires you to enable the wideLines GPU feature.
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	//Multisampling (used for AA) Enabling it requires enabling a GPU feature.
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	//Colour blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	/*colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;*/
	// logicOpEnable to VK_TRUE

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	//A limited amount of the state that we've specified in the previous structs 
	//can actually be changed without recreating the pipeline. Examples are the 
	//size of the viewport, line width and blend constants.
	/*VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;*/

	//Pipeline Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	//Create the pipeline object
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional

	pipelineInfo.layout = m_vkPipelineLayout;

	pipelineInfo.renderPass = m_vkRenderPass;
	pipelineInfo.subpass = 0;

	//Single pipeline
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void csmntVkGraphics::createRenderPass(VkDevice& device)
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_vkSwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//Subpasses
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	//Subpass Dependency - make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//Create the pass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void csmntVkGraphics::createFramebuffers(VkDevice& device)
{
	m_vkSwapChainFramebuffers.resize(m_vkSwapChainImageViews.size());

	for (size_t i = 0; i < m_vkSwapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			m_vkSwapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_vkRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_vkSwapChainExtent.width;
		framebufferInfo.height = m_vkSwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_vkSwapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void csmntVkGraphics::createCommandPool(VkDevice& device, VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface)
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0; // Optional

	//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: 
	//Hint that command buffers are rerecorded with new commands very often 
	//(may change memory allocation behavior)
	//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: 
	//Allow command buffers to be rerecorded individually, without this flag 
	//they all have to be reset together
	//(https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers)

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void csmntVkGraphics::createCommandBuffers(VkDevice& device)
{
	m_vkCommandBuffers.resize(m_vkSwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_vkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();

	//VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, 
	//but cannot be called from other command buffers.
	//VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be 
	//called from primary command buffers.
	//(https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers)

	if (vkAllocateCommandBuffers(device, &allocInfo, m_vkCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	//Record comand buffers
	for (size_t i = 0; i < m_vkCommandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(m_vkCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		//Render Pass
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vkRenderPass;
		renderPassInfo.framebuffer = m_vkSwapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_vkSwapChainExtent;

		//Clear to Black
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_vkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);

		vkCmdDraw(m_vkCommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_vkCommandBuffers[i]);

		if (vkEndCommandBuffer(m_vkCommandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void csmntVkGraphics::createSemaphoresAndFences(VkDevice& device)
{
	m_vkImageAvailableSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
	m_vkRenderFinishedSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
	m_vkInFlightFences.resize(m_MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	//Signal the fence to simulate an initial frame
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &m_vkInFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("failed to create synchronization objects (semaphore or fence) for a frame!");
		}
	}
}

void csmntVkGraphics::drawFrame(VkDevice& device, VkQueue& graphicsQueue, VkQueue& presentQueue,
								VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface,
								SwapChainSupportDetails& swapChainSupport, GLFWwindow* window,
								bool& isFramebufferResized)
{
	VkResult result;

	//Wait for frame to finish before continuing with another
	vkWaitForFences(device, 1, &m_vkInFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t imageIndex;
	result = vkAcquireNextImageKHR(device, m_vkSwapChain, std::numeric_limits<uint64_t>::max(), m_vkImageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	//check if swapchain needs recreation
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		isFramebufferResized = false;

		recreateSwapChain(device,physicalDevice,surface,swapChainSupport,window);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_vkImageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	//bind command buffer
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vkCommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_vkRenderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	//Reset the fences after we check for swapchain recreation etc...
	vkResetFences(device, 1, &m_vkInFlightFences[m_currentFrame]);

	//submit queue and signal fence
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_vkInFlightFences[m_currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	//Present to swap chain
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_vkSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	//allows you to specify an array of VkResult values to check for every individual 
	//swap chain if presentation was successful. It's not necessary if you're only using 
	//a single swap chain, because you can simply use the return value of the present function
	//(https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation)
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	//check swapchain again
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isFramebufferResized) {
		isFramebufferResized = false;
		recreateSwapChain(device, physicalDevice, surface, swapChainSupport, window);
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	//Advance frame
	m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;
}

void csmntVkGraphics::recreateSwapChain(VkDevice& device, VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface,
										SwapChainSupportDetails& swapChainSupport, GLFWwindow* window)
{
	//Check for minimized window state
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	//Recreate the swap chain if window surface changes to non compatible
	vkDeviceWaitIdle(device);

	//cleanup
	cleanupSwapChain(device);

	createSwapChain(device, physicalDevice, surface, swapChainSupport, window);
	createImageViews(device);
	createRenderPass(device);
	createPipeline(device);
	createFramebuffers(device);
	createCommandBuffers(device);

#if _DEBUG
	static size_t creations = 0;
	++creations;
	std::cout << "HEY! recreated the swap chain [" << creations << " times]" << std::endl;
#endif
}

void csmntVkGraphics::createSwapChain(VkDevice& device, VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface, 
									  SwapChainSupportDetails& swapChainSupport, GLFWwindow* window)
{
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

	//Images in the swap chain -- try settle for min + 1, else just go for max
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//Begin creating the swap chain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	//to perform operations like post - processing [...] you may use a value 
	//like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation 
	//to transfer the rendered image to a swap chain image. (https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain)
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//Decide if images are exclusive to queue families or concurrent
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	//It's possible to apply transforms to images like rotations... cool
	//Just use the current one as default
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

	//Ignore the alpha channel for now, usually for blending with other windows etc.
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	//Clip for performance (if windows obscure etc) 
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	//resizing the window etc requires chains to be created from scratch...
	//here's where you have to reference the dead one
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_vkSwapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	//get handles to images created
	vkGetSwapchainImagesKHR(device, m_vkSwapChain, &imageCount, nullptr);
	m_vkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, m_vkSwapChain, &imageCount, m_vkSwapChainImages.data());

	//Store sfc format and extent
	m_vkSwapChainImageFormat = surfaceFormat.format;
	m_vkSwapChainExtent = extent;
}

void csmntVkGraphics::createImageViews(VkDevice& device)
{
	m_vkSwapChainImageViews.resize(m_vkSwapChainImages.size());

	//Iterate over all images 
	for (size_t i = 0; i < m_vkSwapChainImageViews.size(); i++) {
		//2d image view types
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_vkSwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_vkSwapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &createInfo, nullptr, &m_vkSwapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void csmntVkGraphics::cleanupSwapChain(VkDevice& device)
{
	//Destroy all framebuffers
	for (auto framebuffer : m_vkSwapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	//free up command buffers
	vkFreeCommandBuffers(device, m_vkCommandPool, static_cast<uint32_t>(m_vkCommandBuffers.size()), m_vkCommandBuffers.data());

	vkDestroyPipeline(device, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, m_vkPipelineLayout, nullptr);
	vkDestroyRenderPass(device, m_vkRenderPass, nullptr);

	//destroy all image views
	for (auto imageView : m_vkSwapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, m_vkSwapChain, nullptr);
}

std::vector<char> csmntVkGraphics::readFile(const std::string & filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkShaderModule csmntVkGraphics::createShaderModule(const std::vector<char>& code, VkDevice* device)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

VkSurfaceFormatKHR csmntVkGraphics::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	//if vulkan finds no preferred format, use SRGB, BGRA 8
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	//if vulkan finds preferred formats, look for our ideal
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	//if it isn't there just return first, it'll do
	//TODO: rank formats and select?
	return availableFormats[0];
}

VkPresentModeKHR csmntVkGraphics::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	//should always be available
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	//look for mailbox PM:
	//triple buffering is a very nice trade-off. It allows us to avoid tearing while 
	//still maintaining a fairly low latency by rendering new images that are as up-to-date 
	//as possible right until the vertical blank (https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain)
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
		//FIFO is sometimes unsupported by drivers... boo
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D csmntVkGraphics::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow* window)
{
	//swap chain resolution - usually winwow res, but we can sometimes do better
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;

		//get the actual frame buffer size
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
