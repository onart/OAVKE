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

	void VkPlayer::start() {
		if (init()) {
			mainLoop();
		}
		finalize();
	}

	void VkPlayer::exit() {

	}

	bool VkPlayer::init() {
		return createInstance();
	}

	void VkPlayer::mainLoop() {

	}

	void VkPlayer::finalize() {
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
		if (vkCreateInstance(&info, nullptr, &instance) == VK_SUCCESS) return true;
		fprintf(stderr, "Failed to create Vulkan instance\n");
		return false;
	}

	void VkPlayer::destroyInstance() {		
		vkDestroyInstance(instance, nullptr);
	}
}