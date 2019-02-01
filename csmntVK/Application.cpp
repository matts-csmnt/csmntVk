#include "Application.h"
#include <stdexcept>
#include <iostream>
#include <set>
#include <algorithm>

#ifndef _vk_details_h
#define _vk_details_h
#include "vkDetailsStructs.h"
#endif

csmntVkApplication::csmntVkApplication(int winW, int winH)
	: m_winH(winH), m_winW(winW), m_pWindow(nullptr) 
{
	//Add a validation layer
	m_validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");

	//Add required extensions -- swap chain
	m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

#if _DEBUG
	std::cout << "csmntVK Application Create" << std::endl;
#endif
}

csmntVkApplication::~csmntVkApplication()
{
#if _DEBUG
	std::cout << "csmntVK Application Destroy" << std::endl;
#endif
}

void csmntVkApplication::run()
{
	//Init window and vulkan
	initWindow();
	initVulkan();

	//Run until error or closed
	mainLoop();

	//Cleanup application
	shutdown();
}

void csmntVkApplication::mainLoop()
{
	//Run window until error or closed
	while (!glfwWindowShouldClose(m_pWindow)) {
		glfwPollEvents();

		//Render Frame
		m_pGraphics->drawFrame(m_vkDevice, m_vkSwapChain, m_vkGraphicsQueue, m_vkPresentQueue);

		//all of the operations in drawFrame are asynchronous. That means that when we exit the 
		//loop in mainLoop, drawing and presentation operations may still be going on. Cleaning 
		//up resources while that is happening is a bad idea.
		//(https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation)
		vkDeviceWaitIdle(m_vkDevice);
	}
}

void csmntVkApplication::shutdown()
{
	if (m_pGraphics)
	{
		m_pGraphics->shutdown(m_vkDevice);
		delete m_pGraphics;
		m_pGraphics = nullptr;
	}

	for (auto imageView : m_vkSwapChainImageViews) {
		vkDestroyImageView(m_vkDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);

	vkDestroyDevice(m_vkDevice, nullptr);

	if (m_enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);

	vkDestroyInstance(m_vkInstance, nullptr);

	glfwDestroyWindow(m_pWindow);

	glfwTerminate();
}

void csmntVkApplication::initVulkan()
{
	//Create instance and debug callbacks
	createVkInstance();
	setupDebugMessenger();

	//Create the window surface
	createSurface();

	//Pick a GFX card
	pickPhysicalDevice();

	//Create Logical device to interface with GFX card
	createLogicalDevice();

	//Create swap chain
	createSwapChain();

	//Create Swap Chain image views
	createImageViews();

	//Create the graphics pipeline
	createGraphicsPipeline();
}

void csmntVkApplication::initWindow()
{
	//Init glfw library
	glfwInit();

	//Window to be created WITHOUT openGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//Not resizable (TODO: fix this)
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	//Create and store our window
	m_pWindow = glfwCreateWindow(m_winW, m_winH, "csmntVK - Vulkan Framework", nullptr, nullptr);

#if _DEBUG
	std::cout << "glfw window instance created" << std::endl;
#endif
}

std::vector<const char*> csmntVkApplication::getRequiredExtensions() {
	
	uint32_t glfwExtensionCount = 0;
	
	//Pass glfw window extensions to vulkan
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	//Vulkan debug exts
	if (m_enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool csmntVkApplication::checkValidationLayerSupport()
{
	//Look for layers
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	
	//Add layer data
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	//check requested layers exist and are available
	for (const char* layerName : m_validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void csmntVkApplication::setupDebugMessenger()
{
	//Early out if Valid Layers turned off
	if (!m_enableValidationLayers) return;

	//Create debug messenger info
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional

	if (CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void csmntVkApplication::createVkInstance()
{
	//Check validation layers
	if (m_enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	//Create application info
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "csmntVK";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	//Create instance info
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	//Pass required extensions to vulkan
	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	//Validation layers
	if (m_enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//Create instance and check for errors
	if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vulkan instance!");
	}

#if _DEBUG
	std::cout << "vulkan instance created" << std::endl;

	//List vulkan extensions
	std::cout << "available vk extensions:" << std::endl;

	for (const auto& extension : extensions) {
		std::cout << "\t" << extension << std::endl;
	}
#endif
}

void csmntVkApplication::createSurface()
{
	//glfw handles multiplat surface creation
	if (glfwCreateWindowSurface(m_vkInstance, m_pWindow, nullptr, &m_vkSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void csmntVkApplication::createSwapChain()
{
	//find supported swap chain info
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_vkPhysicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	//Images in the swap chain -- try settle for min + 1, else just go for max
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//Begin creating the swap chain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_vkSurface;
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
	QueueFamilyIndices indices = findQueueFamilies(m_vkPhysicalDevice, m_vkSurface);
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

	if (vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_vkSwapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}
	
	//get handles to images created
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, nullptr);
	m_vkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, m_vkSwapChainImages.data());

	//Store sfc format and extent
	m_vkSwapChainImageFormat = surfaceFormat.format;
	m_vkSwapChainExtent = extent;
}

void csmntVkApplication::createImageViews()
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

		if (vkCreateImageView(m_vkDevice, &createInfo, nullptr, &m_vkSwapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void csmntVkApplication::createGraphicsPipeline()
{
	m_pGraphics = new csmntVkGraphics();

	if (!m_pGraphics)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
		return;
	}

	m_pGraphics->createGraphicsPipeline(m_vkDevice, m_vkSwapChainExtent, m_vkSwapChainImageFormat,
										m_vkPhysicalDevice, m_vkSurface, m_vkSwapChainImageViews);
}

void csmntVkApplication::pickPhysicalDevice()
{
	//Look for GFX devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

	//Check our devices are suitable
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			m_vkPhysicalDevice = device;
			break;
		}
	}

	if (m_vkPhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

bool csmntVkApplication::isDeviceSuitable(VkPhysicalDevice device) 
{
	//Get device properties & features
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	//Check features here...

	//Check for devices that can handle commands we want to use
	QueueFamilyIndices indices = findQueueFamilies(device, m_vkSurface);

	//Check supported extensions
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	//Check for swap chain support
	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void csmntVkApplication::createLogicalDevice()
{
	//Find and describe a Queue family with GFX capabilities
	QueueFamilyIndices indices = findQueueFamilies(m_vkPhysicalDevice, m_vkSurface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//Priority for scheduling command buffer execution
	float queuePriority = 1.0f;

	//Find and store unique queue families
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	//Features (queried before with vkGetPhysicalDeviceFeatures)
	//No features required just yet
	VkPhysicalDeviceFeatures deviceFeatures = {};

	//Create the logical device
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	//Features requested here...
	createInfo.pEnabledFeatures = &deviceFeatures;

	//Enable swap chain... etc
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
	
	//Add validation layers to the device
	if (m_enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//Create!
	if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	//Get the device queue
	vkGetDeviceQueue(m_vkDevice, indices.graphicsFamily.value(), 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkDevice, indices.presentFamily.value(), 0, &m_vkPresentQueue);
}

bool csmntVkApplication::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	//Enumerate extensions
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	//Check all required extensions in available extensions
	std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails csmntVkApplication::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	//Basic surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vkSurface, &details.capabilities);

	//Formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, details.formats.data());
	}

	//Presentation modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR csmntVkApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR csmntVkApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
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

VkExtent2D csmntVkApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
{
	//swap chain resolution - usually winwow res, but we can sometimes do better
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { m_winW, m_winH };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL csmntVkApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}
