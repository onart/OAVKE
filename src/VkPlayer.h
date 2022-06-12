/*
MIT License
Copyright (c) 2022 onart
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef __VK_PLAYER_H__
#define __VK_PLAYER_H__

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include "externals/glfw/glfw3.h"

namespace onart {
	class VkPlayer {
	public:
		// Vulkan을 초기화하고, 시작합니다.
		static void start();
		// 모든 것을 정상적으로 리턴하고 종료합니다. 창을 닫은 것과 같은 동작입니다.
		static void exit();
	private:	// 멤버
		static VkInstance instance;					// 인스턴스
		static struct PhysicalDevice {
			VkPhysicalDevice card;
			uint32_t graphicsFamily;
			uint32_t presentFamily;
		}physicalDevice;							// 물리 장치
		static VkDevice device;						// 가상 장치
		static VkQueue graphicsQueue, presentQueue;	// 큐
		static VkCommandPool commandPool;			// 명령풀
		static VkCommandBuffer commandBuffers[];	// 명령 버퍼
		static GLFWwindow* window;					// 응용 창
		static uint32_t width, height;				// 창 크기
		static VkSurfaceKHR surface;				// 창 표면
		static VkSwapchainKHR swapchain;			// 스왑 체인

		static int frame;							// 프레임 번호(1부터 시작)
		static float dt, tp, idt;					// 현재 프레임과 이전 프레임 사이의 간격(초) / 프레임 시작 시점(초) / dt의 역수
	private:	// 함수
		// GLFW, Vulkan을 비롯하여 필요한 모든 것을 초기화합니다.
		static bool init();
		// 업데이트, 렌더링 명령을 수행합니다.
		static void mainLoop();
		// 종료 전 할당한 모든 자원을 해제합니다.
		static void finalize();
		// vulkan 인스턴스를 생성합니다.
		static bool createInstance();
		// vulkan 인스턴스를 해제합니다.
		static void destroyInstance();
		// 가용 물리 장치를 찾습니다.
		static bool findPhysicalDevice();
		// 주어진 물리 장치에 대하여 그래픽스, 전송 큐가 가능하다면 함께 묶어 리턴합니다.
		static PhysicalDevice setQueueFamily(VkPhysicalDevice card);
		// 가상(logical) 장치를 생성합니다.
		static bool createDevice();
		// 가상 장치를 해제합니다.
		static void destroyDevice();
		// 명령 풀과 버퍼를 생성합니다.
		static bool createCommandPool();
		// 명령 풀과 버퍼를 제거합니다.
		static void destroyCommandPool();
		// 응용 창에 벌칸을 통합합니다.
		static bool createWindow();
		// 창을 제거합니다.
		static void destroyWindow();
		// 인스턴스에 대하여 활성화할 확장 이름들을 가져옵니다.
		static std::vector<std::string> getNeededInstanceExtensions();
		// 스왑 체인을 생성합니다.
		static bool createSwapchain();
		// 스왑 체인을 해제합니다.
		static void destroySwapchain();
		// 물리 장치의 원하는 확장 지원 여부를 확인합니다.
		static bool checkDeviceExtension(VkPhysicalDevice);
		
	private:	// 상수
		constexpr static bool USE_VALIDATION_LAYER = true;
		constexpr static const char* VALIDATION_LAYERS[] = { "VK_LAYER_KHRONOS_validation" };
		constexpr static int VALIDATION_LAYER_COUNT = sizeof(VALIDATION_LAYERS) / sizeof(const char*);
		constexpr static int COMMANDBUFFER_COUNT = 4;
		constexpr static const char* DEVICE_EXT[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		constexpr static int DEVICE_EXT_COUNT = sizeof(DEVICE_EXT) / sizeof(const char*);
	};
}

#endif // !__VK_PLAYER_H__

