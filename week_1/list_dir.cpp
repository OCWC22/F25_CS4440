#include <iostream>
#include <filesystem>

int main() {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            std::cout << entry.path().filename().string() << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing directory: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
