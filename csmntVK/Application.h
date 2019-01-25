#pragma once
#ifndef _VK_CLASS_
#define _VK_CLASS_

//GLFW Windowing includes
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdlib>
#include <optional>

//vkCreateDebugUtilsMessengerEXT function to create the VkDebugUtilsMessengerEXT object. 
//Unfortunately, because this function is an extension function, it is not automatically loaded. 
//We have to look up its address ourselves using vkGetInstanceProcAddr (https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers)
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
};
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
};

//Queue Family Checker
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	bool isComplete() {
		return graphicsFamily.has_value();
	}
};

/////////////////////////////////////////////////////
//---csmntVkApplication:
//---Handles Vulkan instances, glfw window instances
/////////////////////////////////////////////////////

class csmntVkApplication {
public:
	csmntVkApplication(int, int);

	void run();
	void mainLoop();
	void shutdown();

private:
	void initVulkan();
	void initWindow();
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
	void setupDebugMessenger();
	void createVkInstance();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	//Window Height & Width (800 x 600 default)
	int m_winH = 600, m_winW = 800;

	GLFWwindow* m_pWindow;
	VkInstance	m_vkInstance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;	//GFX card

	//Validation Layers
	std::vector<const char*> m_validationLayers;

#ifdef NDEBUG
	const bool m_enableValidationLayers = false;
#else
	const bool m_enableValidationLayers = true;
#endif
};

#endif // !_VK_CLASS_
