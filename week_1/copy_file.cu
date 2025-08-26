#include <iostream>
#include <filesystem>
#include <cuda_runtime.h>

__global__ void noop() {}

int main(int argc, char** argv) {
    // Demonstrate CUDA context (will report if not available)
    noop<<<1,1>>>();
    cudaError_t err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        std::cerr << "CUDA not available or error: " << cudaGetErrorString(err) << "\n";
    }

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
