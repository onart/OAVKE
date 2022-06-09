#include "VkPlayer.h"
#include <filesystem>

int main(int argc, char* argv[]) {
	std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());
	onart::VkPlayer::start();
	return 0;
}
