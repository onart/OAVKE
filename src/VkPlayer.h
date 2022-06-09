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
		// Vulkan을 초기화하고, 시작합니다.
		static void start();
		// 모든 것을 정상적으로 리턴하고 종료합니다. 창을 닫은 것과 같은 동작입니다.
		static void exit();
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
	private:	// 멤버
		static VkInstance instance;
	};
}

#endif // !__VK_PLAYER_H__

