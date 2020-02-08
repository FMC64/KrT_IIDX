#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>

namespace Vk {

class Context
{
public:
	Context(int32_t w, int32_t h, bool doProfile);
	~Context(void);

	int32_t w;
	int32_t h;

	GLFWwindow *window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	int shouldClose(void);

private:
	GLFWwindow* createWindow(void);
	VkInstance createInstance(bool doProfile);
	VkDebugUtilsMessengerEXT createDebugMessenger(void);
	VkSurfaceKHR createSurface(void);
};

}