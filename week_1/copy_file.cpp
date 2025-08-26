#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <source> <destination>\n";
        return 2;
    }

    try {
        std::filesystem::copy_file(
            argv[1],
            argv[2],
            std::filesystem::copy_options::overwrite_existing
        );
    } catch (const std::exception& e) {
        std::cerr << "Copy failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
