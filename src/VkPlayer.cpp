/***********************************************************************
MIT License
Copyright (c) 2022 onart
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*************************************************************************/

#define VMA_VULKAN_VERSION 1000000
#define VMA_IMPLEMENTATION
#include "externals/vk_mem_alloc.h"

#include "VkPlayer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "externals/stb_image.h"
#include <thread>
#include <algorithm>
#include <cstring>

#ifdef _MSC_VER
#pragma comment(lib, "externals/vulkan/vulkan-1.lib")
#pragma comment(lib, "externals/glfw/glfw3_mt.lib")
#pragma comment(lib, "externals/shaderc/shaderc_shared.lib")
#endif

namespace onart {

	int rwid = 16, rheight = 9;
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

	VkImage VkPlayer::dsImage = nullptr;
	VkImageView VkPlayer::dsImageView = nullptr;
	VkDeviceMemory VkPlayer::dsmem = nullptr;
	VkFormat VkPlayer::dsImageFormat;

	VkPipeline VkPlayer::pipeline1 = nullptr;
	VkPipelineLayout VkPlayer::pipelineLayout1 = nullptr;

	VkBuffer VkPlayer::ub[VkPlayer::COMMANDBUFFER_COUNT] = {};
	VkDeviceMemory VkPlayer::ubmem[VkPlayer::COMMANDBUFFER_COUNT] = {};
	void* VkPlayer::ubmap[VkPlayer::COMMANDBUFFER_COUNT] = {};
	VkDescriptorSetLayout VkPlayer::ubds = nullptr;
	VkDescriptorPool VkPlayer::ubpool = nullptr;
	VkDescriptorSet VkPlayer::ubset[VkPlayer::COMMANDBUFFER_COUNT] = {};

	VkBuffer VkPlayer::vb = nullptr, VkPlayer::ib = nullptr;
	VkDeviceMemory VkPlayer::vbmem = nullptr, VkPlayer::ibmem = nullptr;

	VkImage VkPlayer::tex0 = nullptr, VkPlayer::tex1 = nullptr;
	VkImageView VkPlayer::texview0 = nullptr, VkPlayer::texview1 = nullptr;
	VkDeviceMemory VkPlayer::texmem0 = nullptr, VkPlayer::texmem1 = nullptr;
	VkSampler VkPlayer::sampler0 = nullptr;
	VkDescriptorSet VkPlayer::samplerSet[1] = {};

	VkDescriptorSetLayout VkPlayer::sp1layout = nullptr;
	VkDescriptorPool VkPlayer::sp1pool = nullptr;
	VkDescriptorSet VkPlayer::sp1set = nullptr;

	VkImage VkPlayer::middleImage = nullptr;
	VkDeviceMemory VkPlayer::middleMem = nullptr;
	VkImageView VkPlayer::middleImageView = nullptr;

	VmaAllocator VkPlayer::allocator = nullptr;

	bool VkPlayer::extSupported[(size_t)VkPlayer::OptionalEXT::OPTIONAL_EXT_MAX_ENUM] = {};

	int VkPlayer::frame = 1;
	float VkPlayer::dt = 1.0f / 60, VkPlayer::tp = 0, VkPlayer::idt = 60.0f;

