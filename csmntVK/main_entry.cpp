//MAIN ENTRY POINT

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "../Libraries/glm/vec4.hpp"
#include "../Libraries/glm/mat4x4.hpp"

#include <iostream>

#include "Application.h"

int main() {
	//Create the application
	csmntVkApplication* p_application = new csmntVkApplication(800, 600);

	//Run the application
	try {
		p_application->run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	//cleanup
	if (p_application)
	{
		delete p_application;
		p_application = nullptr;
	}

	return EXIT_SUCCESS;
}