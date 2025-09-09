// gpu_pipeline_cuda.cu
// Triple-buffered CUDA pipeline demonstrating overlap of H2D, compute, and D2H using streams and events.
// Build: make -C src/production_io_demo gpu

#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>

#ifndef CHECK_CUDA
#define CHECK_CUDA(x) do { cudaError_t err__ = (x); if (err__ != cudaSuccess) { \
  std::fprintf(stderr, "CUDA error %s at %s:%d -> %s\n", #x, __FILE__, __LINE__, cudaGetErrorString(err__)); \
  std::exit(1); } } while(0)
#endif

__global__ void axpby(const float* __restrict__ x, float* __restrict__ y, float a, float b, size_t n) {
  size_t i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n) y[i] = a * x[i] + b;
}

int main(int argc, char** argv) {
  size_t total_elems   = (argc > 1) ? std::strtoull(argv[1], nullptr, 10) : (size_t)100000000; // 100M
  size_t chunk_elems   = (argc > 2) ? std::strtoull(argv[2], nullptr, 10) : (size_t)1 << 20;   // 1M
  int    depth         = (argc > 3) ? std::atoi(argv[3]) : 3; // buffering depth
  if (depth < 2) depth = 2; if (depth > 16) depth = 16;

  size_t total_bytes = total_elems * sizeof(float);
  size_t chunk_bytes = chunk_elems * sizeof(float);
  size_t chunks = (total_elems + chunk_elems - 1) / chunk_elems;

  std::printf("CUDA pipeline: total %.3f MiB, chunk %.3f MiB, chunks %zu, depth %d\n",
              total_bytes / 1048576.0, chunk_bytes / 1048576.0, chunks, depth);

  // Streams for copy/compute/copy-back
  cudaStream_t sH2D, sComp, sD2H;
  CHECK_CUDA(cudaStreamCreateWithFlags(&sH2D, cudaStreamNonBlocking));
  CHECK_CUDA(cudaStreamCreateWithFlags(&sComp, cudaStreamNonBlocking));
  CHECK_CUDA(cudaStreamCreateWithFlags(&sD2H, cudaStreamNonBlocking));

  // Pinned buffers and device buffers per slot
  std::vector<float*> hIn(depth, nullptr), hOut(depth, nullptr);
  std::vector<float*> dIn(depth, nullptr), dOut(depth, nullptr);
  std::vector<cudaEvent_t> evH2D(depth), evComp(depth), evD2H(depth);

  for (int i = 0; i < depth; ++i) {
    CHECK_CUDA(cudaMallocHost(&hIn[i], chunk_bytes));
    CHECK_CUDA(cudaMallocHost(&hOut[i], chunk_bytes));
    CHECK_CUDA(cudaMalloc(&dIn[i],  chunk_bytes));
    CHECK_CUDA(cudaMalloc(&dOut[i], chunk_bytes));
    CHECK_CUDA(cudaEventCreateWithFlags(&evH2D[i], cudaEventDisableTiming));
    CHECK_CUDA(cudaEventCreateWithFlags(&evComp[i], cudaEventDisableTiming));
    CHECK_CUDA(cudaEventCreateWithFlags(&evD2H[i], cudaEventDisableTiming));
  }

  // Kernel config
  const float a = 2.0f, b = 1.0f;
  const int block = 256;

  cudaEvent_t tStart, tStop; CHECK_CUDA(cudaEventCreate(&tStart)); CHECK_CUDA(cudaEventCreate(&tStop));
  CHECK_CUDA(cudaEventRecord(tStart));

  size_t produced = 0;
  for (size_t c = 0; c < chunks; ++c) {
    int slot = (int)(c % depth);
    // Ensure prior use of this slot finished
    if (c >= (size_t)depth) CHECK_CUDA(cudaEventSynchronize(evD2H[slot]));

    // Fill host input for this chunk
    size_t n = std::min(chunk_elems, total_elems - produced);
    for (size_t i = 0; i < n; ++i) hIn[slot][i] = (float)(produced + i) * 0.001f;

    // H2D
    CHECK_CUDA(cudaMemcpyAsync(dIn[slot], hIn[slot], n * sizeof(float), cudaMemcpyHostToDevice, sH2D));
    CHECK_CUDA(cudaEventRecord(evH2D[slot], sH2D));

    // Compute waits on H2D
    CHECK_CUDA(cudaStreamWaitEvent(sComp, evH2D[slot], 0));
    int grid = (int)((n + block - 1) / block);
    axpby<<<grid, block, 0, sComp>>>(dIn[slot], dOut[slot], a, b, n);
    CHECK_CUDA(cudaGetLastError());
    CHECK_CUDA(cudaEventRecord(evComp[slot], sComp));

    // D2H waits on compute
    CHECK_CUDA(cudaStreamWaitEvent(sD2H, evComp[slot], 0));
    CHECK_CUDA(cudaMemcpyAsync(hOut[slot], dOut[slot], n * sizeof(float), cudaMemcpyDeviceToHost, sD2H));
    CHECK_CUDA(cudaEventRecord(evD2H[slot], sD2H));

    produced += n;
  }

  // Drain
  CHECK_CUDA(cudaEventSynchronize(evD2H[(chunks-1)%depth]));
  CHECK_CUDA(cudaStreamSynchronize(sH2D));
  CHECK_CUDA(cudaStreamSynchronize(sComp));
  CHECK_CUDA(cudaStreamSynchronize(sD2H));
  CHECK_CUDA(cudaEventRecord(tStop));
  CHECK_CUDA(cudaEventSynchronize(tStop));

  float ms = 0.0f; CHECK_CUDA(cudaEventElapsedTime(&ms, tStart, tStop));
  double sec = ms / 1000.0;
  double mib = total_bytes / 1048576.0;
  std::printf("Done in %.3f s (%.2f MiB/s)\n", sec, (sec>0? mib/sec:0));

  // Spot-check results for last slot
  int last = (int)((chunks-1) % depth);
  bool ok = true; size_t n = std::min(chunk_elems, total_elems - (chunks-1)*chunk_elems);
  for (size_t i = 0; i < std::min(n,(size_t)10); ++i) {
    float x = (float)(((chunks-1)*chunk_elems) + i) * 0.001f;
    float exp = a * x + b;
    if (std::abs(hOut[last][i] - exp) > 1e-4f) { ok = false; break; }
  }
  std::printf("Validation: %s\n", ok ? "OK" : "MISMATCH");

  for (int i = 0; i < depth; ++i) {
    cudaEventDestroy(evH2D[i]); cudaEventDestroy(evComp[i]); cudaEventDestroy(evD2H[i]);
    cudaFree(dIn[i]); cudaFree(dOut[i]);
    cudaFreeHost(hIn[i]); cudaFreeHost(hOut[i]);
  }
  cudaEventDestroy(tStart); cudaEventDestroy(tStop);
  cudaStreamDestroy(sH2D); cudaStreamDestroy(sComp); cudaStreamDestroy(sD2H);
  return ok ? 0 : 1;
}
