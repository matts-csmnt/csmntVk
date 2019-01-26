#include "Application.h"
#include <stdexcept>
#include <iostream>

csmntVkApplication::csmntVkApplication(int winW, int winH)
	: m_winH(winH), m_winW(winW), m_pWindow(nullptr) 
{
	//Add a validation layer
	m_validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
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
	}
}

void csmntVkApplication::shutdown()
{
	vkDestroyDevice(m_vkDevice, nullptr);

	if (m_enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
	}

	vkDestroyInstance(m_vkInstance, nullptr);

	glfwDestroyWindow(m_pWindow);

	glfwTerminate();
}

void csmntVkApplication::initVulkan()
{
	//Create instance and debug callbacks
	createVkInstance();
	setupDebugMessenger();

	//Pick a GFX card
	pickPhysicalDevice();

	//Create Logical device to interface with GFX card
	createLogicalDevice();
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

	//Check features here

	//Check for devices that can handle commands we want to use
	QueueFamilyIndices indices = findQueueFamilies(device);

	return indices.isComplete();
}

void csmntVkApplication::createLogicalDevice()
{
	//Find and describe a Queue family with GFX capabilities
	QueueFamilyIndices indices = findQueueFamilies(m_vkPhysicalDevice);

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;

	//Priority for scheduling command buffer execution
	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	//Features (queried before with vkGetPhysicalDeviceFeatures)
	//No features required just yet
	VkPhysicalDeviceFeatures deviceFeatures = {};

	//Create the logical device
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	//Features requested here...
	createInfo.pEnabledFeatures = &deviceFeatures;

	//Add validation layers to the device
	createInfo.enabledExtensionCount = 0;

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
}

QueueFamilyIndices csmntVkApplication::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

VKAPI_ATTR VkBool32 VKAPI_CALL csmntVkApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}
