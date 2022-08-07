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
#include "externals/vk_mem_alloc.h"
#include "externals/shaderc/shaderc.hpp"

namespace onart {
	class VkPlayer {
	public:
		// Vulkan�� �ʱ�ȭ�ϰ�, �����մϴ�.
		static void start();
		// ��� ���� ���������� �����ϰ� �����մϴ�. â�� ���� �Ͱ� ���� �����Դϴ�.
		static void exit();
	private:	// ���
		static VkInstance instance;					// �ν��Ͻ�
		static struct PhysicalDevice {
			VkPhysicalDevice card;
			uint32_t graphicsFamily;
			uint32_t presentFamily;
		}physicalDevice;							// ���� ��ġ
		static VkDevice device;						// ���� ��ġ
		static VkQueue graphicsQueue, presentQueue;	// ť
		static VkCommandPool commandPool;			// ����Ǯ
		static VkCommandBuffer commandBuffers[];	// ���� ����
		static int commandBufferNumber;				// ���� �� ���� ���� ��ȣ
		static GLFWwindow* window;					// ���� â
		static uint32_t width, height;				// â ũ��
		static VkSurfaceKHR surface;				// â ǥ��
		static VkSwapchainKHR swapchain;			// ���� ü��
		static VkExtent2D swapchainExtent;			// ���� ������� ����ü�� �̹��� ũ��
		static std::vector<VkImageView> swapchainImageViews;	// ���� ü�� �̹��� ��
		static VkFormat swapchainImageFormat;		// ���� ü�� �̹����� ����
		static VkRenderPass renderPass0;			// �ܼ� �����н�
		static std::vector<VkFramebuffer> endFramebuffers;	// ����ü�� �̹����� �����ϴ� �����ӹ���
		static VkPipeline pipeline0;				// �ܼ� ������(��ȯ - �ؽ�ó,������ �ϰ� ��) ����������
		static VkPipelineLayout pipelineLayout0;	// 0���� ���̾ƿ�
		static VkSemaphore fixedSp[], presentSp[];	// ���� ��������
		static VkFence bufferFence[];				// ���� �潺
		static VkImage dsImage;						// ����/���ٽ� �̹���
		static VkImageView dsImageView;				// dsImage�� ��
		static VkDeviceMemory dsmem;				// dsImage�� ���� �޸�
		static VkFormat dsImageFormat;

		static VkImage middleImage;
		static VkDeviceMemory middleMem;
		static VkImageView middleImageView;

		static VkPipeline pipeline1;
		static VkPipelineLayout pipelineLayout1;
		static VkDescriptorSetLayout sp1layout;
		static VkDescriptorPool sp1pool;
		static VkDescriptorSet sp1set;

		static VkBuffer ub[];						// ���� ����
		static VkDeviceMemory ubmem[];				// ���� ���� �޸� �ڵ�
		static void* ubmap[];						// ���� ���� �޸� �� ���� ����
		static VkDescriptorSetLayout ubds;			// 
		static VkDescriptorPool ubpool;				// ��ũ���� Ǯ
		static VkDescriptorSet ubset[];				// ��ũ���� ����

		static VkBuffer vb, ib;						// �Ͻ����� ���� ���� ����
		static VkDeviceMemory vbmem, ibmem;			// vb �޸� �ڵ�

		static VkImage tex0, tex1;
		static VkDeviceMemory texmem0, texmem1;
		static VkImageView texview0, texview1;
		static VkSampler sampler0;
		static VkDescriptorSet samplerSet[];

		enum class OptionalEXT { ANISOTROPIC, OPTIONAL_EXT_MAX_ENUM };
		static bool extSupported[];

		static VmaAllocator allocator;

