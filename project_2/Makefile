# CS4440 Project 2 Makefile
# Builds all three concurrency problems with hardware-aware optimizations

# Compiler and flags
CC = gcc
# Note: -Wno-deprecated-declarations for macOS semaphore compatibility
CFLAGS = -std=c11 -Wall -Wextra -Werror -Wno-deprecated-declarations -pedantic -O2 -pthread
LDFLAGS = -pthread

# Targets
TARGETS = bounded_buffer mh airline

# Default target: build all programs
all: $(TARGETS)

# Problem 1: Bounded-Buffer Producer/Consumer
bounded_buffer: bounded_buffer.c
	$(CC) $(CFLAGS) -o bounded_buffer bounded_buffer.c $(LDFLAGS)

# Problem 2: Mother Hubbard
mh: mh.c
	$(CC) $(CFLAGS) -o mh mh.c $(LDFLAGS)

# Problem 3: Airline Passengers
airline: airline.c
	$(CC) $(CFLAGS) -o airline airline.c $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(TARGETS) *.o

# Run tests for all programs
test: all
	@echo "=== Testing Bounded Buffer ==="
	./bounded_buffer
	@echo ""
	@echo "=== Testing Mother Hubbard (3 cycles) ==="
	./mh 3
	@echo ""
	@echo "=== Testing Airline (10 passengers, 2 handlers, 2 screeners, 1 attendant) ==="
	./airline 10 2 2 1

# Individual test targets
test_bounded_buffer: bounded_buffer
	./bounded_buffer

test_mh: mh
	./mh 5

test_airline: airline
	./airline 20 3 4 2

# Help target
help:
	@echo "CS4440 Project 2 - Concurrency and Synchronization"
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build all programs (default)"
	@echo "  bounded_buffer   - Build Problem 1"
	@echo "  mh               - Build Problem 2"
	@echo "  airline          - Build Problem 3"
	@echo "  test             - Run all tests"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Individual test targets:"
	@echo "  test_bounded_buffer"
	@echo "  test_mh"
	@echo "  test_airline"

.PHONY: all clean test test_bounded_buffer test_mh test_airline help
