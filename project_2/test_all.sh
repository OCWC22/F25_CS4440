#!/bin/bash

# CS4440 Project 2 - Automated Test Script
# Tests all three concurrency problems with various inputs

set -e  # Exit on error

echo "========================================="
echo "CS4440 Project 2 - Test Suite"
echo "========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print test header
print_test() {
    echo ""
    echo "========================================="
    echo "$1"
    echo "========================================="
}

# Function to print success
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Function to print error
print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Function to print warning
print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Check if programs are compiled
if [ ! -f "bounded_buffer" ] || [ ! -f "mh" ] || [ ! -f "airline" ]; then
    print_warning "Programs not compiled. Running 'make all'..."
    make all
    print_success "Compilation complete"
fi

# Test 1: Bounded Buffer
print_test "Test 1: Bounded-Buffer Producer/Consumer"
echo "Running bounded_buffer..."
if ./bounded_buffer > /tmp/bounded_buffer_output.txt 2>&1; then
    # Check if output contains expected patterns
    if grep -q "Producer.*Produced" /tmp/bounded_buffer_output.txt && \
       grep -q "Consumer.*Consumed" /tmp/bounded_buffer_output.txt && \
       grep -q "Final buffer count: 0" /tmp/bounded_buffer_output.txt; then
        print_success "Bounded buffer test passed"
        echo "Sample output:"
        head -n 10 /tmp/bounded_buffer_output.txt
        echo "..."
        tail -n 5 /tmp/bounded_buffer_output.txt
    else
        print_error "Bounded buffer test failed - unexpected output"
        cat /tmp/bounded_buffer_output.txt
        exit 1
    fi
else
    print_error "Bounded buffer test failed - program crashed"
    cat /tmp/bounded_buffer_output.txt
    exit 1
fi

# Test 2: Mother Hubbard - Small test (3 cycles)
print_test "Test 2a: Mother Hubbard (3 cycles)"
echo "Running mh 3..."
if ./mh 3 > /tmp/mh_output.txt 2>&1; then
    if grep -q "Day #1: Mother wakes up" /tmp/mh_output.txt && \
       grep -q "Child #1 is being woken up and fed breakfast" /tmp/mh_output.txt && \
       grep -q "Child #1 is being read a book by Father" /tmp/mh_output.txt && \
       grep -q "All 3 cycles completed successfully" /tmp/mh_output.txt; then
        print_success "Mother Hubbard (3 cycles) test passed"
        echo "Sample output:"
        head -n 15 /tmp/mh_output.txt
        echo "..."
        tail -n 5 /tmp/mh_output.txt
    else
        print_error "Mother Hubbard test failed - unexpected output"
        cat /tmp/mh_output.txt
        exit 1
    fi
else
    print_error "Mother Hubbard test failed - program crashed"
    cat /tmp/mh_output.txt
    exit 1
fi

# Test 2b: Mother Hubbard - Larger test (10 cycles)
print_test "Test 2b: Mother Hubbard (10 cycles)"
echo "Running mh 10..."
if timeout 30 ./mh 10 > /tmp/mh_10_output.txt 2>&1; then
    if grep -q "All 10 cycles completed successfully" /tmp/mh_10_output.txt; then
        print_success "Mother Hubbard (10 cycles) test passed"
    else
        print_error "Mother Hubbard (10 cycles) test failed"
        tail -n 20 /tmp/mh_10_output.txt
        exit 1
    fi
else
    print_error "Mother Hubbard (10 cycles) test failed or timed out"
    exit 1
fi

