# Production I/O + GPU Overlap Demo Makefile

BINDIR := bin
CFLAGS := -O2 -Wall -Wextra -Wpedantic -std=c11
CXXFLAGS := -O2 -Wall -Wextra -Wpedantic -std=c++17
LDFLAGS :=
LIBS :=

# Enable Linux io_uring backend with: make IO_URING=1
ifeq ($(IO_URING),1)
CFLAGS += -DUSE_IO_URING
LIBS += -luring
endif

# Detect platform for clock_gettime fallback on macOS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
CFLAGS += -D_DARWIN_C_SOURCE
endif

# Targets
CPU_DEMO := $(BINDIR)/cpu_io_demo
GPU_FALLBACK := $(BINDIR)/gpu_pipeline_fallback
GPU_CUDA := $(BINDIR)/gpu_pipeline_cuda

.PHONY: all gpu clean dirs

all: dirs $(CPU_DEMO) $(GPU_FALLBACK)

cpu: dirs $(CPU_DEMO)

gpu: dirs $(GPU_CUDA)

$(BINDIR):
	@mkdir -p $(BINDIR)

dirs: $(BINDIR)

$(CPU_DEMO): cpu_io_demo.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(GPU_FALLBACK): gpu_pipeline_fallback.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build CUDA demo if nvcc exists
NVCC := $(shell command -v nvcc 2>/dev/null)
$(GPU_CUDA): gpu_pipeline_cuda.cu
ifndef NVCC
	@echo "nvcc not found; install CUDA or set PATH."
	@false
else
	nvcc -O2 -std=c++17 -Xcompiler -Wall,-Wextra,-Wpedantic -o $@ $^
endif

clean:
	rm -rf $(BINDIR)
