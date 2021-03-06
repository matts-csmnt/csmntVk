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

#include "Application.h"
#include "vkHelpers.h"

#define GLM_FORCE_RADIANS
#include "../Libraries/glm/glm.hpp"
#include "../Libraries/glm/gtc/matrix_transform.hpp"

#include <chrono>

#include "uniformBuffer.h"

#pragma region CTOR & DTOR
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
#pragma endregion

#pragma region INIT & SHUTDOWN
void csmntVkGraphics::initGraphicsModule(csmntVkApplication* pApp, SwapChainSupportDetails& swapChainSupport)
{
	//Create models
	m_pModel = new Model();

	//Create all required functionality for graphics pipeline
	createSwapChain(pApp, swapChainSupport);
	createImageViews(pApp->getVkDevice());

	createRenderPass(pApp);

	createDescriptorSetLayout(pApp->getVkDevice());

	createPipeline(pApp->getVkDevice());

	createCommandPool(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), pApp->getVkSurfaceKHR());

	//Depth Buffer
	createDepthResources(pApp);

	createFramebuffers(pApp->getVkDevice());

	//Textures
	createTexture(pApp);
	createTextureSampler(pApp);

	createVertexBuffer(pApp);
	createIndexBuffer(pApp);
	createUniformBuffers(pApp);

	createDescriptorPool(pApp->getVkDevice());
	createDescriptorSets(pApp->getVkDevice());

	createCommandBuffers(pApp->getVkDevice());

	createSemaphoresAndFences(pApp->getVkDevice());
}

