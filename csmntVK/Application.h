#pragma once
#ifndef _VK_CLASS_
#define _VK_CLASS_

//GLFW Windowing includes
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdlib>
//#include <optional>

#include "Graphics.h"

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
//--------------------

/////////////////////////////////////////////////////
//---csmntVkApplication:
//---Handles Vulkan instances, glfw window instances
/////////////////////////////////////////////////////

class csmntVkApplication {
public:
	csmntVkApplication(int, int);
	~csmntVkApplication();

	csmntVkApplication(csmntVkApplication&) = delete;
	csmntVkApplication& operator=(const csmntVkApplication&) = delete;

	void run();
	void mainLoop();
	void shutdown();

private:
	void initVulkan();
	void initWindow();
	
	bool checkValidationLayerSupport();
	void setupDebugMessenger();
	
	void createVkInstance();
	void createSurface();

	void initGraphicsModule();

	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice);
	
	void createLogicalDevice();

	std::vector<const char*> getRequiredExtensions();
	bool checkDeviceExtensionSupport(VkPhysicalDevice);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice&);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT*,
		void*);

	static void framebufferResizeCallback(GLFWwindow*, int, int);

	//Window Height & Width (800 x 600 default)
	int m_winH = 600, m_winW = 800;
	bool						m_frameBufferResized = false;

	GLFWwindow*					m_pWindow;
	VkInstance					m_vkInstance;
	VkDebugUtilsMessengerEXT	m_debugMessenger;
	VkPhysicalDevice			m_vkPhysicalDevice = VK_NULL_HANDLE;	//GFX card
	VkDevice					m_vkDevice;
	VkQueue						m_vkGraphicsQueue;
	VkQueue						m_vkPresentQueue;
	VkSurfaceKHR				m_vkSurface;

	//Graphics Module
	csmntVkGraphics*			m_pGraphics;

	//Validation Layers
	std::vector<const char*>	m_validationLayers;

	//Required device extensions
	std::vector<const char*>	m_deviceExtensions;

#ifdef NDEBUG
	const bool m_enableValidationLayers = false;
#else
	const bool m_enableValidationLayers = true;
#endif
};

#endif // !_VK_CLASS_