	bool VkPlayer::resizing = false, VkPlayer::shouldRecreateSwapchain = false;
	VkDeviceSize VkPlayer::minUniformBufferOffset;

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
			&& initAllocator()
			&& createCommandPool()
			&& createSwapchain()
			&& createSwapchainImageViews()
			&& createDSBuffer()
			&& createRenderPasses()
			&& createFramebuffers()
			&& createUniformBuffer()
			&& createTex0()
			&& createSampler0()
			&& createDescriptorSet()
			&& createPipelines()
			&& createFixedVertexBuffer()
			&& createFixedIndexBuffer()
			&& createSemaphore()
			;
	}

	void VkPlayer::mainLoop() {
		for (frame = 1; glfwWindowShouldClose(window) != GLFW_TRUE; frame++) {
			glfwPollEvents();
			if (resizing) {
				resizing = false;
				continue;
			}
			if (shouldRecreateSwapchain) {
				int width = 0, height = 0;
				do {
					glfwGetFramebufferSize(window, &width, &height);
					glfwWaitEvents();
				} while (width == 0 || height == 0);
				vkDeviceWaitIdle(device);

				destroyDescriptorSet();
				destroyPipelines();
				destroyFramebuffers();
				destroyDSBuffer();
				destroySwapchainImageViews();
				destroySwapchain();

				createSwapchain();
				createSwapchainImageViews();
				createDSBuffer();
				createFramebuffers();
				createDescriptorSet();
				createPipelines();
				shouldRecreateSwapchain = false;
			}
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
		destroyFixedIndexBuffer();
		destroyFixedVertexBuffer();
		destroyPipelines();
		destroyDescriptorSet();
		destroySampler0();
		destroyTex0();
		destroyUniformBuffer();
		destroyFramebuffers();
		destroyRenderPasses();
		destroyDSBuffer();
		destroySwapchainImageViews();
		destroySwapchain();
		destroyCommandPool();
		destroyDevice();
		destroyWindow();
		destroyInstance();
	}

	bool VkPlayer::createInstance() {
		VkApplicationInfo ainfo{};
		ainfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;	// ������
		ainfo.apiVersion = VK_API_VERSION_1_0;	//VK_HEADER_VERSION_COMPLETE;

		ainfo.pApplicationName = "OAVKE";
		ainfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
		ainfo.pEngineName = "OAVKE";
		ainfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);

		VkInstanceCreateInfo info{};	// �ϴ� ��� 0���� �ʱ�ȭ��
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	// ������
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
			//printf("maximum %lld buffers\n",properties.limits.minUniformBufferOffsetAlignment);

			vkGetPhysicalDeviceFeatures(cards[i], &features);
			if (!checkDeviceExtension(cards[i])) continue;
			PhysicalDevice pd = setQueueFamily(cards[i]);
			if (pd.card) {
				physicalDevice = pd;
				extSupported[(size_t)OptionalEXT::ANISOTROPIC] = features.samplerAnisotropy;
				minUniformBufferOffset = properties.limits.minUniformBufferOffsetAlignment;
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
		features.samplerAnisotropy = hasExt(OptionalEXT::ANISOTROPIC);

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

	void VkPlayer::onResize(GLFWwindow* window, int width, int height) {
		resizing = true;
		shouldRecreateSwapchain = true;
	}

	bool VkPlayer::createWindow() {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		window = glfwCreateWindow(width, height, u8"OAVKE", nullptr, nullptr);
		if (!window) {
			fprintf(stderr, "Failed to create window\n");
			return false;
		}
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create window surface\n");
			return false;
		}
		glfwSetFramebufferSizeCallback(window, onResize);
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
			if (form.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && form.format == VK_FORMAT_B8G8R8A8_SRGB) { sf = form; }
		}

		uint32_t idx[2] = { physicalDevice.graphicsFamily,physicalDevice.presentFamily };

		VkSwapchainCreateInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.surface = surface;
		
		info.minImageCount = std::min(caps.minImageCount - 1, caps.maxImageCount - 1) + 1;
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
			info.pQueueFamilyIndices = &physicalDevice.graphicsFamily; // ���߿� �� ���������� ����
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
				fprintf(stderr, "Failed to create image views\n");
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
		VkAttachmentDescription attachments[3]{};
		VkAttachmentDescription& colorAttachment = attachments[0];
		VkAttachmentDescription& depthAttachment = attachments[1];
		VkAttachmentDescription& middleColorAttachment = attachments[2];

		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		depthAttachment.format = dsImageFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		middleColorAttachment.format = swapchainImageFormat;
		middleColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		middleColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		middleColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		middleColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		middleColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		middleColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		middleColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};	// �����н��� �� ��ȣ�� ���̾ƿ� ����
		colorAttachmentRef.attachment = 2;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference inputAttachmentRef{};
		inputAttachmentRef.attachment = 2;
		inputAttachmentRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference finalColor{};
		finalColor.attachment = 0;
		finalColor.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpasses[2] = {};
		VkSubpassDescription& subpass0 = subpasses[0];
		VkSubpassDescription& subpass1 = subpasses[1];

		subpass0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass0.colorAttachmentCount = 1;
		subpass0.pColorAttachments = &colorAttachmentRef;
		subpass0.pDepthStencilAttachment = &depthAttachmentRef;

		subpass1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass1.colorAttachmentCount = 1;
		subpass1.pColorAttachments = &finalColor;
		subpass1.inputAttachmentCount = 1;
		subpass1.pInputAttachments = &inputAttachmentRef;

		VkSubpassDependency dependencies[2] = {};
		VkSubpassDependency& dependency0 = dependencies[0];
		VkSubpassDependency& dependency1 = dependencies[1];
		dependency0.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency0.dstSubpass = 1;	// 1�� �����н��� �ܺο� ���� �������� ����
		dependency0.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // ����� srcSubpass������ �۾� ����
		dependency0.srcAccessMask = 0;
		dependency0.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 
		dependency0.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependency1.srcSubpass = 0;
		dependency1.dstSubpass = 1;
		dependency1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency1.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependency1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
		info.pAttachments = attachments;
		info.subpassCount = sizeof(subpasses) / sizeof(subpasses[0]);
		info.pSubpasses = subpasses;
		info.dependencyCount = sizeof(dependencies) / sizeof(dependencies[0]);
		info.pDependencies = dependencies;
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

		VkImageView attachments[] = { nullptr,dsImageView, middleImageView };
		for (size_t i = 0; i < swapchainImageViews.size(); i++) {
			attachments[0] = swapchainImageViews[i];
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
		return createPipeline0() && createPipeline1();
	}

	void VkPlayer::destroyPipelines() {
		destroyPipeline1();
		destroyPipeline0();
	}

	struct Vertex {	// �ӽ�
		float pos[3];
		float tc[2];
	};

	template <class T, unsigned D>	// �ӽ�
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
		// programmable function: ���̴� ������ ���� ������ �� ���̹Ƿ�, �����ε�� ������ ��
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

		// fixed function: ���� �Է� ������ ����. �켱 ����
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
		vattrs[1].format = VK_FORMAT_R32G32_SFLOAT;
		vattrs[1].location = 1;
		vattrs[1].offset = offsetof(Vertex, tc);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &vbind;
		vertexInputInfo.vertexAttributeDescriptionCount = 2;
		vertexInputInfo.pVertexAttributeDescriptions = vattrs;

		// fixed function: ���� ������
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// 3���� ���� �ﰢ��
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;	// 0xffff Ȥ�� 0xffffffff �ε����� ��Ʈ�� ���� ���� ����

		// fixed function: ����Ʈ, ���� (����Ʈ: cvv�� �׸��� �׷��� ���� ���簢��, ����: ����ü�� �̹������� �׷����� ���� ����� �κ�)
		// ** ����Ʈ�� ��Ÿ�ӿ� ���� ���� ** 
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

		// fixed function: �����Ͷ�����
		VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
		rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerInfo.depthClampEnable = VK_FALSE;	// ǥ�� �� ������ z��ǥ�� �ʰ��ϸ� �߶��� �ʰ� z�� ��ü�� ���̸鼭 �츲
		rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;	// TRUE�� ��� ����� �ȵ�
		rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;	// ���̾������� �� ���� ����. �Ƹ� �ǵ帱 �� ���� ��
		rasterizerInfo.lineWidth = 1.0f;					// �� �ʺ�: �ǵ帱 �� ����
		rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;	// �� �Ÿ�: �ǵ帱 �� ����
		rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// �̰� �ǵ帱 �� ���� ��
		rasterizerInfo.depthBiasEnable = VK_FALSE;			// ������� 4���� ���� ���� ���� �ǵ帮�� �κ�. ����� �� ���� ��
		rasterizerInfo.depthBiasConstantFactor = 0.0f;
		rasterizerInfo.depthBiasClamp = 0.0f;
		rasterizerInfo.depthBiasSlopeFactor = 0.0f;

		// fixed function: ��Ƽ���ø� (����� ��������)
		// ��Ƽ���ϸ���̿� ����ϴ� ��쿡 ���Ͽ� �Ű����� ������ �ʿ䰡 ������
		VkPipelineMultisampleStateCreateInfo msInfo{};
		msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		msInfo.sampleShadingEnable = VK_FALSE;
		msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		msInfo.minSampleShading = 1.0f;
		msInfo.alphaToCoverageEnable = VK_FALSE;
		msInfo.alphaToOneEnable = VK_FALSE;
		msInfo.pSampleMask = nullptr;

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilInfo.depthTestEnable = VK_TRUE;
		depthStencilInfo.depthWriteEnable = VK_TRUE;
		depthStencilInfo.stencilTestEnable = VK_TRUE;
		depthStencilInfo.front.compareMask = 0xff;
		depthStencilInfo.front.compareOp = VkCompareOp::VK_COMPARE_OP_EQUAL;
		depthStencilInfo.front.writeMask = 0xff;
		depthStencilInfo.front.failOp = VkStencilOp::VK_STENCIL_OP_KEEP;
		depthStencilInfo.front.depthFailOp = VkStencilOp::VK_STENCIL_OP_KEEP;
		depthStencilInfo.front.passOp = VkStencilOp::VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		depthStencilInfo.front.reference = 0x00;

		// fixed function: �� ������
		// �������� ��ü�� SRC_ALPHA, 1-SRC_ALPHA�� �ϴ� �������� ���� �Ŷ�� ������ ���� ���� ����?
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

		VkDynamicState dynamicStates[1] = { VK_DYNAMIC_STATE_VIEWPORT };
		VkPipelineDynamicStateCreateInfo dynamics{};
		dynamics.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamics.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
		dynamics.pDynamicStates = dynamicStates;

		VkPushConstantRange pushRange{};
		pushRange.offset = 0;
		pushRange.size = 20;
		pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// fixed function: ���������� ���̾ƿ�: uniform �� push ������ ���� ��
		VkDescriptorSetLayout setlayouts[2] = { ubds,ubds };
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = setlayouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushRange;
		vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout0);

		// ���������� ����
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = sizeof(shaderStagesInfo) / sizeof(shaderStagesInfo[0]);
		pipelineInfo.pStages = shaderStagesInfo;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasterizerInfo;
		pipelineInfo.pMultisampleState = &msInfo;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pColorBlendState = &colorBlendInfo;
		pipelineInfo.pDynamicState = &dynamics;
		pipelineInfo.layout = pipelineLayout0;
		pipelineInfo.renderPass = renderPass0;
		pipelineInfo.subpass = 0;
		// �Ʒ� 2��: ���� ������������ ������� ����ϰ� �����ϱ� ����
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
		opts.SetInvertY(true);	// ��ĭ�� �Ʒ����� y=1�� -> opengl�� ������ ����. * z���� 0~1 ����.. �ε� glsl���� �� �ɼ��� �ǹ̰� ���ٴ� ���� ����
		shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(code, size, kind, name, opts);
		if (result.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success) {
			fprintf(stderr, "compiler : %s\n", result.GetErrorMessage().c_str());
		}
		return { result.cbegin(),result.cend() };
	}
