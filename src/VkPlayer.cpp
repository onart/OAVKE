/***********************************************************************
MIT License
Copyright (c) 2022 onart
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*************************************************************************/
#include "VkPlayer.h"
#include "externals/shaderc/shaderc.hpp"

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
	VkQueue VkPlayer::transferQueue = nullptr;

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
			&& findPhysicalDevice()
			&& createDevice();
	}

	void VkPlayer::mainLoop() {

	}

	void VkPlayer::finalize() {
		destroyDevice();
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
		PhysicalDevice ret{};
		uint32_t qfcount;
		vkGetPhysicalDeviceQueueFamilyProperties(card, &qfcount, nullptr);
		std::vector<VkQueueFamilyProperties> qfs(qfcount);
		vkGetPhysicalDeviceQueueFamilyProperties(card, &qfcount, qfs.data());
		for (uint32_t i = 0; i < qfcount; i++) {
			if (qfs[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)) { return { card,i,i }; }
			if (!ret.graphicsFamily.has_value() && (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) { ret.graphicsFamily = i; }
			if (!ret.transferFamily.has_value() && (qfs[i].queueFlags & VK_QUEUE_TRANSFER_BIT)) { ret.transferFamily = i; }
		}
		if (!ret.graphicsFamily.has_value() && !ret.transferFamily.has_value()) { return {}; }
		ret.card = card;
		return ret;
	}

	bool VkPlayer::createDevice() {
		VkDeviceQueueCreateInfo qInfo[2]{};
		float queuePriority = 1.0f;
		qInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qInfo[0].queueFamilyIndex = physicalDevice.graphicsFamily.value();
		qInfo[0].queueCount = 1;
		qInfo[0].pQueuePriorities = &queuePriority;
		qInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qInfo[1].queueFamilyIndex = physicalDevice.transferFamily.value();
		qInfo[1].queueCount = 1;
		qInfo[1].pQueuePriorities = &queuePriority;
		
		VkPhysicalDeviceFeatures features{};

		VkDeviceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		info.pQueueCreateInfos = qInfo;
		info.queueCreateInfoCount = (physicalDevice.graphicsFamily.value() == physicalDevice.transferFamily.value()) ? 1 : 2;
		info.pEnabledFeatures = &features;

		bool result = vkCreateDevice(physicalDevice.card, &info, nullptr, &device) == VK_SUCCESS;
		if (result) {
			vkGetDeviceQueue(device, physicalDevice.graphicsFamily.value(), 0, &graphicsQueue);
			vkGetDeviceQueue(device, physicalDevice.transferFamily.value(), 0, &transferQueue);
		}
		return result;
	}

	void VkPlayer::destroyDevice() {
		vkDestroyDevice(device, nullptr);
	}
}