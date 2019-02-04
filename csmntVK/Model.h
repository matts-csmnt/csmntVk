#pragma once
#ifndef _MODEL_CLASS_
#define _MODEL_CLASS_

#include <vector>
#include <array>

#include <vulkan/vulkan.h>
#include "../Libraries/glm/glm.hpp"

struct Vertex {
  glm::vec2 pos;
  glm::vec3 colour;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
  }
  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
	//Position
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);
	//Colour
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, colour);
    return attributeDescriptions;
  }
};

class Model {
 public:
  const std::vector<Vertex>& getVertices() { return vertices; };
 
 private:
  const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};
};

#endif