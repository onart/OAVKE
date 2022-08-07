TARGET = ../bin/OAVKE.out

${TARGET}: main.o vkp.o
	g++ main.o vkp.o -L../bin/ -Lexternals/glfw/ -Lexternals/vulkan/ -lvulkan -lshaderc_shared -lglfw3 -lX11 -o ${TARGET} -Wl,-rpath,.
main.o: main.cpp
	g++ -c main.cpp -o main.o
vkp.o: VkPlayer.cpp VkPlayer.h
	g++ -c VkPlayer.cpp -o vkp.o
cleano:
	rm main.o vkp.o
clean:
	make cleano
	rm ${TARGET}