#ifndef MSC_VER
	int fopen_s(FILE** fpp, const char* fileName, const char* mode){
		*fpp = fopen(fileName, mode);
		return *fpp != NULL;
	}

	size_t fread_s(void *buffer, size_t bufferSize, size_t elementSize, size_t count, FILE *stream){
		return fread(buffer, elementSize, bufferSize >= elementSize * count ? count : bufferSize / elementSize, stream);
	}
#endif

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
			{{-0.5,0.5,0.99},{0,1}}, // ����
			{{0.5,0.5,0.99},{1,1}},  // ����
			{{0.5,-0.5,0.99},{1,0}}, // ���
			{{-0.5,-0.5,0.99},{0,0}}, // �»�

			{{-1,1,0.9},{0,1}},
			{{1,1,0.9},{1,1}},
			{{1,-1,0.9},{1,0}},
			{{-1,-1,0.9},{0,0}}
		};
		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		info.size = sizeof(ar);
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer vb;
		VkDeviceMemory vbmem;

		if (vkCreateBuffer(device, &info, nullptr, &vb) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create fixed vertex buffer\n");
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
		vkGetBufferMemoryRequirements(device, VkPlayer::vb, &mreq);
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
			fprintf(stderr, "Failed to allocate command buffer for copying vertex buffer\n");
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
		copyRegion.size = sizeof(ar); // ������ ���� ����
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
		float asp = (float)rheight / rwid;
		float rotation[16] = {
			ct * asp,st,0,0,
			-st * asp,ct,0,0,
			0,0,1,0,
			0,0,0,1
		};
		memcpy(ubmap[commandBufferNumber], rotation, sizeof(rotation));
		rotation[1] *= -1;
		rotation[4] *= -1;
		memcpy((char*)ubmap[commandBufferNumber] + minUniformBufferOffset, rotation, sizeof(rotation));

		VkCommandBufferBeginInfo buffbegin{};
		buffbegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		buffbegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(commandBuffers[commandBufferNumber], &buffbegin) != VK_SUCCESS) {
			fprintf(stderr, "Fail 2\n");
			return;
		}
		VkViewport viewport{};
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		float r = (float)rwid / rheight;

		if (swapchainExtent.width * rheight < swapchainExtent.height * rwid) {
			viewport.x = 0.0f;
			viewport.width = (float)swapchainExtent.width;
			viewport.height = swapchainExtent.width / r;
			viewport.y = (swapchainExtent.height - viewport.height) * 0.5f;
		}
		else {
			viewport.y = 0.0f;
			viewport.height = (float)swapchainExtent.height;
			viewport.width = swapchainExtent.height * r;
			viewport.x = (swapchainExtent.width - viewport.width) * 0.5f;
		}
		VkRect2D scissor{};

		if (swapchainExtent.width * rheight < swapchainExtent.height * rwid) {
			scissor.extent.width = swapchainExtent.width;
			scissor.extent.height = swapchainExtent.width * rheight / rwid;
			scissor.offset.x = 0;
			scissor.offset.y = int((swapchainExtent.height - scissor.extent.height) * 0.5f);
		}
		else {
			scissor.extent.height = swapchainExtent.height;
			scissor.extent.width = swapchainExtent.height * rwid / rheight;
			scissor.offset.y = 0;
			scissor.offset.x = int((swapchainExtent.width - scissor.extent.width) * 0.5f);
		}

		VkRenderPassBeginInfo rpbegin{};
		rpbegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpbegin.renderPass = renderPass0;
		rpbegin.framebuffer = endFramebuffers[imgIndex];
		rpbegin.renderArea.offset = { 0,0 };
		rpbegin.renderArea.extent = { swapchainExtent.width, swapchainExtent.height };
		VkClearValue clearColor = { 0.03f,0.03f,0.03f,1.0f };
		VkClearValue clearDepthStencil = { 1.0f,0 };
		VkClearValue clearValue[] = { clearColor, clearDepthStencil, clearColor };
		rpbegin.clearValueCount = sizeof(clearValue) / sizeof(clearValue[0]);
		rpbegin.pClearValues = clearValue;
		vkCmdBeginRenderPass(commandBuffers[commandBufferNumber], &rpbegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[commandBufferNumber], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline0);
		vkCmdSetViewport(commandBuffers[commandBufferNumber], 0, 1, &viewport);
		const VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[commandBufferNumber], 0, 1, &vb, offsets);
		vkCmdBindIndexBuffer(commandBuffers[commandBufferNumber], ib, 0, VK_INDEX_TYPE_UINT16);
		VkDescriptorSet bindDs[] = { ubset[commandBufferNumber],samplerSet[0] };
		uint32_t dynamicOffs[] = { 0,0 };
		vkCmdBindDescriptorSets(commandBuffers[commandBufferNumber], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout0, 0, 2, bindDs, sizeof(dynamicOffs) / sizeof(dynamicOffs[0]), dynamicOffs);
		struct {
			float clr[4] = { 1.0f,0,0,1 };
			int idx = 0;
		}psh;
		vkCmdPushConstants(commandBuffers[commandBufferNumber], pipelineLayout0, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 20, &psh);
		vkCmdDrawIndexed(commandBuffers[commandBufferNumber], 6, 1, 0, 0, 0);
		dynamicOffs[0] = minUniformBufferOffset;
		vkCmdBindDescriptorSets(commandBuffers[commandBufferNumber], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout0, 0, 2, bindDs, sizeof(dynamicOffs) / sizeof(dynamicOffs[0]), dynamicOffs);
		psh.clr[1] = 1.0f; psh.idx = 1;
		vkCmdPushConstants(commandBuffers[commandBufferNumber], pipelineLayout0, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 20, &psh);
		vkCmdDrawIndexed(commandBuffers[commandBufferNumber], 6, 1, 0, 4, 0);
		vkCmdBindPipeline(commandBuffers[commandBufferNumber], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline1);
		vkCmdSetScissor(commandBuffers[commandBufferNumber], 0, 1, &scissor);
		vkCmdBindDescriptorSets(commandBuffers[commandBufferNumber], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout1, 0, 1, &sp1set, 0, nullptr);
		vkCmdNextSubpass(commandBuffers[commandBufferNumber], VK_SUBPASS_CONTENTS_INLINE);
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
		VkDeviceSize dynamicAlignment = sizeof(float) * 16;
		if (minUniformBufferOffset > 0) {
			dynamicAlignment = dynamicAlignment + minUniformBufferOffset - 1;
			dynamicAlignment -= dynamicAlignment % minUniformBufferOffset;
		}

		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		info.size = dynamicAlignment * 2;
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
 // TODO: 다이나믹 디스크립터셋 오프셋 256고정 말고 진짜값으로 적용
	bool VkPlayer::createDescriptorSet() {
		VkDescriptorSetLayoutBinding dsBindings[2] = {};
		VkDescriptorSetLayoutBinding& uboBinding = dsBindings[0];
		VkDescriptorSetLayoutBinding& samplerBinding = dsBindings[1];

		uboBinding.binding = 0;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		uboBinding.descriptorCount = 1;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	// VK_SHADER_STAGE_ALL_GRAPHICS

		samplerBinding.binding = 1;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.descriptorCount = 2;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = sizeof(dsBindings) / sizeof(dsBindings[0]);
		info.pBindings = dsBindings;

		if (vkCreateDescriptorSetLayout(device, &info, nullptr, &ubds) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create descriptor set layout for uniform buffer\n");
			return false;
		}


		VkDescriptorPoolSize sizes[2] = {};

		VkDescriptorPoolSize& ubsize = sizes[0];
		VkDescriptorPoolSize& smsize = sizes[1];

		ubsize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		ubsize.descriptorCount = 5;

		smsize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		smsize.descriptorCount = 10; // reserved number of descriptor for this type in this pool: we're using 4 sets of DYNAMIC UB for each command buffers and 1 set of sampler, so if we want to use command buffers like this, it'll be better to use another pool for this one
		// because we allocate each types simultaneously, not respectively

		VkDescriptorPoolCreateInfo dpinfo{};
		dpinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		dpinfo.poolSizeCount = sizeof(sizes) / sizeof(sizes[0]);
		dpinfo.pPoolSizes = sizes;
		dpinfo.maxSets = COMMANDBUFFER_COUNT * 2;

		if (vkCreateDescriptorPool(device, &dpinfo, nullptr, &ubpool) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create descriptor pool\n");
			return false;
		}

		std::vector<VkDescriptorSetLayout> layouts(COMMANDBUFFER_COUNT, ubds);
		VkDescriptorSetAllocateInfo setInfo{};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setInfo.descriptorPool = ubpool;
		setInfo.descriptorSetCount = COMMANDBUFFER_COUNT;
		setInfo.pSetLayouts = layouts.data();

		if (vkAllocateDescriptorSets(device, &setInfo, ubset) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate descriptor set\n");
			return false;
		}
		VkDeviceSize dynamicAlignment = minUniformBufferOffset;
		for (size_t i = 0; i < COMMANDBUFFER_COUNT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = ub[i];
			bufferInfo.offset = 0;
			bufferInfo.range = dynamicAlignment;

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = ubset[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		}

		std::vector<VkDescriptorSetLayout> samplerLayouts(sizeof(samplerSet) / sizeof(samplerSet[0]), ubds);
		setInfo.descriptorSetCount = samplerLayouts.size();
		setInfo.pSetLayouts = samplerLayouts.data();
		if (vkAllocateDescriptorSets(device, &setInfo, samplerSet) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate descriptor set for sampler\n");
			return false;
		}
		for (size_t i = 0; i < sizeof(samplerSet) / sizeof(samplerSet[0]); i++) {
			VkDescriptorImageInfo imageInfo[2]={};
			imageInfo[0].imageView = texview0;
			imageInfo[0].sampler = sampler0;
			imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo[1].imageView = texview1;
			imageInfo[1].sampler = sampler0;
			imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = samplerSet[i];
			descriptorWrite.dstBinding = 1;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 2;
			descriptorWrite.pImageInfo = imageInfo;
			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		}

		VkDescriptorSetLayoutBinding sp1binding{};
		sp1binding.binding = 0;
		sp1binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		sp1binding.descriptorCount = 1;
		sp1binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		info.bindingCount = 1;
		info.pBindings = &sp1binding;
		if (vkCreateDescriptorSetLayout(device, &info, nullptr, &sp1layout) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create descriptor set layout for input attachment\n");
			return false;
		}

		VkDescriptorPoolSize sp1poolSize{};
		sp1poolSize.descriptorCount = 1;
		sp1poolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

		dpinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		dpinfo.poolSizeCount = 1;
		dpinfo.pPoolSizes = &sp1poolSize;
		dpinfo.maxSets = 1;

		if (vkCreateDescriptorPool(device, &dpinfo, nullptr, &sp1pool) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create descriptor pool for input attachment\n");
			return false;
		}
		setInfo.descriptorPool = sp1pool;
		setInfo.descriptorSetCount = 1;
		setInfo.pSetLayouts = &sp1layout;
		if (vkAllocateDescriptorSets(device, &setInfo, &sp1set) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate descriptor set for input attachment\n");
			return false;
		}
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = middleImageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = sp1set;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

		return true;
	}

	void VkPlayer::destroyDescriptorSet() {
		vkDestroyDescriptorPool(device, sp1pool, nullptr);
		vkDestroyDescriptorSetLayout(device, sp1layout, nullptr);
		vkDestroyDescriptorPool(device, ubpool, nullptr);
		vkDestroyDescriptorSetLayout(device, ubds, nullptr);
	}

	bool VkPlayer::createFixedIndexBuffer() {
		uint16_t ar[] = { 0,1,2,0,2,3 };

		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		info.size = sizeof(ar);
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer ib;
		VkDeviceMemory ibmem;

		if (vkCreateBuffer(device, &info, nullptr, &ib) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create fixed index buffer\n");
			return false;
		}

		VkMemoryRequirements mreq;
		vkGetBufferMemoryRequirements(device, ib, &mreq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice.card);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &ibmem) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate memory for fixed ib\n");
			return false;
		}
		void* data;
		if (vkMapMemory(device, ibmem, 0, info.size, 0, &data) != VK_SUCCESS) {
			fprintf(stderr, "Failed to map to allocated memory\n");
			return false;
		}
		memcpy(data, ar, info.size);
		vkUnmapMemory(device, ibmem);
		if (vkBindBufferMemory(device, ib, ibmem, 0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to bind buffer object and memory\n");
			return false;
		}

		info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (vkCreateBuffer(device, &info, nullptr, &VkPlayer::ib) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create fixed index buffer\n");
			return false;
		}
		vkGetBufferMemoryRequirements(device, VkPlayer::ib, &mreq);
		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice.card);
		if (vkAllocateMemory(device, &allocInfo, nullptr, &VkPlayer::ibmem) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate memory for fixed ib\n");
			return false;
		}
		if (vkBindBufferMemory(device, VkPlayer::ib, VkPlayer::ibmem, 0) != VK_SUCCESS) {
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
			fprintf(stderr, "Failed to allocate command buffer for copying index buffer\n");
			return false;
		}
		VkCommandBufferBeginInfo copyBegin{};
		copyBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		copyBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(copyBuffer, &copyBegin) != VK_SUCCESS) {
			fprintf(stderr, "Failed to begin command buffer for copying index buffer\n");
			return false;
		}
		VkBufferCopy copyRegion{};
		copyRegion.size = sizeof(ar); // ������ ���� ����
		vkCmdCopyBuffer(copyBuffer, ib, VkPlayer::ib, 1, &copyRegion);
		if (vkEndCommandBuffer(copyBuffer) != VK_SUCCESS) {
			fprintf(stderr, "Failed to end command buffer for copying index buffer\n");
			return false;
		}
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyBuffer;
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			fprintf(stderr, "Failed to submit command buffer for copying index buffer\n");
			return false;
		}
		vkQueueWaitIdle(graphicsQueue);
		vkFreeCommandBuffers(device, commandPool, 1, &copyBuffer);
		vkDestroyBuffer(device, ib, nullptr);
		vkFreeMemory(device, ibmem, nullptr);
		return true;
	}

	void VkPlayer::destroyFixedIndexBuffer() {
		vkFreeMemory(device, ibmem, nullptr);
		vkDestroyBuffer(device, ib, nullptr);
	}

	bool VkPlayer::createDSBuffer() {
		VkFormat imgFormat = VK_FORMAT_D32_SFLOAT;
		VkImageTiling imgTiling = VK_IMAGE_TILING_LINEAR;
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice.card, VK_FORMAT_D24_UNORM_S8_UINT, &props);
		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			imgFormat = VK_FORMAT_D24_UNORM_S8_UINT;
			imgTiling = VK_IMAGE_TILING_OPTIMAL;
		}
		else if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			imgFormat = VK_FORMAT_D24_UNORM_S8_UINT;
			imgTiling = VK_IMAGE_TILING_LINEAR;
		}

		VkImageCreateInfo imgInfo{};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.extent.width = swapchainExtent.width;
		imgInfo.extent.height = swapchainExtent.height;
		imgInfo.extent.depth = 1;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imgInfo.mipLevels = 1;
		imgInfo.arrayLayers = 1;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.format = imgFormat;
		imgInfo.tiling = imgTiling;
		if (vkCreateImage(device, &imgInfo, nullptr, &dsImage) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create depth/stencil buffer image\n");
			return false;
		}

		VkMemoryRequirements mreq;
		vkGetImageMemoryRequirements(device, dsImage, &mreq);
		VkMemoryAllocateInfo memInfo{};
		memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memInfo.allocationSize = mreq.size;
		memInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice.card);
		if (vkAllocateMemory(device, &memInfo, nullptr, &dsmem) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate depth/stencil buffer image memory\n");
			return false;
		}
		if (vkBindImageMemory(device, dsImage, dsmem, 0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to bind depth/stencil buffer image - memory\n");
			return false;
		}
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = dsImage;
		viewInfo.format = imgInfo.format;
		viewInfo.components.r = viewInfo.components.g = viewInfo.components.b = viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		if (vkCreateImageView(device, &viewInfo, nullptr, &dsImageView) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create depth/stencil buffer image view\n");
			return false;
		}
		dsImageFormat = imgInfo.format;

		// �� �� ÷�ι�
		imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		imgInfo.format = swapchainImageFormat;
		if (vkCreateImage(device, &imgInfo, nullptr, &middleImage) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create intermediate image\n");
			return false;
		}
		vkGetImageMemoryRequirements(device, middleImage, &mreq);
		memInfo.allocationSize = mreq.size;
		memInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice.card);
		if (vkAllocateMemory(device, &memInfo, nullptr, &middleMem) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate intermediate image memory\n");
			return false;
		}
		if (vkBindImageMemory(device, middleImage, middleMem, 0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to bind intermediate image - memory\n");
			return false;
		}
		viewInfo.image = middleImage;
		viewInfo.format = imgInfo.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		if (vkCreateImageView(device, &viewInfo, nullptr, &middleImageView) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create intermediate image view\n");
			return false;
		}

		return true;
	}

	void VkPlayer::destroyDSBuffer() {
		vkDestroyImageView(device, middleImageView, nullptr);
		vkDestroyImageView(device, dsImageView, nullptr);
		vkFreeMemory(device, middleMem, nullptr);
		vkFreeMemory(device, dsmem, nullptr);
		vkDestroyImage(device, middleImage, nullptr);
		vkDestroyImage(device, dsImage, nullptr);
	}

	inline static unsigned char* readImageData(const unsigned char* data, size_t len, int* width, int* height, int* nChannels) {
		unsigned char* pix = stbi_load_from_memory(data, len, width, height, nChannels, 4);
		return pix;
	}

	inline static unsigned char* readImageFile(const char* file, int* width, int* height, int* nChannels) {
		unsigned char* pix = stbi_load(file, width, height, nChannels, 4);
		return pix;
	}

	bool VkPlayer::createTex0() {
		int w, h, ch;
		unsigned char* pix = readImageFile("no1.png", &w, &h, &ch);
		if (!pix) {
			fprintf(stderr, "Failed to read image file\n");
			return false;
		}

		VkImageCreateInfo imgInfo{};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.extent.width = (uint32_t)w;
		imgInfo.extent.height = (uint32_t)h;
		imgInfo.extent.depth = 1;
		imgInfo.mipLevels = 1;
		imgInfo.arrayLayers = 1;
		imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		if (vkCreateImage(device, &imgInfo, nullptr, &tex0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create texture image object\n");
			free(pix);
			return false;
		}

		VkMemoryRequirements mreq;
		vkGetImageMemoryRequirements(device, tex0, &mreq);
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice.card);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &texmem0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate memory for texture image\n");
			free(pix);
			return false;
		}
		if (vkBindImageMemory(device, tex0, texmem0, 0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to bind texture image and device memory\n");
			free(pix);
			return false;
		}

		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		info.size = static_cast<VkDeviceSize>(w) * h * 4;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer temp;
		VkDeviceMemory tempMem;
		if (vkCreateBuffer(device, &info, nullptr, &temp) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create temporary buffer for texture image\n");
			free(pix);
			return false;
		}

		vkGetBufferMemoryRequirements(device, temp, &mreq);

		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice.card);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &tempMem) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate memory for temporary buffer for texture image\n");
			free(pix);
			return false;
		}
		void* data;
		if (vkMapMemory(device, tempMem, 0, info.size, 0, &data) != VK_SUCCESS) {
			fprintf(stderr, "Failed to map to allocated memory\n");
			free(pix);
			return false;
		}
		memcpy(data, pix, info.size);
		free(pix);
		vkUnmapMemory(device, tempMem);
		if (vkBindBufferMemory(device, temp, tempMem, 0) != VK_SUCCESS) {
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
			fprintf(stderr, "Failed to allocate command buffer for copying texture image\n");
			return false;
		}
		VkCommandBufferBeginInfo copyBegin{};
		copyBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		copyBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(copyBuffer, &copyBegin) != VK_SUCCESS) {
			fprintf(stderr, "Failed to begin command buffer for copying texture image\n");
			return false;
		}

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = tex0;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkCmdPipelineBarrier(copyBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkBufferImageCopy copyRegion{};
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageOffset = { 0,0,0 };
		copyRegion.imageExtent = imgInfo.extent;

		vkCmdCopyBufferToImage(copyBuffer, temp, tex0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(copyBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyBuffer;
		if (vkEndCommandBuffer(copyBuffer) != VK_SUCCESS) {
			fprintf(stderr, "Failed to end command buffer for copying texture data\n");
			return false;
		}
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			fprintf(stderr, "Failed to submit command buffer for copying vertex buffer\n");
			return false;
		}
		vkQueueWaitIdle(graphicsQueue);
		vkDestroyBuffer(device, temp, nullptr);
		vkFreeMemory(device, tempMem, nullptr);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = tex0;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY ,VK_COMPONENT_SWIZZLE_IDENTITY ,VK_COMPONENT_SWIZZLE_IDENTITY };

		if (vkCreateImageView(device, &viewInfo, nullptr, &texview0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create image view for texture\n");
			return false;
		}

		// 2��°
		pix = readImageFile("no2.png", &w, &h, &ch);
		if (!pix) {
			fprintf(stderr, "Failed to read image file\n");
			return false;
		}

		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.extent.width = (uint32_t)w;
		imgInfo.extent.height = (uint32_t)h;
		imgInfo.extent.depth = 1;
		imgInfo.mipLevels = 1;
		imgInfo.arrayLayers = 1;
		imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		if (vkCreateImage(device, &imgInfo, nullptr, &tex1) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create texture image object\n");
			free(pix);
			return false;
		}

		vkGetImageMemoryRequirements(device, tex1, &mreq);
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice.card);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &texmem1) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate memory for texture image\n");
			free(pix);
			return false;
		}
		if (vkBindImageMemory(device, tex1, texmem1, 0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to bind texture image and device memory\n");
			free(pix);
			return false;
		}

		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		info.size = static_cast<VkDeviceSize>(w) * h * 4;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &info, nullptr, &temp) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create temporary buffer for texture image\n");
			free(pix);
			return false;
		}

		vkGetBufferMemoryRequirements(device, temp, &mreq);

		allocInfo.allocationSize = mreq.size;
		allocInfo.memoryTypeIndex = findMemorytype(mreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice.card);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &tempMem) != VK_SUCCESS) {
			fprintf(stderr, "Failed to allocate memory for temporary buffer for texture image\n");
			free(pix);
			return false;
		}

		if (vkMapMemory(device, tempMem, 0, info.size, 0, &data) != VK_SUCCESS) {
			fprintf(stderr, "Failed to map to allocated memory\n");
			free(pix);
			return false;
		}
		memcpy(data, pix, info.size);
		free(pix);
		vkUnmapMemory(device, tempMem);
		if (vkBindBufferMemory(device, temp, tempMem, 0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to bind buffer object and memory\n");
			return false;
		}

		copyBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		copyBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(copyBuffer, &copyBegin) != VK_SUCCESS) {
			fprintf(stderr, "Failed to begin command buffer for copying texture image\n");
			return false;
		}

		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = tex1;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkCmdPipelineBarrier(copyBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageOffset = { 0,0,0 };
		copyRegion.imageExtent = imgInfo.extent;

		vkCmdCopyBufferToImage(copyBuffer, temp, tex1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(copyBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyBuffer;
		if (vkEndCommandBuffer(copyBuffer) != VK_SUCCESS) {
			fprintf(stderr, "Failed to end command buffer for copying texture data\n");
			return false;
		}
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			fprintf(stderr, "Failed to submit command buffer for copying vertex buffer\n");
			return false;
		}
		vkQueueWaitIdle(graphicsQueue);
		vkFreeCommandBuffers(device, commandPool, 1, &copyBuffer);
		vkDestroyBuffer(device, temp, nullptr);
		vkFreeMemory(device, tempMem, nullptr);

		viewInfo.image = tex1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &texview1) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create image view for texture\n");
			return false;
		}

		return true;
	}

	void VkPlayer::destroyTex0() {
		vkDestroyImageView(device, texview0, nullptr);
		vkDestroyImageView(device, texview1, nullptr);
		vkFreeMemory(device, texmem0, nullptr);
		vkFreeMemory(device, texmem1, nullptr);
		vkDestroyImage(device, tex1, nullptr);
		vkDestroyImage(device, tex0, nullptr);
	}

	bool VkPlayer::createSampler0() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler0) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create sampler\n");
			return false;
		}
		return true;
	}

	void VkPlayer::destroySampler0() {
		vkDestroySampler(device, sampler0, nullptr);
	}

	bool VkPlayer::createPipeline1() {
		// programmable function: ���̴� ������ ���� ������ �� ���̹Ƿ�, �����ε�� ������ ��
		VkShaderModule vertModule = createShaderModule("post.vert", shaderc_shader_kind::shaderc_glsl_vertex_shader);
		VkShaderModule fragModule = createShaderModule("post.frag", shaderc_shader_kind::shaderc_glsl_fragment_shader);
		VkPipelineShaderStageCreateInfo shaderStagesInfo[2] = {};
		shaderStagesInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStagesInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStagesInfo[0].module = vertModule;
		shaderStagesInfo[0].pName = "main";

		shaderStagesInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStagesInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStagesInfo[1].module = fragModule;
		shaderStagesInfo[1].pName = "main";

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// 3���� ���� �ﰢ��
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;	// 0xffff Ȥ�� 0xffffffff �ε����� ��Ʈ�� ���� ���� ����

		// fixed function: ����Ʈ, ���� (����Ʈ: cvv�� �׸��� �׷��� ���� ���簢��, ����: ����ü�� �̹������� �׷����� ���� ����� �κ�)
		// ** ����Ʈ�� ��Ÿ�ӿ� ���� ���� ** 
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

		// fixed function: �����Ͷ�����
		VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
		rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerInfo.depthClampEnable = VK_FALSE;	// ǥ�� �� ������ z��ǥ�� �ʰ��ϸ� �߶��� �ʰ� z�� ��ü�� ���̸鼭 �츲
		rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;	// TRUE�� ��� ����� �ȵ�
		rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;	// ���̾������� �� ���� ����. �Ƹ� �ǵ帱 �� ���� ��
		rasterizerInfo.lineWidth = 1.0f;					// �� �ʺ�: �ǵ帱 �� ����
		rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;	// �� �Ÿ�: �ǵ帱 �� ����
		rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// �̰� �ǵ帱 �� ���� ��
		rasterizerInfo.depthBiasEnable = VK_FALSE;			// ������� 4���� ���� ���� ���� �ǵ帮�� �κ�. ����� �� ���� ��
		rasterizerInfo.depthBiasConstantFactor = 0.0f;
		rasterizerInfo.depthBiasClamp = 0.0f;
		rasterizerInfo.depthBiasSlopeFactor = 0.0f;

		// fixed function: ��Ƽ���ø� (����� ��������)
		// ��Ƽ���ϸ���̿� ����ϴ� ��쿡 ���Ͽ� �Ű����� ������ �ʿ䰡 ������
		VkPipelineMultisampleStateCreateInfo msInfo{};
		msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		msInfo.sampleShadingEnable = VK_FALSE;
		msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		msInfo.minSampleShading = 1.0f;
		msInfo.alphaToCoverageEnable = VK_FALSE;
		msInfo.alphaToOneEnable = VK_FALSE;
		msInfo.pSampleMask = nullptr;

		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		// fixed function: �� ������
		// �������� ��ü�� SRC_ALPHA, 1-SRC_ALPHA�� �ϴ� �������� ���� �Ŷ�� ������ ���� ���� ����?
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

		VkDynamicState dynamicStates[1] = { VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamics{};
		dynamics.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamics.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
		dynamics.pDynamicStates = dynamicStates;

		VkPushConstantRange pushRange{};
		pushRange.offset = 0;
		pushRange.size = 16;
		pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// fixed function: ���������� ���̾ƿ�: uniform �� push ������ ���� ��
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &sp1layout;
		vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout1);

		// ���������� ����
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = sizeof(shaderStagesInfo) / sizeof(shaderStagesInfo[0]);
		pipelineInfo.pStages = shaderStagesInfo;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasterizerInfo;
		pipelineInfo.pMultisampleState = &msInfo;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pColorBlendState = &colorBlendInfo;
		pipelineInfo.pDynamicState = &dynamics;
		pipelineInfo.layout = pipelineLayout1;
		pipelineInfo.renderPass = renderPass0;
		pipelineInfo.subpass = 1;
		// �Ʒ� 2��: ���� ������������ ������� ����ϰ� �����ϱ� ����
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline1);

		vkDestroyShaderModule(device, vertModule, nullptr);
		vkDestroyShaderModule(device, fragModule, nullptr);
		if (result != VK_SUCCESS) {
			fprintf(stderr, "Failed to create pipeline 0\n");
			return false;
		}
		return true;
	}

	void VkPlayer::destroyPipeline1() {
		vkDestroyPipelineLayout(device, pipelineLayout1, nullptr);
		vkDestroyPipeline(device, pipeline1, nullptr);
	}

	bool VkPlayer::initAllocator() {
		VmaAllocatorCreateInfo info{};
		info.instance = instance;
		info.device = device;
		info.physicalDevice = physicalDevice.card;
		info.vulkanApiVersion = VK_API_VERSION_1_0;
		if (vmaCreateAllocator(&info, &allocator) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create VMA allocator\n");
			return false;
		}
		return true;
	}

	void VkPlayer::destroyAllocator() {
		vmaDestroyAllocator(allocator);
	}
}