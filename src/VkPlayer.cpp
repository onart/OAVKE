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
#include <algorithm>
#include <cstring>

#ifdef _MSC_VER
	#pragma comment(lib, "externals/vulkan/vulkan-1.lib")
	#pragma comment(lib, "externals/glfw/glfw3_mt.lib")
    #pragma comment(lib, "externals/shaderc/shaderc_shared.lib")
#endif

namespace onart {

	VkInstance VkPlayer::instance = nullptr;
	VkPlayer::PhysicalDevice VkPlayer::physicalDevice{};
	VkDevice VkPlayer::device = nullptr;
	VkQueue VkPlayer::graphicsQueue = nullptr, VkPlayer::presentQueue = nullptr;
	VkCommandPool VkPlayer::commandPool = nullptr;
	VkCommandBuffer VkPlayer::commandBuffers[VkPlayer::COMMANDBUFFER_COUNT] = {};
	GLFWwindow* VkPlayer::window = nullptr;
	uint32_t VkPlayer::width = 800, VkPlayer::height = 640;
	VkSurfaceKHR VkPlayer::surface = nullptr;
	VkSwapchainKHR VkPlayer::swapchain = nullptr;
	std::vector<VkImageView> VkPlayer::swapchainImageViews;
	VkFormat VkPlayer::swapchainImageFormat;

	int VkPlayer::frame = 1;
	float VkPlayer::dt = 1.0f / 60, VkPlayer::tp = 0, VkPlayer::idt = 60.0f;

	void VkPlayer::start() {
		if (init()) {
			mainLoop();
		}
		finalize();
	}

	void VkPlayer::exit() {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	bool VkPlayer::init() {
		return 
			createInstance()
			&& createWindow()
			&& findPhysicalDevice()
			&& createDevice()
			&& createCommandPool()
			&& createSwapchain()
			&& createSwapchainImageViews()
			;
	}

	void VkPlayer::mainLoop() {
		for (frame = 1; glfwWindowShouldClose(window) != GLFW_TRUE; frame++) {
			glfwPollEvents();
			static float prev = 0;
			tp = (float)glfwGetTime();
			dt = tp - prev;
			idt = 1.0f / dt;
			if ((frame & 15) == 0) printf("%f\r",idt);
			prev = tp;
			// loop body
		}
	}

	void VkPlayer::finalize() {
		destroySwapchainImageViews();
		destroySwapchain();
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
			if (!checkDeviceExtension(cards[i])) continue;
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
		PhysicalDevice ret;
		vkGetPhysicalDeviceQueueFamilyProperties(card, &qfcount, nullptr);
		std::vector<VkQueueFamilyProperties> qfs(qfcount);
		vkGetPhysicalDeviceQueueFamilyProperties(card, &qfcount, qfs.data());
		bool gq = false, pq = false;
		for (uint32_t i = 0; i < qfcount; i++) {
			VkBool32 supported;
			vkGetPhysicalDeviceSurfaceSupportKHR(card, i, surface, &supported);
			if (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { 
				if (supported) return { card,i,i };
				ret.graphicsFamily = i; gq = true;
			}
			if (supported) { ret.presentFamily = i; pq = true; }
		}
		if (gq && pq) {
			ret.card = card;
			return ret;
		}
		return {};
	}

	bool VkPlayer::createDevice() {
		VkDeviceQueueCreateInfo qInfo[2]{};
		float queuePriority = 1.0f;
		qInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qInfo[0].queueFamilyIndex = physicalDevice.graphicsFamily;
		qInfo[0].queueCount = 1;
		qInfo[0].pQueuePriorities = &queuePriority;

		qInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qInfo[1].queueFamilyIndex = physicalDevice.presentFamily;
		qInfo[1].queueCount = 1;
		qInfo[1].pQueuePriorities = &queuePriority;
		
		VkPhysicalDeviceFeatures features{};

		VkDeviceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		info.pQueueCreateInfos = qInfo;
		info.queueCreateInfoCount = 1 + physicalDevice.graphicsFamily != physicalDevice.presentFamily;
		info.pEnabledFeatures = &features;
		info.ppEnabledExtensionNames = DEVICE_EXT;
		info.enabledExtensionCount = DEVICE_EXT_COUNT;

		bool result = vkCreateDevice(physicalDevice.card, &info, nullptr, &device) == VK_SUCCESS;
		if (result) { 
			vkGetDeviceQueue(device, physicalDevice.graphicsFamily, 0, &graphicsQueue);
			vkGetDeviceQueue(device, physicalDevice.presentFamily, 0, &presentQueue);
		}
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

	bool VkPlayer::createSwapchain() {
		uint32_t count;
		VkSurfaceCapabilitiesKHR caps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.card, surface, &caps);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.card, surface, &count, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.card, surface, &count, formats.data());
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.card, surface, &count, nullptr);
		std::vector<VkPresentModeKHR> modes(count);

		VkSurfaceFormatKHR sf = formats[0];
		for (VkSurfaceFormatKHR& form : formats) {
			if (form.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && form.format == VK_FORMAT_B8G8R8A8_SRGB) sf = form;
		}

		uint32_t idx[2] = { physicalDevice.graphicsFamily,physicalDevice.presentFamily };

		VkSwapchainCreateInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.surface = surface;
		
		info.minImageCount = caps.minImageCount > caps.maxImageCount - 1 ? caps.minImageCount + 1 : caps.maxImageCount;
		info.imageFormat = swapchainImageFormat = sf.format;
		info.imageColorSpace = sf.colorSpace;
		info.imageExtent.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
		info.imageExtent.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
		info.presentMode = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.end() ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		info.preTransform = caps.currentTransform;
		info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.clipped = VK_TRUE;
		info.oldSwapchain = VK_NULL_HANDLE;
		if (physicalDevice.graphicsFamily == physicalDevice.presentFamily) {
			info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		else {
			info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			info.queueFamilyIndexCount = 2;
			info.pQueueFamilyIndices = &physicalDevice.graphicsFamily;
		}
		if (vkCreateSwapchainKHR(device, &info, nullptr, &swapchain) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create swapchain\n");
			return false;
		}
		return true;
	}

	void VkPlayer::destroySwapchain() {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
	}

	bool VkPlayer::checkDeviceExtension(VkPhysicalDevice device) {
		uint32_t count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> exts(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, exts.data());
		for (int i = 0; i < DEVICE_EXT_COUNT; i++) {
			bool flag = false;
			for (VkExtensionProperties& pr : exts) {
				if (strcmp(DEVICE_EXT[i], pr.extensionName) == 0) {
					flag = true;
					break;
				}
			}
			if (!flag)return false;
		}
		return true;
	}

	bool VkPlayer::createSwapchainImageViews() {
		uint32_t count;
		vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
		std::vector<VkImage> images(count);
		vkGetSwapchainImagesKHR(device, swapchain, &count, images.data());
		swapchainImageViews.resize(count);
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = swapchainImageFormat;
		info.components.r = info.components.g = info.components.b = info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		for (size_t i = 0; i < swapchainImageViews.size(); i++) {
			info.image = images[i];
			if (vkCreateImageView(device, &info, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
				fprintf(stderr,"Failed to create image views\n");
				return false;
			}
		}
		return true;
	}

	void VkPlayer::destroySwapchainImageViews() {
		for (int i = 0; i < swapchainImageViews.size(); i++) {
			vkDestroyImageView(device, swapchainImageViews[i], nullptr);
		}
	}
}