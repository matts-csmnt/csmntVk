#pragma once
#ifndef _VK_DETAILS_STRUCTS_
#define _VK_DETAILS_STRUCTS_

//Swap chain details
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

#endif