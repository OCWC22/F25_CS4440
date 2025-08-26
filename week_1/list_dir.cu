#include <iostream>
#include <filesystem>
#include <cuda_runtime.h>

__global__ void noop() {}

int main() {
    // Demonstrate CUDA context (will report if not available)
    noop<<<1,1>>>();
    cudaError_t err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        std::cerr << "CUDA not available or error: " << cudaGetErrorString(err) << "\n";
    }

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