void csmntVkGraphics::shutdown(csmntVkApplication* pApp)
{
	cleanupSwapChain(pApp->getVkDevice());

	vkDestroyDescriptorPool(pApp->getVkDevice(), m_vkDescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(pApp->getVkDevice(), m_vkDescriptorSetLayout, nullptr);

	for (size_t i = 0; i < m_vkSwapChainImages.size(); i++) {
		vkDestroyBuffer(pApp->getVkDevice(), m_uniformBuffers[i], nullptr);
		vkFreeMemory(pApp->getVkDevice(), m_uniformBuffersMemory[i], nullptr);
	}

	//buffers
	vkDestroyBuffer(pApp->getVkDevice(), m_vkIndexBuffer, nullptr);
	vkFreeMemory(pApp->getVkDevice(), m_vkIndexBufferMemory, nullptr);

	vkDestroyBuffer(pApp->getVkDevice(), m_vkVertexBuffer, nullptr);
	vkFreeMemory(pApp->getVkDevice(), m_vkVertexBufferMemory, nullptr);

	for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(pApp->getVkDevice(), m_vkRenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(pApp->getVkDevice(), m_vkImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(pApp->getVkDevice(), m_vkInFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(pApp->getVkDevice(), m_vkCommandPool, nullptr);

	//Cleanup Models
	if (m_pModel)
	{
		delete m_pModel;
		m_pModel = nullptr;
	}

	vkDestroySampler(pApp->getVkDevice(), m_linearTexSampler, nullptr);

	cleanupTexture(pApp);
}
#pragma endregion

#pragma region CREATIONS
void csmntVkGraphics::createDescriptorSetLayout(VkDevice& device) 
{
	//uniform buffers
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	//image sampler
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//create the layout info
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void csmntVkGraphics::createDescriptorSets(VkDevice& device)
{
	std::vector<VkDescriptorSetLayout> layouts(m_vkSwapChainImages.size(), m_vkDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_vkDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_vkSwapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	m_vkDescriptorSets.resize(m_vkSwapChainImages.size());
	if (vkAllocateDescriptorSets(device, &allocInfo, m_vkDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < m_vkSwapChainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_pTexture->getVkImageView();
		imageInfo.sampler = m_linearTexSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_vkDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_vkDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void csmntVkGraphics::createPipeline(VkDevice& device)
{
	auto vertShaderCode = vkHelpers::readFile("../Shaders/vert.spv");
	auto fragShaderCode = vkHelpers::readFile("../Shaders/frag.spv");

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

	//Vertex Input - from Model Vertex format
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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

	//Depth Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional

	//Pipeline Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_vkDescriptorSetLayout;
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
	pipelineInfo.pDepthStencilState = &depthStencil;
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

void csmntVkGraphics::createRenderPass(csmntVkApplication* pApp)
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

	//Subpass Dependency - make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//Depth
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat(pApp->getVkPhysicalDevice());
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//Subpass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	//Create the pass
	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(pApp->getVkDevice(), &renderPassInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void csmntVkGraphics::createFramebuffers(VkDevice& device)
{
	m_vkSwapChainFramebuffers.resize(m_vkSwapChainImageViews.size());

	for (size_t i = 0; i < m_vkSwapChainImageViews.size(); i++) {
		
		std::array<VkImageView, 2> attachments = {
			m_vkSwapChainImageViews[i],
			m_vkDepthImageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_vkRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
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

void csmntVkGraphics::createVertexBuffer(csmntVkApplication* pApp)
{
	//Vertices from models
	const std::vector<Vertex> verts = m_pModel->getVertices();

	VkDeviceSize bufferSize = sizeof(verts[0]) * verts.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vkHelpers::createVkBuffer(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(pApp->getVkDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, verts.data(), (size_t)bufferSize);
	vkUnmapMemory(pApp->getVkDevice(), stagingBufferMemory);

	vkHelpers::createVkBuffer(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT
		| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vkVertexBuffer, m_vkVertexBufferMemory);
	vkHelpers::copyVkBuffer(pApp, stagingBuffer, m_vkVertexBuffer, bufferSize, m_vkCommandPool);

	vkDestroyBuffer(pApp->getVkDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pApp->getVkDevice(), stagingBufferMemory, nullptr);
}

void csmntVkGraphics::createIndexBuffer(csmntVkApplication* pApp)
{
	//indices from model
	const std::vector<uint16_t> indices = m_pModel->getIndices();
	m_vkIndexCount = indices.size();

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vkHelpers::createVkBuffer(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(pApp->getVkDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(pApp->getVkDevice(), stagingBufferMemory);

	vkHelpers::createVkBuffer(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT
		| VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vkIndexBuffer, m_vkIndexBufferMemory);

	vkHelpers::copyVkBuffer(pApp, stagingBuffer, m_vkIndexBuffer, bufferSize, m_vkCommandPool);

	vkDestroyBuffer(pApp->getVkDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pApp->getVkDevice(), stagingBufferMemory, nullptr);
}

void csmntVkGraphics::createUniformBuffers(csmntVkApplication* pApp) {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_uniformBuffers.resize(m_vkSwapChainImages.size());
	m_uniformBuffersMemory.resize(m_vkSwapChainImages.size());

	for (size_t i = 0; i < m_vkSwapChainImages.size(); i++) {
		vkHelpers::createVkBuffer(pApp->getVkDevice(), pApp->getVkPhysicalDevice(), bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			m_uniformBuffers[i], m_uniformBuffersMemory[i]);
	}
}

void csmntVkGraphics::createCommandBuffers(VkDevice& device)
{
	m_vkCommandBuffers.resize(m_vkSwapChainFramebuffers.size());

	//Clear values
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

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

		//Clear to clear values
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_vkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);

		//Drawing -- offsets for gathering multiple buffers and drawing
		VkBuffer vertexBuffers[] = { m_vkVertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_vkCommandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_vkCommandBuffers[i], m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout,
			0, 1, &m_vkDescriptorSets[i], 0, nullptr);

		//vkCmdDraw(m_vkCommandBuffers[i], static_cast<uint32_t>(m_pModel->getVertices().size()), 1, 0, 0);
		vkCmdDrawIndexed(m_vkCommandBuffers[i], static_cast<uint32_t>(m_vkIndexCount), 1, 0, 0, 0);

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

void csmntVkGraphics::createSwapChain(csmntVkApplication* pApp, SwapChainSupportDetails& swapChainSupport)
{
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, pApp->getWindow());

	//Images in the swap chain -- try settle for min + 1, else just go for max
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//Begin creating the swap chain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = pApp->getVkSurfaceKHR();
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
	QueueFamilyIndices indices = findQueueFamilies(pApp->getVkPhysicalDevice(), pApp->getVkSurfaceKHR());
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

	if (vkCreateSwapchainKHR(pApp->getVkDevice(), &createInfo, nullptr, &m_vkSwapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	//get handles to images created
	vkGetSwapchainImagesKHR(pApp->getVkDevice(), m_vkSwapChain, &imageCount, nullptr);
	m_vkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(pApp->getVkDevice(), m_vkSwapChain, &imageCount, m_vkSwapChainImages.data());

	//Store sfc format and extent
	m_vkSwapChainImageFormat = surfaceFormat.format;
	m_vkSwapChainExtent = extent;
}

void csmntVkGraphics::createImageViews(VkDevice& device)
{
	m_vkSwapChainImageViews.resize(m_vkSwapChainImages.size());

	//Iterate over all images 
	for (size_t i = 0; i < m_vkSwapChainImageViews.size(); i++) 
	{
		m_vkSwapChainImageViews[i] = vkHelpers::createVkImageView(device, m_vkSwapChainImages[i], m_vkSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void csmntVkGraphics::createDescriptorPool(VkDevice& device)
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	//uniform buffers
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(m_vkSwapChainImages.size());
	//image sampler
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(m_vkSwapChainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(m_vkSwapChainImages.size());

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void csmntVkGraphics::createTexture(csmntVkApplication* pApp)
{
	m_pTexture = new Texture(pApp, m_vkCommandPool, "../Assets/Textures/profile.png", 4);

	if (!m_pTexture)
	{
		std::runtime_error("failed to create the texture container!");
	}
}

void csmntVkGraphics::recreateSwapChain(csmntVkApplication* pApp, SwapChainSupportDetails& swapChainSupport)
{
	//Check for minimized window state
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(pApp->getWindow(), &width, &height);
		glfwWaitEvents();
	}

	//Recreate the swap chain if window surface changes to non compatible
	vkDeviceWaitIdle(pApp->getVkDevice());

	//cleanup
	cleanupSwapChain(pApp->getVkDevice());

	createSwapChain(pApp, swapChainSupport);
	createImageViews(pApp->getVkDevice());
	createRenderPass(pApp);
	createPipeline(pApp->getVkDevice());
	createDepthResources(pApp);
	createFramebuffers(pApp->getVkDevice());
	createCommandBuffers(pApp->getVkDevice());

#if _DEBUG
	static size_t creations = 0;
	++creations;
	std::cout << "HEY! recreated the swap chain [" << creations << " times]" << std::endl;
#endif
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

void csmntVkGraphics::createTextureSampler(csmntVkApplication* pApp)
{
	//Create a linear sampler w/Anistro
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(pApp->getVkDevice(), &samplerInfo, nullptr, &m_linearTexSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void csmntVkGraphics::createDepthResources(csmntVkApplication* pApp)
{
	VkFormat depthFormat = findDepthFormat(pApp->getVkPhysicalDevice());

	vkHelpers::createVkImage(pApp->getVkDevice(), pApp->getVkPhysicalDevice(),
		m_vkSwapChainExtent.width, m_vkSwapChainExtent.height, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		m_vkDepthImage, m_vkDepthImageMemory);
	m_vkDepthImageView = vkHelpers::createVkImageView(pApp->getVkDevice(), m_vkDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}
#pragma endregion

#pragma region EVERY FRAME
void csmntVkGraphics::drawFrame(csmntVkApplication* pApp, SwapChainSupportDetails& swapChainSupport)
{
	VkResult result;

	//Wait for frame to finish before continuing with another
	vkWaitForFences(pApp->getVkDevice(), 1, &m_vkInFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t imageIndex;
	result = vkAcquireNextImageKHR(pApp->getVkDevice(), m_vkSwapChain, std::numeric_limits<uint64_t>::max(), m_vkImageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	updateUniformBuffer(imageIndex, pApp->getVkDevice());

	//check if swapchain needs recreation
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		pApp->setIsFrameBufferResized(false);

		recreateSwapChain(pApp, swapChainSupport);
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
	vkResetFences(pApp->getVkDevice(), 1, &m_vkInFlightFences[m_currentFrame]);

	//submit queue and signal fence
	if (vkQueueSubmit(pApp->getGraphicsQueue(), 1, &submitInfo, m_vkInFlightFences[m_currentFrame]) != VK_SUCCESS) {
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

	result = vkQueuePresentKHR(pApp->getPresentQueue(), &presentInfo);

	//check swapchain again
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || pApp->getIsFrameBufferResized()) {
		pApp->setIsFrameBufferResized(false);
		recreateSwapChain(pApp, swapChainSupport);
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	//Advance frame
	m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;
}

void csmntVkGraphics::updateUniformBuffer(uint32_t currentImage, VkDevice& device)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), m_vkSwapChainExtent.width / (float)m_vkSwapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device, m_uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, m_uniformBuffersMemory[currentImage]);
}
#pragma endregion

#pragma region CLEANUP
void csmntVkGraphics::cleanupTexture(csmntVkApplication* pApp)
{
	if (m_pTexture)
	{
		m_pTexture->cleanupTexture(pApp);
		delete m_pTexture;
		m_pTexture = nullptr;
	}
}

void csmntVkGraphics::cleanupSwapChain(VkDevice& device)
{
	//cleanup depth buffer
	vkDestroyImageView(device, m_vkDepthImageView, nullptr);
	vkDestroyImage(device, m_vkDepthImage, nullptr);
	vkFreeMemory(device, m_vkDepthImageMemory, nullptr);

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
#pragma endregion

#pragma region SWAP CHAIN HELPERS
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
#pragma endregion

#pragma region DEPTH BUFFER HELPERS
VkFormat csmntVkGraphics::findDepthFormat(VkPhysicalDevice& physicalDevice) 
{
	return vkHelpers::findSupportedFormat(
		physicalDevice,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}
#pragma endregion