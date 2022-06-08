#define GLFW_INCLUDE_VULKAN
#include "externals/glfw/glfw3.h"
#include "externals/shaderc/shaderc.hpp"
#include <cstdio>
#include <filesystem>

#ifdef _MSC_VER
	#pragma comment(lib, "externals/vulkan/vulkan-1.lib")
	#pragma comment(lib, "externals/glfw/glfw3_mt.lib")
#endif

int main(int argc, char* argv[]) {
	std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, u8"Ω√¿€", nullptr, nullptr);
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); printf("%d ext available\n", extensionCount);
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	} glfwDestroyWindow(window); glfwTerminate();
	return 0;
}
