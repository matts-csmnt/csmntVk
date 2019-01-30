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
	void createPipeline(VkDevice*, VkExtent2D&);

private:
	VkPipelineLayout m_vkPipelineLayout;

	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice*);
};

#endif