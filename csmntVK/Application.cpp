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
	: m_winW(winW), m_winH(winH), m_pWindow(nullptr)
{
	//Add a validation layer
	m_validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");

	//Add required extensions -- swap chain
	m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

#if _DEBUG
	std::cout << "HEY! csmntVK Application Created" << std::endl;
#endif
}

csmntVkApplication::~csmntVkApplication()
{
#if _DEBUG
	std::cout << "HEY! csmntVK Application Destroyed" << std::endl;
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
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_vkPhysicalDevice);
		m_pGraphics->drawFrame(m_vkDevice, m_vkGraphicsQueue, m_vkPresentQueue, m_vkPhysicalDevice, 
								m_vkSurface, swapChainSupport, m_pWindow, m_frameBufferResized);

		//all of the operations in drawFrame are asynchronous. That means that when we exit the 
		//loop in mainLoop, drawing and presentation operations may still be going on. Cleaning 
		//up resources while that is happening is a bad idea.
		//(https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation)
		vkDeviceWaitIdle(m_vkDevice);
	}
}

void csmntVkApplication::initGraphicsModule()
{
	m_pGraphics = new csmntVkGraphics();

	if (!m_pGraphics)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
		return;
	}

	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_vkPhysicalDevice);

	m_pGraphics->initGraphicsModule(m_vkDevice, m_vkPhysicalDevice, m_vkSurface, swapChainSupport, m_pWindow);
}

void csmntVkApplication::shutdown()
{
	if (m_pGraphics)
	{
		m_pGraphics->shutdown(m_vkDevice);
		delete m_pGraphics;
		m_pGraphics = nullptr;
	}

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

	//Create the graphics module
	initGraphicsModule();
}

void csmntVkApplication::initWindow()
{
	//Init glfw library
	glfwInit();

	//Window to be created WITHOUT openGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//Create and store our window
	m_pWindow = glfwCreateWindow(m_winW, m_winH, "csmntVK - Vulkan Framework", nullptr, nullptr);

	//Set the framebuffer resize callback
	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetFramebufferSizeCallback(m_pWindow, framebufferResizeCallback);

#if _DEBUG
	std::cout << "HEY! glfw window instance created" << std::endl;
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
	std::cout << "HEY! vulkan instance created" << std::endl;

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

SwapChainSupportDetails csmntVkApplication::querySwapChainSupport(VkPhysicalDevice& device)
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

VKAPI_ATTR VkBool32 VKAPI_CALL csmntVkApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void csmntVkApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<csmntVkApplication*>(glfwGetWindowUserPointer(window));
	app->m_frameBufferResized = true;
}
