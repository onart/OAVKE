/***********************************************************************
MIT License
Copyright (c) 2022 onart
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*************************************************************************/
#include "VkPlayer.h"
#include "externals/shaderc/shaderc.hpp"
#include <thread>

#ifdef _MSC_VER
	#pragma comment(lib, "externals/vulkan/vulkan-1.lib")
	#pragma comment(lib, "externals/glfw/glfw3_mt.lib")
    #pragma comment(lib, "externals/shaderc/shaderc_shared.lib")
#endif

namespace onart {

	VkInstance VkPlayer::instance = nullptr;
	VkPlayer::PhysicalDevice VkPlayer::physicalDevice{};
	VkDevice VkPlayer::device = nullptr;
	VkQueue VkPlayer::graphicsQueue = nullptr;
	VkCommandPool VkPlayer::commandPool = nullptr;
	VkCommandBuffer VkPlayer::commandBuffers[VkPlayer::COMMANDBUFFER_COUNT] = {};
	GLFWwindow* VkPlayer::window = nullptr;
	uint16_t VkPlayer::width = 800, VkPlayer::height = 640;
	VkSurfaceKHR VkPlayer::surface = nullptr;
	int VkPlayer::frame = 1;
	float VkPlayer::dt = 1.0f / 60, VkPlayer::tp = 0, VkPlayer::idt = 60.0f;

	void VkPlayer::start() {
		if (init()) {
			mainLoop();
		}
		finalize();
	}

	void VkPlayer::exit() {
		
	}

	bool VkPlayer::init() {
		return 
			createInstance()
			&& createWindow()
			&& findPhysicalDevice()
			&& createDevice()
			&& createCommandPool();
	}

	void VkPlayer::mainLoop() {
		for (frame = 1; glfwWindowShouldClose(window) != GLFW_TRUE; frame++) {
			glfwPollEvents();
			static float prev = 0;
			tp = (float)glfwGetTime();
			dt = tp - prev;
			idt = 1.0f / dt;
			if ((frame & 15) == 0) printf("%f\r",idt);

			//std::this_thread::sleep_until()
			prev = tp;
			// loop body
		}
	}

	void VkPlayer::finalize() {
		destroyCommandPool();
		destroyDevice();
		destroyWindow();
		destroyInstance();
	}

	bool VkPlayer::createInstance() {
		VkApplicationInfo ainfo{};
		ainfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;	// 고정값
		ainfo.apiVersion = VK_API_VERSION_1_0;	//VK_HEADER_VERSION_COMPLETE;

		ainfo.pApplicationName = "OAVKE";
		ainfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
		ainfo.pEngineName = "OAVKE";
		ainfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);

		VkInstanceCreateInfo info{};	// 일단 모두 0으로 초기화함
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	// 고정값
		info.pApplicationInfo = &ainfo;
		
		std::vector<std::string> names = getNeededInstanceExtensions();
		std::vector<const char*> pnames(names.size());
		for (size_t i = 0; i < pnames.size(); i++) { pnames[i] = names[i].c_str(); }
		info.enabledExtensionCount = (uint32_t)pnames.size();
		info.ppEnabledExtensionNames = pnames.data();

		if constexpr (USE_VALIDATION_LAYER) {
			info.enabledLayerCount = VALIDATION_LAYER_COUNT;
			info.ppEnabledLayerNames = VALIDATION_LAYERS;
		}

