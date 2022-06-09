/*
MIT License
Copyright (c) 2022 onart
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef __VK_PLAYER_H__
#define __VK_PLAYER_H__

#define GLFW_INCLUDE_VULKAN
#include "externals/glfw/glfw3.h"

namespace onart {
	class VkPlayer {
	public:
		// Vulkan�� �ʱ�ȭ�ϰ�, �����մϴ�.
		static void start();
		// ��� ���� ���������� �����ϰ� �����մϴ�. â�� ���� �Ͱ� ���� �����Դϴ�.
		static void exit();
	private:	// �Լ�
		// GLFW, Vulkan�� ����Ͽ� �ʿ��� ��� ���� �ʱ�ȭ�մϴ�.
		static bool init();
		// ������Ʈ, ������ ����� �����մϴ�.
		static void mainLoop();
		// ���� �� �Ҵ��� ��� �ڿ��� �����մϴ�.
		static void finalize();
		// vulkan �ν��Ͻ��� �����մϴ�.
		static bool createInstance();
		// vulkan �ν��Ͻ��� �����մϴ�.
		static void destroyInstance();
	private:	// ���
		static VkInstance instance;
	private:	// ���
		constexpr static bool USE_VALIDATION_LAYER = true;
		constexpr static const char* VALIDATION_LAYERS[] = { "VK_LAYER_KHRONOS_validation" };
		constexpr static const int VALIDATION_LAYER_COUNT = sizeof(VALIDATION_LAYERS) / sizeof(const char*);
	};
}

#endif // !__VK_PLAYER_H__

