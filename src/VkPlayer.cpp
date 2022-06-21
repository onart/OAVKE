/***********************************************************************
MIT License
Copyright (c) 2022 onart
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*************************************************************************/
#include "VkPlayer.h"
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
	int VkPlayer::commandBufferNumber = 0;
	GLFWwindow* VkPlayer::window = nullptr;
	uint32_t VkPlayer::width = 800, VkPlayer::height = 640;
	VkExtent2D VkPlayer::swapchainExtent;
	VkSurfaceKHR VkPlayer::surface = nullptr;
	VkSwapchainKHR VkPlayer::swapchain = nullptr;
	std::vector<VkImageView> VkPlayer::swapchainImageViews;
	VkFormat VkPlayer::swapchainImageFormat;
	VkRenderPass VkPlayer::renderPass0 = nullptr;
	std::vector<VkFramebuffer> VkPlayer::endFramebuffers;
	VkPipeline VkPlayer::pipeline0 = nullptr;
	VkPipelineLayout VkPlayer::pipelineLayout0 = nullptr;
	VkSemaphore VkPlayer::fixedSp[VkPlayer::COMMANDBUFFER_COUNT] = {};
	VkSemaphore VkPlayer::presentSp[VkPlayer::COMMANDBUFFER_COUNT] = {};
	VkFence VkPlayer::bufferFence[VkPlayer::COMMANDBUFFER_COUNT] = {};

	VkBuffer VkPlayer::ub[VkPlayer::COMMANDBUFFER_COUNT] = {};
	VkDeviceMemory VkPlayer::ubmem[VkPlayer::COMMANDBUFFER_COUNT] = {};
	void* VkPlayer::ubmap[VkPlayer::COMMANDBUFFER_COUNT] = {};
	VkDescriptorSetLayout VkPlayer::ubds = nullptr;
	VkDescriptorPool VkPlayer::ubpool = nullptr;
	VkDescriptorSet VkPlayer::ubset[VkPlayer::COMMANDBUFFER_COUNT] = {};

	VkBuffer VkPlayer::vb = nullptr;
	VkDeviceMemory VkPlayer::vbmem = nullptr;

	int VkPlayer::frame = 1;
	float VkPlayer::dt = 1.0f / 60, VkPlayer::tp = 0, VkPlayer::idt = 60.0f;

	void VkPlayer::start() {
		if (init()) {
			mainLoop();
			vkDeviceWaitIdle(device);
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
			&& createRenderPasses()
			&& createFramebuffers()
			&& createUniformBuffer()
			&& createDescriptorSet()
			&& createPipelines()
			&& createFixedVertexBuffer()
			&& createSemaphore()
			;
	}

	void VkPlayer::mainLoop() {
		for (frame = 1; glfwWindowShouldClose(window) != GLFW_TRUE; frame++) {
			glfwPollEvents();
			static float prev = 0;
			tp = (float)glfwGetTime();
			dt = tp - prev;
			idt = 1.0f / dt;
			//if ((frame & 15) == 0) printf("%f\r",idt);
			if ((frame & 1023) == 0) printf("%f\r", frame / glfwGetTime());
			prev = tp;
			// loop body
			fixedDraw();
		}
	}

	void VkPlayer::finalize() {
		destroySemaphore();
		destroyFixedVertexBuffer();
		destroyPipelines();
		destroyDescriptorSet();
		destroyUniformBuffer();
		destroyFramebuffers();
		destroyRenderPasses();
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
			// printf("maximum %d buffers\n",properties.limits.maxMemoryAllocationCount);
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
		info.queueCreateInfoCount = 1 + (physicalDevice.graphicsFamily != physicalDevice.presentFamily);
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
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		
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
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.card, surface, &count, modes.data());

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
			info.pQueueFamilyIndices = &physicalDevice.graphicsFamily; // 나중에 더 직관적으로 변경
		}
		if (vkCreateSwapchainKHR(device, &info, nullptr, &swapchain) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create swapchain\n");
			return false;
		}
		swapchainExtent = info.imageExtent;
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

	bool VkPlayer::createRenderPasses() {
		return createRenderPass0();
	}

	void VkPlayer::destroyRenderPasses() {
		destroyRenderPass0();
	}

	bool VkPlayer::createRenderPass0() {
		VkAttachmentDescription colorAttachment{};	// 실제 첨부물이 아님. 실제 첨부물을 읽을 방식을 정의
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};	// 서브패스가 볼 번호와 레이아웃 정의
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;	// 0번 서브패스가 외부에 대한 의존성이 존재
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 대기할 srcSubpass에서의 작업 유형
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = 1;
		info.pAttachments = &colorAttachment;
		info.pSubpasses = &subpass;
		info.subpassCount = 1;
		info.pDependencies = &dependency;
		info.dependencyCount = 1;
		if (vkCreateRenderPass(device, &info, nullptr, &renderPass0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create render pass 0\n");
			return false;
		}
		return true;
	}

	void VkPlayer::destroyRenderPass0() {
		vkDestroyRenderPass(device, renderPass0, nullptr);
	}

	bool VkPlayer::createFramebuffers() {
		return createEndFramebuffers();
	}

	bool VkPlayer::createEndFramebuffers() {
		endFramebuffers.resize(swapchainImageViews.size());
		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.width = swapchainExtent.width;
		info.height = swapchainExtent.height;
		info.layers = 1;
		info.renderPass = renderPass0;

		for (size_t i = 0; i < swapchainImageViews.size(); i++) {
			VkImageView attachments[] = { swapchainImageViews[i] };
			info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
			info.pAttachments = attachments;
			if (vkCreateFramebuffer(device, &info, nullptr, &endFramebuffers[i]) != VK_SUCCESS) {
				fprintf(stderr, "Failed to create framebuffer\n");
				return false;
			}
		}
		return true;
	}

	void VkPlayer::destroyFramebuffers() {
		destroyEndFramebuffers();
	}

	void VkPlayer::destroyEndFramebuffers() {
		for (VkFramebuffer fb : endFramebuffers) {
			vkDestroyFramebuffer(device, fb, nullptr);
		}
	}

	bool VkPlayer::createPipelines() {
		return createPipeline0();
	}

	void VkPlayer::destroyPipelines() {
		destroyPipeline0();
	}

	struct Vertex {	// 임시
		float pos[3];
		float color[3];
	};

	template <class T, unsigned D>	// 임시
	struct UBentry {
		alignas(sizeof(T)* (D <= 2 ? 2 : 4)) T entry[D];
		T& operator[](size_t p) { return entry[p]; }
	};
	template <class T>
	struct UBentry<T, 1> {
		alignas(sizeof(T)) T value;
		T& operator=(T v) { return value = v; }
	};
	using UBfloat = UBentry<float, 1>;
	using UBint = UBentry<int, 1>;
	using UBbool = UBentry<int, 1>;
	using UBvec2 = UBentry<float, 2>;
	using UBvec3 = UBentry<float, 3>;
	using UBvec4 = UBentry<float, 4>;
	using UBivec2 = UBentry<int, 2>;
	using UBivec3 = UBentry<int, 3>;
	using UBivec4 = UBentry<int, 4>;
	using UBmat4 = UBentry<float, 16>;

	bool VkPlayer::createPipeline0() {
		// programmable function: 세이더 종류는 수십 가지쯤 돼 보이므로, 오버로드로 수용할 것
		VkShaderModule vertModule = createShaderModule("tri.vert", shaderc_shader_kind::shaderc_glsl_vertex_shader);
		VkShaderModule fragModule = createShaderModule("tri.frag", shaderc_shader_kind::shaderc_glsl_fragment_shader);
		VkPipelineShaderStageCreateInfo shaderStagesInfo[2] = {};
		shaderStagesInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStagesInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStagesInfo[0].module = vertModule;
		shaderStagesInfo[0].pName = "main";

		shaderStagesInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStagesInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStagesInfo[1].module = fragModule;
		shaderStagesInfo[1].pName = "main";

		// fixed function: 정점 입력 데이터 형식. 우선 공란
		VkVertexInputBindingDescription vbind{};
		vbind.binding = 0;
		vbind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vbind.stride = sizeof(Vertex);

		VkVertexInputAttributeDescription vattrs[2]{};
		vattrs[0].binding = 0;
		vattrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vattrs[0].location = 0;
		vattrs[0].offset = offsetof(Vertex, pos);

		vattrs[1].binding = 0;
		vattrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vattrs[1].location = 1;
		vattrs[1].offset = offsetof(Vertex, color);
		
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &vbind;
		vertexInputInfo.vertexAttributeDescriptionCount = 2;
		vertexInputInfo.pVertexAttributeDescriptions = vattrs;
		
		// fixed function: 정점 모으기
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// 3개씩 끊어 삼각형
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;	// 0xffff 혹은 0xffffffff 인덱스로 스트립 끊기 가능 여부
		
		// fixed function: 뷰포트, 시저 (뷰포트: cvv상 그림이 그려질 최종 직사각형, 시저: 스왑체인 이미지에서 그려지는 것을 허용할 부분)
		// ** 뷰포트는 런타임에 조정 가능 ** 
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchainExtent.width;
		viewport.height = (float)swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = swapchainExtent;

		VkPipelineViewportStateCreateInfo viewportStateInfo{};
		viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.pViewports = &viewport;
		viewportStateInfo.scissorCount = 1;
		viewportStateInfo.pScissors = &scissor;

		// fixed function: 래스터라이저
		VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
		rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerInfo.depthClampEnable = VK_FALSE;	// 표준 뷰 볼륨의 z좌표가 초과하면 잘라내지 않고 z값 자체를 줄이면서 살림
		rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;	// TRUE인 경우 출력이 안됨
		rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;	// 와이어프레임 등 지정 가능. 아마 건드릴 일 없을 것
		rasterizerInfo.lineWidth = 1.0f;					// 선 너비: 건드릴 일 없음
		rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;	// 면 거름: 건드릴 일 있음
		rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// 이건 건드릴 일 없을 듯
		rasterizerInfo.depthBiasEnable = VK_FALSE;			// 여기부터 4개는 깊이 값을 직접 건드리는 부분. 사용할 일 없을 듯
		rasterizerInfo.depthBiasConstantFactor = 0.0f;
		rasterizerInfo.depthBiasClamp = 0.0f;
		rasterizerInfo.depthBiasSlopeFactor = 0.0f;
		
		// fixed function: 멀티샘플링 (현재는 공란으로)
		// 안티에일리어싱에 사용하는 경우에 대하여 매개변수 수용할 필요가 있을듯
		VkPipelineMultisampleStateCreateInfo msInfo{};
		msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		msInfo.sampleShadingEnable = VK_FALSE;
		msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		msInfo.minSampleShading = 1.0f;
		msInfo.alphaToCoverageEnable = VK_FALSE;
		msInfo.alphaToOneEnable = VK_FALSE;
		msInfo.pSampleMask = nullptr;

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		
		// fixed function: 색 블렌딩
		// 블렌딩은 대체로 SRC_ALPHA, 1-SRC_ALPHA로 하니 반투명이 없을 거라면 성능을 위해 끄는 정도?
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_TRUE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.logicOpEnable = VK_FALSE;
		colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendInfo.attachmentCount = 1;
		colorBlendInfo.pAttachments = &colorBlendAttachmentState;
		colorBlendInfo.blendConstants[0] = 0.0f;
		colorBlendInfo.blendConstants[1] = 0.0f;
		colorBlendInfo.blendConstants[2] = 0.0f;
		colorBlendInfo.blendConstants[3] = 0.0f;

		VkDynamicState dynamicStates[1] = { /*VK_DYNAMIC_STATE_VIEWPORT*/ };
		VkPipelineDynamicStateCreateInfo dynamics{};
		dynamics.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamics.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
		dynamics.pDynamicStates = dynamicStates;

		// fixed function: 파이프라인 레이아웃: uniform 및 push 변수에 관한 것
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &ubds;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout0);

		// 파이프라인 생성
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = sizeof(shaderStagesInfo) / sizeof(shaderStagesInfo[0]);
		pipelineInfo.pStages = shaderStagesInfo;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasterizerInfo;
		pipelineInfo.pMultisampleState = &msInfo;
		pipelineInfo.pDepthStencilState = nullptr;	// 보류
		pipelineInfo.pColorBlendState = &colorBlendInfo;
		pipelineInfo.pDynamicState = nullptr;	// 보류
		pipelineInfo.layout = pipelineLayout0;
		pipelineInfo.renderPass = renderPass0;
		pipelineInfo.subpass = 0;
		// 아래 2개: 기존 파이프라인을 기반으로 비슷하게 생성하기 위함
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline0);

		vkDestroyShaderModule(device, vertModule, nullptr);
		vkDestroyShaderModule(device, fragModule, nullptr);
		if (result != VK_SUCCESS) {
			fprintf(stderr, "Failed to create pipeline 0\n");
			return false;
		}
		return true;
	}

	void VkPlayer::destroyPipeline0() {
		vkDestroyPipelineLayout(device, pipelineLayout0, nullptr);
		vkDestroyPipeline(device, pipeline0, nullptr);
	}

	static std::vector<uint32_t> compileShader(const char* code, size_t size, shaderc_shader_kind kind, const char* name) {
		shaderc::Compiler compiler;
		shaderc::CompileOptions opts;
		opts.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);
		opts.SetInvertY(true);	// 벌칸은 아래쪽이 y=1임 -> opengl과 통일을 위해. * z축은 0~1 범위.. 인데 glsl에는 이 옵션이 의미가 없다는 말이 있음
		shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(code, size, kind, name, opts);
		if (result.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success) {
			fprintf(stderr, "compiler : %s\n", result.GetErrorMessage().c_str());
		}
		return { result.cbegin(),result.cend() };
	}

	static std::vector<uint32_t> compileShader(const char* fileName, shaderc_shader_kind kind) {
		FILE* fp;
		fopen_s(&fp, fileName, "rb");
		if (!fp) {
			perror("fopen_s");
			return {};
		}
		fseek(fp, 0, SEEK_END);
		size_t sz = (size_t)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		std::string code;
		code.resize(sz);
		fread_s(code.data(), sz, 1, sz, fp);
		std::vector<uint32_t> result = compileShader(code.c_str(), code.size(), kind, fileName);
		fclose(fp);
		return result;
	}

	static std::vector<uint32_t> loadShader(const char* fileName) {
		FILE* fp;
		fopen_s(&fp, fileName, "rb");
		if (!fp) {
			perror("fopen_s");
			return {};
		}
		fseek(fp, 0, SEEK_END);
		size_t sz = (size_t)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		std::vector<uint32_t> bcode(sz / sizeof(uint32_t));
		fread_s(bcode.data(), sz, sizeof(uint32_t), sz / sizeof(uint32_t), fp);
		fclose(fp);
		return bcode;
	}

	VkShaderModule VkPlayer::createShaderModuleFromSpv(const std::vector<uint32_t>& bcode) {
		VkShaderModule ret;
		VkShaderModuleCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.pCode = bcode.data();
		info.codeSize = bcode.size() * sizeof(uint32_t);
		VkResult res = vkCreateShaderModule(device, &info, nullptr, &ret);
		if (res == VK_SUCCESS) return ret;
		else return nullptr;
	}

	VkShaderModule VkPlayer::createShaderModule(const char* fileName) { return createShaderModuleFromSpv(loadShader(fileName)); }
	VkShaderModule VkPlayer::createShaderModule(const char* fileName, shaderc_shader_kind kind) { return createShaderModuleFromSpv(compileShader(fileName, kind)); }
	VkShaderModule VkPlayer::createShaderModule(const char* code, size_t size, shaderc_shader_kind kind, const char* name) { return createShaderModuleFromSpv(compileShader(code, size, kind, name)); }

	static uint32_t findMemorytype(uint32_t typeFilter, VkMemoryPropertyFlags props, VkPhysicalDevice card) {
		VkPhysicalDeviceMemoryProperties mprops;
		vkGetPhysicalDeviceMemoryProperties(card, &mprops);
		for (uint32_t i = 0; i < mprops.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && ((mprops.memoryTypes[i].propertyFlags & props) == props)) {
				return i;
			}
		}
		return -1;
	}

	bool VkPlayer::createFixedVertexBuffer() {
		Vertex ar[]{
			{{-0.5,0.5,0},{1,0,0}},
			{{0.5,0.5,0},{0,1,0}},
			{{0,-0.5,0},{0,0,1}},
		};
		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		info.size = sizeof(ar);
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		VkBuffer vb;
		VkDeviceMemory vbmem;

		if (vkCreateBuffer(device, &info, nullptr, &vb) != VK_SUCCESS) {
			fprintf(stderr,"Failed to create fixed vertex buffer\n");
			return false;
		}
		
		VkMemoryRequirements mreq;
		vkGetBufferMemoryRequirements(device, vb, &mreq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice.card);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &vbmem) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate memory for fixed vb\n");
			return false;
		}
		void* data;
		if (vkMapMemory(device, vbmem, 0, info.size, 0, &data) != VK_SUCCESS) {
			fprintf(stderr, "Failed to map to allocated memory\n");
			return false;
		}
		memcpy(data, ar, info.size);
		vkUnmapMemory(device, vbmem);
		if (vkBindBufferMemory(device, vb, vbmem, 0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to bind buffer object and memory\n");
			return false;
		}

		info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (vkCreateBuffer(device, &info, nullptr, &VkPlayer::vb) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create fixed vertex buffer\n");
			return false;
		}
		vkGetBufferMemoryRequirements(device, vb, &mreq);
		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice.card);
		if (vkAllocateMemory(device, &allocInfo, nullptr, &VkPlayer::vbmem) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate memory for fixed vb\n");
			return false;
		}
		if (vkBindBufferMemory(device, VkPlayer::vb, VkPlayer::vbmem, 0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to bind buffer object and memory\n");
			return false;
		}

		VkCommandBufferAllocateInfo cmdInfo{};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdInfo.commandBufferCount = 1;
		cmdInfo.commandPool = commandPool;

		VkCommandBuffer copyBuffer;
		if (vkAllocateCommandBuffers(device, &cmdInfo, &copyBuffer) != VK_SUCCESS) {
			fprintf(stderr,"Failed to allocate command buffer for copying vertex buffer\n");
			return false;
		}
		VkCommandBufferBeginInfo copyBegin{};
		copyBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		copyBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(copyBuffer, &copyBegin) != VK_SUCCESS) {
			fprintf(stderr, "Failed to begin command buffer for copying vertex buffer\n");
			return false;
		}
		VkBufferCopy copyRegion{};
		copyRegion.size = sizeof(ar); // 오프셋 설정 가능
		vkCmdCopyBuffer(copyBuffer, vb, VkPlayer::vb, 1, &copyRegion);
		if (vkEndCommandBuffer(copyBuffer) != VK_SUCCESS) {
			fprintf(stderr, "Failed to end command buffer for copying vertex buffer\n");
			return false;
		}
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyBuffer;
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			fprintf(stderr, "Failed to submit command buffer for copying vertex buffer\n");
			return false;
		}
		vkQueueWaitIdle(graphicsQueue);
		vkFreeCommandBuffers(device, commandPool, 1, &copyBuffer);
		vkDestroyBuffer(device, vb, nullptr);
		vkFreeMemory(device, vbmem, nullptr);
		return true;
	}

	void VkPlayer::destroyFixedVertexBuffer() {
		vkFreeMemory(device, vbmem, nullptr);
		vkDestroyBuffer(device, vb, nullptr);
	}

	void VkPlayer::fixedDraw() {
		uint32_t imgIndex;
		if (vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, fixedSp[commandBufferNumber], nullptr, &imgIndex) != VK_SUCCESS) {
			fprintf(stderr, "Fail 1\n");
			return;
		}

		vkWaitForFences(device, 1, &bufferFence[commandBufferNumber], VK_FALSE, UINT64_MAX);
		vkResetFences(device, 1, &bufferFence[commandBufferNumber]);
		vkResetCommandBuffer(commandBuffers[commandBufferNumber], 0);

		float st = sinf(tp), ct = cosf(tp);
		float asp = (float)swapchainExtent.height / swapchainExtent.width;
		float rotation[16] = {
			ct*asp,st,0,0,
			-st*asp,ct,0,0,
			0,0,1,0,
			0,0,0,1
		};
		memcpy(ubmap[commandBufferNumber], rotation, sizeof(rotation));

		VkCommandBufferBeginInfo buffbegin{};
		buffbegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		buffbegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(commandBuffers[commandBufferNumber], &buffbegin) != VK_SUCCESS) {
			fprintf(stderr, "Fail 2\n");
			return;
		}

		VkRenderPassBeginInfo rpbegin{};
		rpbegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpbegin.renderPass = renderPass0;
		rpbegin.framebuffer = endFramebuffers[imgIndex];
		rpbegin.renderArea.offset = { 0,0 };
		rpbegin.renderArea.extent = { swapchainExtent.width, swapchainExtent.height };
		VkClearValue clearColor = { 0.03f,0.03f,0.03f,1.0f };
		rpbegin.clearValueCount = 1;
		rpbegin.pClearValues = &clearColor;
		vkCmdBeginRenderPass(commandBuffers[commandBufferNumber], &rpbegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[commandBufferNumber], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline0);
		const VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[commandBufferNumber], 0, 1, &vb, offsets);
		vkCmdBindDescriptorSets(commandBuffers[commandBufferNumber], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout0, 0, 1, &ubset[commandBufferNumber], 0, nullptr);
		vkCmdDraw(commandBuffers[commandBufferNumber], 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[commandBufferNumber]);
		if (vkEndCommandBuffer(commandBuffers[commandBufferNumber]) != VK_SUCCESS) {
			fprintf(stderr, "Fail 3\n");
			return;
		}
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[commandBufferNumber];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &fixedSp[commandBufferNumber];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &presentSp[commandBufferNumber];
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, bufferFence[commandBufferNumber]) != VK_SUCCESS) {
			fprintf(stderr, "Fail 4\n");
			return;
		}
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.swapchainCount = 1;
		presentInfo.pImageIndices = &imgIndex;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &presentSp[commandBufferNumber];
		
		if (vkQueuePresentKHR(presentQueue, &presentInfo) != VK_SUCCESS) {
			fprintf(stderr, "Fail 5\n");
			return;
		}
		commandBufferNumber = (commandBufferNumber + 1) % COMMANDBUFFER_COUNT;
	}

	bool VkPlayer::createSemaphore() {
		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i = 0; i < COMMANDBUFFER_COUNT; i++) {
			if (vkCreateSemaphore(device, &info, nullptr, &fixedSp[i]) != VK_SUCCESS) {
				fprintf(stderr, "Failed to create semaphore\n");
				return false;
			}
			if (vkCreateSemaphore(device, &info, nullptr, &presentSp[i]) != VK_SUCCESS) {
				fprintf(stderr, "Failed to create semaphore\n");
				return false;
			}
			if (vkCreateFence(device, &fenceInfo, nullptr, &bufferFence[i]) != VK_SUCCESS) {
				fprintf(stderr, "Failed to create fence\n");
				return false;
			}
		}
		return true;
	}

	void VkPlayer::destroySemaphore() {
		for (int i = 0; i < COMMANDBUFFER_COUNT; i++) {
			vkDestroySemaphore(device, fixedSp[i], nullptr);
			vkDestroySemaphore(device, presentSp[i], nullptr);
			vkDestroyFence(device, bufferFence[i], nullptr);
		}
	}

	bool VkPlayer::createUniformBuffer() {
		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		info.size = sizeof(float) * 16;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		for (int i = 0; i < COMMANDBUFFER_COUNT; i++) {
			if (vkCreateBuffer(device, &info, nullptr, &ub[i]) != VK_SUCCESS) {
				fprintf(stderr, "Failed to create fixed uniform buffer\n");
				return false;
			}
		}

		for (int i = 0; i < COMMANDBUFFER_COUNT; i++) {
			VkMemoryRequirements mreq;
			vkGetBufferMemoryRequirements(device, ub[i], &mreq);
			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = mreq.size;
			allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice.card);
			if (vkAllocateMemory(device, &allocInfo, nullptr, &ubmem[i]) != VK_SUCCESS) {
				fprintf(stderr, "Failed to allocate memory for fixed ub\n");
				return false;
			}
			if (vkMapMemory(device, ubmem[i], 0, info.size, 0, &ubmap[i]) != VK_SUCCESS) {
				fprintf(stderr, "Failed to map to allocated memory\n");
				return false;
			}
			if (vkBindBufferMemory(device, ub[i], ubmem[i], 0) != VK_SUCCESS) {
				fprintf(stderr, "Failed to bind ub and memory\n");
				return false;
			}
		}
		return true;
	}

	void VkPlayer::destroyUniformBuffer() {
		for (int i = 0; i < COMMANDBUFFER_COUNT; i++) {
			vkFreeMemory(device, ubmem[i], nullptr);
			ubmap[i] = nullptr;
			vkDestroyBuffer(device, ub[i], nullptr);
		}
	}

	bool VkPlayer::createDescriptorSet() {
		VkDescriptorSetLayoutBinding uboBinding{};
		uboBinding.binding = 0;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.descriptorCount = 1;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	// VK_SHADER_STAGE_ALL_GRAPHICS

		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = 1;
		info.pBindings = &uboBinding;
		
		if (vkCreateDescriptorSetLayout(device, &info, nullptr, &ubds) != VK_SUCCESS) {
			fprintf(stderr,"Failed to create descriptor set layout for uniform buffer\n");
			return false;
		}
		
		VkDescriptorPoolSize size{};
		size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		size.descriptorCount = COMMANDBUFFER_COUNT;

		VkDescriptorPoolCreateInfo dpinfo{};
		dpinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		dpinfo.poolSizeCount = 1;
		dpinfo.pPoolSizes = &size;
		dpinfo.maxSets = COMMANDBUFFER_COUNT;
		
		if (vkCreateDescriptorPool(device, &dpinfo, nullptr, &ubpool) != VK_SUCCESS) {
			fprintf(stderr,"Failed to create descriptor pool\n");
			return false;
		}
		
		std::vector<VkDescriptorSetLayout> layouts(COMMANDBUFFER_COUNT, ubds);
		VkDescriptorSetAllocateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setInfo.descriptorPool = ubpool;
		setInfo.descriptorSetCount = COMMANDBUFFER_COUNT;
		setInfo.pSetLayouts = layouts.data();
		
		if (vkAllocateDescriptorSets(device, &setInfo, ubset) != VK_SUCCESS) {
			fprintf(stderr,"Failed to allocate descriptor set\n");
			return false;
		}

		for (size_t i = 0; i < COMMANDBUFFER_COUNT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = ub[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(float) * 16;
			
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = ubset[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		}

		return true;
	}

	void VkPlayer::destroyDescriptorSet() {
		vkDestroyDescriptorPool(device, ubpool, nullptr);
		vkDestroyDescriptorSetLayout(device, ubds, nullptr);
	}	
}