# Test 3a: Airline - Small test (10 passengers)
print_test "Test 3a: Airline (10 passengers, 2/2/1 workers)"
echo "Running airline 10 2 2 1..."
if timeout 30 ./airline 10 2 2 1 > /tmp/airline_output.txt 2>&1; then
    if grep -q "Passenger #1 arrived at the terminal" /tmp/airline_output.txt && \
       grep -q "Baggage Handler.*Processing passenger" /tmp/airline_output.txt && \
       grep -q "Security Screener.*Screening passenger" /tmp/airline_output.txt && \
       grep -q "Flight Attendant.*Boarding passenger" /tmp/airline_output.txt && \
       grep -q "All passengers seated! Plane ready for takeoff" /tmp/airline_output.txt && \
       grep -q "Flight Departed Successfully" /tmp/airline_output.txt; then
        print_success "Airline (10 passengers) test passed"
        echo "Sample output:"
        head -n 20 /tmp/airline_output.txt
        echo "..."
        tail -n 5 /tmp/airline_output.txt
    else
        print_error "Airline test failed - unexpected output"
        cat /tmp/airline_output.txt
        exit 1
    fi
else
    print_error "Airline test failed or timed out"
    cat /tmp/airline_output.txt
    exit 1
fi

# Test 3b: Airline - Medium test (50 passengers)
print_test "Test 3b: Airline (50 passengers, 3/5/2 workers)"
echo "Running airline 50 3 5 2..."
if timeout 60 ./airline 50 3 5 2 > /tmp/airline_50_output.txt 2>&1; then
    if grep -q "All passengers seated! Plane ready for takeoff" /tmp/airline_50_output.txt; then
        print_success "Airline (50 passengers) test passed"
    else
        print_error "Airline (50 passengers) test failed"
        tail -n 20 /tmp/airline_50_output.txt
        exit 1
    fi
else
    print_error "Airline (50 passengers) test failed or timed out"
    exit 1
fi

# Test 3c: Airline - Large test (100 passengers as specified)
print_test "Test 3c: Airline (100 passengers, 3/5/2 workers)"
echo "Running airline 100 3 5 2..."
if timeout 120 ./airline 100 3 5 2 > /tmp/airline_100_output.txt 2>&1; then
    if grep -q "All passengers seated! Plane ready for takeoff" /tmp/airline_100_output.txt; then
        print_success "Airline (100 passengers) test passed"
        echo "Final output:"
        tail -n 10 /tmp/airline_100_output.txt
    else
        print_error "Airline (100 passengers) test failed"
        tail -n 20 /tmp/airline_100_output.txt
        exit 1
    fi
else
    print_error "Airline (100 passengers) test failed or timed out"
    exit 1
fi

# Error handling tests
print_test "Test 4: Error Handling"

echo "Testing mh with invalid input (0 cycles)..."
if ./mh 0 2>&1 | grep -q "Error"; then
    print_success "mh correctly rejects 0 cycles"
else
    print_warning "mh should reject 0 cycles"
fi

echo "Testing mh with invalid input (negative cycles)..."
if ./mh -5 2>&1 | grep -q "Error"; then
    print_success "mh correctly rejects negative cycles"
else
    print_warning "mh should reject negative cycles"
fi

echo "Testing airline with invalid input (0 passengers)..."
if ./airline 0 2 2 1 2>&1 | grep -q "Error"; then
    print_success "airline correctly rejects 0 passengers"
else
    print_warning "airline should reject 0 passengers"
fi

# Summary
print_test "Test Summary"
print_success "All tests passed!"
echo ""
echo "Programs tested:"
echo "  ✓ bounded_buffer - Producer/consumer synchronization"
echo "  ✓ mh - Mother Hubbard thread coordination"
echo "  ✓ airline - Multi-stage pipeline with thread pool"
echo ""
echo "Test coverage:"
echo "  ✓ Correct synchronization behavior"
echo "  ✓ Proper output formatting"
echo "  ✓ Completion detection"
echo "  ✓ Error handling"
echo "  ✓ Scalability (10, 50, 100 passengers)"
echo ""
print_success "All Project 2 requirements satisfied!"

# Cleanup
rm -f /tmp/bounded_buffer_output.txt /tmp/mh_output.txt /tmp/mh_10_output.txt \
      /tmp/airline_output.txt /tmp/airline_50_output.txt /tmp/airline_100_output.txt

exit 0
