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

#include "externals/shaderc/shaderc.hpp"

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
		static int commandBufferNumber;				// 지금 쓸 명령 버퍼 번호
		static GLFWwindow* window;					// 응용 창
		static uint32_t width, height;				// 창 크기
		static VkSurfaceKHR surface;				// 창 표면
		static VkSwapchainKHR swapchain;			// 스왑 체인
		static VkExtent2D swapchainExtent;			// 실제 만들어진 스왑체인 이미지 크기
		static std::vector<VkImageView> swapchainImageViews;	// 스왑 체인 이미지 뷰
		static VkFormat swapchainImageFormat;		// 스왑 체인 이미지의 형식
		static VkRenderPass renderPass0;			// 단순 렌더패스
		static std::vector<VkFramebuffer> endFramebuffers;	// 스왑체인 이미지를 참조하는 프레임버퍼
		static VkPipeline pipeline0;				// 단순 렌더링(변환 - 텍스처,라이팅 하고 끝) 파이프라인
		static VkPipelineLayout pipelineLayout0;	// 0번의 레이아웃
		static VkSemaphore fixedSp[], presentSp[];	// 고정 세마포어
		static VkFence bufferFence[];				// 버퍼 펜스
		static VkImage dsImage;						// 깊이/스텐실 이미지
		static VkImageView dsImageView;				// dsImage의 뷰
		static VkDeviceMemory dsmem;				// dsImage를 위한 메모리
		static VkFormat dsImageFormat;

		static VkBuffer ub[];						// 공유 버퍼
		static VkDeviceMemory ubmem[];				// 공유 버퍼 메모리 핸들
		static void* ubmap[];						// 공유 버퍼 메모리 맵 공유 지점
		static VkDescriptorSetLayout ubds;			// 
		static VkDescriptorPool ubpool;				// 디스크립터 풀
		static VkDescriptorSet ubset[];				// 디스크립터 집합

		static VkBuffer vb, ib;						// 일시적인 고정 정점 버퍼
		static VkDeviceMemory vbmem, ibmem;			// vb 메모리 핸들

		static VkImage tex0;
		static VkDeviceMemory texmem0;
		static VkImageView texview0;
		static VkSampler sampler0;

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
		// 스왑 체인에서 이미지 뷰를 생성합니다.
		static bool createSwapchainImageViews();
		// 스왑 체인의 이미지 뷰를 해제합니다.
		static void destroySwapchainImageViews();
		// 렌더패스를 생성합니다.
		static bool createRenderPasses();
		// 렌더패스를 해제합니다.
		static void destroyRenderPasses();
		// 단순 렌더링을 위한 패스를 생성합니다.
		static bool createRenderPass0();
		// 단순 렌더링을 위한 패스를 제거합니다.
		static void destroyRenderPass0();
		// 필요한 프레임버퍼를 생성합니다.
		static bool createFramebuffers();
		// 프레임버퍼를 해제합니다.
		static void destroyFramebuffers();
		// 최종 단계(스왑 체인에 연결된)의 프레임버퍼를 생성합니다.
		static bool createEndFramebuffers();
		// 최종 단계(스왑 체인에 연결된)의 프레임버퍼를 해제합니다.
		static void destroyEndFramebuffers();
		// 필요한 파이프라인을 생성합니다.
		static bool createPipelines();
		// 파이프라인을 해제합니다.
		static void destroyPipelines();
		// 단순 파이프라인을 생성합니다.
		static bool createPipeline0();
		// 단순 파이프라인을 해제합니다.
		static void destroyPipeline0();
		// SPIR-V 코드를 받아 셰이더 모듈을 생성합니다.
		static VkShaderModule createShaderModuleFromSpv(const std::vector<uint32_t>& bcode);
		// SPIR-V 코드를 파일에서 읽어 셰이더 모듈을 생성합니다.
		static VkShaderModule createShaderModule(const char* fileName);
		// GLSL 코드를 파일에서 읽어 셰이더 모듈을 생성합니다.
		static VkShaderModule createShaderModule(const char* fileName, shaderc_shader_kind kind);
		// 메모리 상의 변수로부터 GLSL 코드를 읽어 모듈을 생성합니다.
		static VkShaderModule createShaderModule(const char* code, size_t size, shaderc_shader_kind kind, const char* name);
		// 삼각형 2개를 그리기 위한 정점 버퍼를 생성합니다. 당연하지만 이후 지우고, Mesh 같은 이름의 클래스로 일반화하겠죠.
		static bool createFixedVertexBuffer();
		// 삼각형 2개를 그리기 위한 정점 버퍼를 해제합니다.
		static void destroyFixedVertexBuffer();
		// 고정 정점을 가지고 한 번 그려 봅니다.
		static void fixedDraw();
		// 필요한 세마포어를 생성합니다.
		static bool createSemaphore();
		// 만든 세마포어를 해제합니다.
		static void destroySemaphore();
		// 공유 버퍼를 생성합니다.
		static bool createUniformBuffer();
		// 공유 버퍼를 해제합니다.
		static void destroyUniformBuffer();
		// 디스크립터 집합을 생성합니다.
		static bool createDescriptorSet();
		// 디스크립터 집합을 해제합니다.
		static void destroyDescriptorSet();
		// 직사각형 하나를 그리기 위한 고정 인덱스 버퍼를 생성합니다.
		static bool createFixedIndexBuffer();
		// 직사각형 하나를 그리기 위한 고정 인덱스 버퍼를 해제합니다.
		static void destroyFixedIndexBuffer();
		// 깊이/스텐실 버퍼를 위한 이미지와 이미지 뷰를 생성합니다.
		static bool createDSBuffer();
		// 깊이/스텐실 버퍼를 위한 이미지와 이미지 뷰를 해제합니다.
		static void destroyDSBuffer();
		// 고정된 텍스처 이미지를 하나 만들어 봅니다.
		static bool createTex0();
		// 고정된 텍스처 이미지를 해제합니다.
		static void destroyTex0();
		// tex0 샘플러를 생성합니다.
		static bool createSampler0();
		// tex0 샘플러를 제거합니다.
		static void destroySampler0();
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