		if (vkCreateInstance(&info, nullptr, &instance) == VK_SUCCESS) return true;
		fprintf(stderr, "Failed to create Vulkan instance\n");
		return false;
	}

	void VkPlayer::destroyInstance() {		
		vkDestroyInstance(instance, nullptr);
	}

	bool VkPlayer::findPhysicalDevice() {
		uint32_t count;
		vkEnumeratePhysicalDevices(instance, &count, nullptr);
		std::vector<VkPhysicalDevice> cards(count);
		vkEnumeratePhysicalDevices(instance, &count, cards.data());
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		for (uint32_t i = 0; i < count; i++) {
			vkGetPhysicalDeviceProperties(cards[i], &properties);
			vkGetPhysicalDeviceFeatures(cards[i], &features);
			PhysicalDevice pd = setQueueFamily(cards[i]);
			if (pd.card) { 
				physicalDevice = pd;
				return true;
			}
		}
		fprintf(stderr, "Couldn't find adequate graphics device\n");
		return false;
	}

	
	VkPlayer::PhysicalDevice VkPlayer::setQueueFamily(VkPhysicalDevice card) {
		uint32_t qfcount;
		vkGetPhysicalDeviceQueueFamilyProperties(card, &qfcount, nullptr);
		std::vector<VkQueueFamilyProperties> qfs(qfcount);
		vkGetPhysicalDeviceQueueFamilyProperties(card, &qfcount, qfs.data());
		for (uint32_t i = 0; i < qfcount; i++) {
			if (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { return { card,i }; }
		}
		return {};
	}

	bool VkPlayer::createDevice() {
		VkDeviceQueueCreateInfo qInfo[1]{};
		float queuePriority = 1.0f;
		qInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qInfo[0].queueFamilyIndex = physicalDevice.graphicsFamily;
		qInfo[0].queueCount = 1;
		qInfo[0].pQueuePriorities = &queuePriority;
		
		VkPhysicalDeviceFeatures features{};

		VkDeviceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		info.pQueueCreateInfos = qInfo;
		info.queueCreateInfoCount = sizeof(qInfo) / sizeof(VkDeviceQueueCreateInfo);
		info.pEnabledFeatures = &features;

		bool result = vkCreateDevice(physicalDevice.card, &info, nullptr, &device) == VK_SUCCESS;
		if (result) { vkGetDeviceQueue(device, physicalDevice.graphicsFamily, 0, &graphicsQueue); }
		else { fprintf(stderr, "Failed to create logical device\n"); }
		return result;
	}

	void VkPlayer::destroyDevice() {
		vkDestroyDevice(device, nullptr);
	}

	bool VkPlayer::createCommandPool() {
		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = physicalDevice.graphicsFamily;
		
		if (vkCreateCommandPool(device, &info, nullptr, &commandPool) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create graphics/transfer command pool\n");
			return false;
		}
		VkCommandBufferAllocateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferInfo.commandPool = commandPool;
		bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferInfo.commandBufferCount = COMMANDBUFFER_COUNT;
		if (vkAllocateCommandBuffers(device, &bufferInfo, commandBuffers) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create graphics/transfer command buffers\n");
			return false;
		}
		return true;
	}
	
	void VkPlayer::destroyCommandPool() {
		vkFreeCommandBuffers(device, commandPool, COMMANDBUFFER_COUNT, commandBuffers);
		vkDestroyCommandPool(device, commandPool, nullptr);
	}

	bool VkPlayer::createWindow() {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(width, height, u8"OAVKE", nullptr, nullptr);
		if (!window) {
			fprintf(stderr, "Failed to create window\n");
			return false;
		}
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create window surface\n");
			return false;
		}
		return true;
	}

	void VkPlayer::destroyWindow() {
		vkDestroySurfaceKHR(instance, surface, nullptr);
		glfwDestroyWindow(window);
	}

	std::vector<std::string> VkPlayer::getNeededInstanceExtensions() {
		if (glfwInit() != GLFW_TRUE) {
			fprintf(stderr, "Failed to initialize GLFW\n");
			return {};
		}
		if (glfwVulkanSupported() != GLFW_TRUE) {
			fprintf(stderr, "Vulkan not supported with GLFW\n");
			return {};
		}
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		idt = (float)mode->refreshRate;
		dt = 1.0f / idt;
		uint32_t count;
		const char** names = glfwGetRequiredInstanceExtensions(&count);
		std::vector<std::string> strs(count);
		for (uint32_t i = 0; i < count; i++) {
			strs[i] = names[i];
		}
		return strs;
	}
}