		static int frame;							// ������ ��ȣ(1���� ����)
		static float dt, tp, idt;					// ���� �����Ӱ� ���� ������ ������ ����(��) / ������ ���� ����(��) / dt�� ����
		static bool resizing, shouldRecreateSwapchain;
		static VkDeviceSize minUniformBufferOffset;
	private:	// �Լ�
		// GLFW, Vulkan�� ����Ͽ� �ʿ��� ��� ���� �ʱ�ȭ�մϴ�.
		static bool init();
		// ������Ʈ, ������ ������ �����մϴ�.
		static void mainLoop();
		// ���� �� �Ҵ��� ��� �ڿ��� �����մϴ�.
		static void finalize();
		// vulkan �ν��Ͻ��� �����մϴ�.
		static bool createInstance();
		// vulkan �ν��Ͻ��� �����մϴ�.
		static void destroyInstance();
		// ���� ���� ��ġ�� ã���ϴ�.
		static bool findPhysicalDevice();
		// �־��� ���� ��ġ�� ���Ͽ� �׷��Ƚ�, ���� ť�� �����ϴٸ� �Բ� ���� �����մϴ�.
		static PhysicalDevice setQueueFamily(VkPhysicalDevice card);
		// ����(logical) ��ġ�� �����մϴ�.
		static bool createDevice();
		// ���� ��ġ�� �����մϴ�.
		static void destroyDevice();
		// ���� Ǯ�� ���۸� �����մϴ�.
		static bool createCommandPool();
		// ���� Ǯ�� ���۸� �����մϴ�.
		static void destroyCommandPool();
		// ���� â�� ��ĭ�� �����մϴ�.
		static bool createWindow();
		// â�� �����մϴ�.
		static void destroyWindow();
		// �ν��Ͻ��� ���Ͽ� Ȱ��ȭ�� Ȯ�� �̸����� �����ɴϴ�.
		static std::vector<std::string> getNeededInstanceExtensions();
		// ���� ü���� �����մϴ�.
		static bool createSwapchain();
		// ���� ü���� �����մϴ�.
		static void destroySwapchain();
		// ���� ��ġ�� ���ϴ� Ȯ�� ���� ���θ� Ȯ���մϴ�.
		static bool checkDeviceExtension(VkPhysicalDevice);
		// ���� ü�ο��� �̹��� �並 �����մϴ�.
		static bool createSwapchainImageViews();
		// ���� ü���� �̹��� �並 �����մϴ�.
		static void destroySwapchainImageViews();
		// �����н��� �����մϴ�.
		static bool createRenderPasses();
		// �����н��� �����մϴ�.
		static void destroyRenderPasses();
		// �ܼ� �������� ���� �н��� �����մϴ�.
		static bool createRenderPass0();
		// �ܼ� �������� ���� �н��� �����մϴ�.
		static void destroyRenderPass0();
		// �ʿ��� �����ӹ��۸� �����մϴ�.
		static bool createFramebuffers();
		// �����ӹ��۸� �����մϴ�.
		static void destroyFramebuffers();
		// ���� �ܰ�(���� ü�ο� �����)�� �����ӹ��۸� �����մϴ�.
		static bool createEndFramebuffers();
		// ���� �ܰ�(���� ü�ο� �����)�� �����ӹ��۸� �����մϴ�.
		static void destroyEndFramebuffers();
		// �ʿ��� ������������ �����մϴ�.
		static bool createPipelines();
		// ������������ �����մϴ�.
		static void destroyPipelines();
		// �ܼ� ������������ �����մϴ�.
		static bool createPipeline0();
		// �ܼ� ������������ �����մϴ�.
		static void destroyPipeline0();
		// SPIR-V �ڵ带 �޾� ���̴� ����� �����մϴ�.
		static VkShaderModule createShaderModuleFromSpv(const std::vector<uint32_t>& bcode);
		// SPIR-V �ڵ带 ���Ͽ��� �о� ���̴� ����� �����մϴ�.
		static VkShaderModule createShaderModule(const char* fileName);
		// GLSL �ڵ带 ���Ͽ��� �о� ���̴� ����� �����մϴ�.
		static VkShaderModule createShaderModule(const char* fileName, shaderc_shader_kind kind);
		// �޸� ���� �����κ��� GLSL �ڵ带 �о� ����� �����մϴ�.
		static VkShaderModule createShaderModule(const char* code, size_t size, shaderc_shader_kind kind, const char* name);
		// �ﰢ�� 2���� �׸��� ���� ���� ���۸� �����մϴ�. �翬������ ���� �����, Mesh ���� �̸��� Ŭ������ �Ϲ�ȭ�ϰ���.
		static bool createFixedVertexBuffer();
		// �ﰢ�� 2���� �׸��� ���� ���� ���۸� �����մϴ�.
		static void destroyFixedVertexBuffer();
		// ���� ������ ������ �� �� �׷� ���ϴ�.
		static void fixedDraw();
		// �ʿ��� ������� �����մϴ�.
		static bool createSemaphore();
		// ���� ������� �����մϴ�.
		static void destroySemaphore();
		// ���� ���۸� �����մϴ�.
		static bool createUniformBuffer();
		// ���� ���۸� �����մϴ�.
		static void destroyUniformBuffer();
		// ��ũ���� ������ �����մϴ�.
		static bool createDescriptorSet();
		// ��ũ���� ������ �����մϴ�.
		static void destroyDescriptorSet();
		// ���簢�� �ϳ��� �׸��� ���� ���� �ε��� ���۸� �����մϴ�.
		static bool createFixedIndexBuffer();
		// ���簢�� �ϳ��� �׸��� ���� ���� �ε��� ���۸� �����մϴ�.
		static void destroyFixedIndexBuffer();
		// ����/���ٽ� ���۸� ���� �̹����� �̹��� �並 �����մϴ�.
		static bool createDSBuffer();
		// ����/���ٽ� ���۸� ���� �̹����� �̹��� �並 �����մϴ�.
		static void destroyDSBuffer();
		// ������ �ؽ�ó �̹����� �ϳ� ����� ���ϴ�.
		static bool createTex0();
		// ������ �ؽ�ó �̹����� �����մϴ�.
		static void destroyTex0();
		// tex0 ���÷��� �����մϴ�.
		static bool createSampler0();
		// tex0 ���÷��� �����մϴ�.
		static void destroySampler0();
		// Ȯ�� ���� ���θ� Ȯ���մϴ�.
		inline static bool hasExt(OptionalEXT ext) { return extSupported[(size_t)ext]; }
		// ��ó�� ������������ �����մϴ�.
		static bool createPipeline1();
		// ��ó�� ������������ �����մϴ�.
		static void destroyPipeline1();
		// glfw �����ӹ��� �������� �ݹ��Դϴ�.
		static void onResize(GLFWwindow* window, int width, int height);
		// VMA �Ҵ�⸦ �ʱ�ȭ�մϴ�.
		static bool initAllocator();
		// VMA �Ҵ�� ����� �����մϴ�.
		static void destroyAllocator();
	private:	// ���
		constexpr static bool USE_VALIDATION_LAYER = true;
		constexpr static const char* VALIDATION_LAYERS[] = { "VK_LAYER_KHRONOS_validation" };
		constexpr static int VALIDATION_LAYER_COUNT = sizeof(VALIDATION_LAYERS) / sizeof(const char*);
		constexpr static int COMMANDBUFFER_COUNT = 4;
		constexpr static const char* DEVICE_EXT[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		constexpr static int DEVICE_EXT_COUNT = sizeof(DEVICE_EXT) / sizeof(const char*);
	};
}

#endif // !__VK_PLAYER_H__
