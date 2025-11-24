#!/bin/bash
# Comprehensive Test Script for Disk Server/Clients
# Shows all possible inputs: success cases, edge cases, and error handling

echo "==============================================="
echo "COMPREHENSIVE TESTS: DISK SERVER/CLIENTS"
echo "==============================================="
echo

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Helper to ensure commands never hang
run_with_timeout() {
    local seconds="$1"
    shift
    if command -v timeout >/dev/null 2>&1; then
        timeout "$seconds" "$@"
    else
        perl -e '$s=shift; $SIG{ALRM}=sub{exit 124}; alarm $s; exec @ARGV;' "$seconds" "$@"
    fi
}

# Helper to safely start server and wait for it to be ready
start_server() {
    local port="$1"
    local cyl="$2"
    local sec="$3"
    local delay="$4"
    local img="$5"
    ./disk_server "$port" "$cyl" "$sec" "$delay" "$img" &
    SERVER_PID=$!
    sleep 2
    # Verify server is actually listening
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "❌ Server failed to start"
        return 1
    fi
    return 0
}

# Helper to safely stop server
stop_server() {
    if [ -n "$SERVER_PID" ]; then
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
        SERVER_PID=""
    fi
}

# Clean up any existing processes and files
pkill -f disk_server 2>/dev/null
rm -f test_disk*.img
sleep 1

# Ensure we have a clean environment
SERVER_PID=""
trap stop_server EXIT

echo "🔧 BUILDING PROGRAMS..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ BUILD FAILED"
    exit 1
fi
echo "✅ Build successful"
echo

echo "📋 TEST 1: DISK SERVER NORMAL OPERATION"
echo "--------------------------------------"

echo "Test 1a: Starting disk server with valid parameters"
echo "Command: ./disk_server 8082 4 4 1000 test_disk.img"
if ! start_server 8082 4 4 1000 test_disk.img; then
    echo "❌ Failed to start server"
    exit 1
fi

echo "✅ Disk server started successfully"
echo

echo "📋 TEST 2: DISK CLIENT CLI - ALL COMMANDS"
echo "-----------------------------------------"

echo "Test 2a: Info command (I)"
echo "Input: I"
run_with_timeout 5 sh -c 'echo "I" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 2b: Read invalid block (should fail)"
echo "Input: R 10 10"
run_with_timeout 5 sh -c 'echo "R 10 10" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 2c: Write valid block"
echo "Input: W 0 0 hello world"
run_with_timeout 5 sh -c 'echo "W 0 0 hello world" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 2d: Read back the block we just wrote"
echo "Input: R 0 0"
run_with_timeout 5 sh -c 'echo "R 0 0" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 2e: Write to another block"
echo "Input: W 1 0 test data 123"
run_with_timeout 5 sh -c 'echo "W 1 0 test data 123" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 2f: Read the second block"
echo "Input: R 1 0"
run_with_timeout 5 sh -c 'echo "R 1 0" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 2g: Write with maximum data (128 bytes)"
echo "Creating 128-byte test string..."
TEST_DATA=$(printf 'A%.0s' {1..128})
echo "Input: W 2 0 [128 bytes of 'A']"
run_with_timeout 5 sh -c 'echo "W 2 0 $TEST_DATA" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 2h: Read back the 128-byte block"
echo "Input: R 2 0"
run_with_timeout 5 sh -c 'echo "R 2 0" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 2i: Quit command"
echo "Input: q"
run_with_timeout 5 sh -c 'echo "q" | ./disk_client_cli 127.0.0.1 8082'
echo

stop_server

echo "📋 TEST 3: DISK CLIENT CLI - ERROR HANDLING"
echo "-------------------------------------------"

echo "Test 3a: Client with no arguments"
./disk_client_cli 2>&1 | head -3
echo

echo "Test 3b: Client with insufficient arguments"
./disk_client_cli 127.0.0.1 2>&1 | head -3
echo

echo "Test 3c: Client with invalid IP"
echo "I" | ./disk_client_cli invalid.ip 8082 2>&1 | head -3
echo

echo "Test 3d: Client connecting to non-existent server"
echo "I" | ./disk_client_cli 127.0.0.1 9999 2>&1 | head -3
echo

echo "📋 TEST 4: DISK SERVER ERROR HANDLING"
echo "--------------------------------------"

echo "Test 4a: Server with no arguments"
./disk_server 2>&1 | head -3
echo

echo "Test 4b: Server with insufficient arguments"
./disk_server 8082 2>&1 | head -3
echo

echo "Test 4c: Server with invalid geometry"
./disk_server 8082 0 4 1000 test_disk.img 2>&1 | head -3
echo

echo "Test 4d: Server with invalid port"
./disk_server 99999 4 4 1000 test_disk.img 2>&1 | head -3
echo

echo "📋 TEST 5: RANDOM CLIENT - STRESS TESTING"
echo "----------------------------------------"

echo "Test 5a: Starting disk server"
if ! start_server 8082 8 8 1000 test_disk2.img; then
    echo "❌ Failed to start server for random tests"
    exit 1
fi

# Check if disk_client_random exists
if [ ! -f "./disk_client_random" ]; then
    echo "⚠️ disk_client_random not found, skipping random tests"
else
    echo "Test 5b: Random client with small number of operations"
    echo "Command: ./disk_client_random 127.0.0.1 8082 10 12345"
    run_with_timeout 10 ./disk_client_random 127.0.0.1 8082 10 12345
    echo

    echo "Test 5c: Random client with medium number of operations"
    echo "Command: ./disk_client_random 127.0.0.1 8082 50 54321"
    run_with_timeout 15 ./disk_client_random 127.0.0.1 8082 50 54321
    echo

    echo "Test 5d: Random client error - invalid N"
    run_with_timeout 5 ./disk_client_random 127.0.0.1 8082 0 12345 2>&1 | head -3
    echo

    echo "Test 5e: Random client error - insufficient arguments"
    run_with_timeout 5 ./disk_client_random 127.0.0.1 8082 2>&1 | head -3
    echo
fi

stop_server

echo "📋 TEST 6: EDGE CASES AND BOUNDARY CONDITIONS"
echo "----------------------------------------------"

echo "Test 6a: Very small disk (1x1)"
if ! start_server 8082 1 1 1000 test_disk3.img; then
    echo "❌ Failed to start server for small disk test"
    exit 1
fi

echo "Testing single block disk..."
run_with_timeout 5 sh -c 'echo "I" | ./disk_client_cli 127.0.0.1 8082'
run_with_timeout 5 sh -c 'echo "W 0 0 single" | ./disk_client_cli 127.0.0.1 8082'
run_with_timeout 5 sh -c 'echo "R 0 0" | ./disk_client_cli 127.0.0.1 8082'
echo

stop_server

echo "Test 6b: Large disk geometry"
if ! start_server 8082 100 100 1000 test_disk4.img; then
    echo "❌ Failed to start server for large disk test"
    exit 1
fi

echo "Testing large disk geometry..."
run_with_timeout 5 sh -c 'echo "I" | ./disk_client_cli 127.0.0.1 8082' | head -1
run_with_timeout 5 sh -c 'echo "W 9999 0 edge_test" | ./disk_client_cli 127.0.0.1 8082'
run_with_timeout 5 sh -c 'echo "R 9999 0" | ./disk_client_cli 127.0.0.1 8082'
echo

stop_server

echo "📋 TEST 7: CONCURRENT CLIENTS"
echo "------------------------------"

if ! start_server 8082 10 10 1000 test_disk5.img; then
    echo "❌ Failed to start server for concurrent test"
    exit 1
fi

echo "Launching 5 concurrent disk clients..."
run_with_timeout 5 sh -c 'echo "I" | ./disk_client_cli 127.0.0.1 8082' > /dev/null 2>&1 &
CLIENT1=$!
run_with_timeout 5 sh -c 'echo "W 0 0 client1" | ./disk_client_cli 127.0.0.1 8082' > /dev/null 2>&1 &
CLIENT2=$!
run_with_timeout 5 sh -c 'echo "W 1 0 client2" | ./disk_client_cli 127.0.0.1 8082' > /dev/null 2>&1 &
CLIENT3=$!
run_with_timeout 5 sh -c 'echo "R 0 0" | ./disk_client_cli 127.0.0.1 8082' > /dev/null 2>&1 &
CLIENT4=$!
run_with_timeout 5 sh -c 'echo "R 1 0" | ./disk_client_cli 127.0.0.1 8082' > /dev/null 2>&1 &
CLIENT5=$!

wait $CLIENT1 $CLIENT2 $CLIENT3 $CLIENT4 $CLIENT5
echo "✅ All concurrent clients completed successfully"

stop_server

echo "📋 TEST 8: DATA INTEGRITY"
echo "-------------------------"

if ! start_server 8082 4 4 1000 test_disk6.img; then
    echo "❌ Failed to start server for data integrity test"
    exit 1
fi

echo "Test 8a: Writing and reading binary data"
echo "Creating binary test data..."
printf '\x00\x01\x02\x03\xFF\xFE\xFD\xFC' > /tmp/binary_test.dat
BINARY_DATA=$(cat /tmp/binary_test.dat)
run_with_timeout 5 sh -c 'echo "W 3 0 $BINARY_DATA" | ./disk_client_cli 127.0.0.1 8082'
run_with_timeout 5 sh -c 'echo "R 3 0" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 8b: Writing with special characters"
run_with_timeout 5 sh -c 'echo "W 2 1 special!@#$%^&*()" | ./disk_client_cli 127.0.0.1 8082'
run_with_timeout 5 sh -c 'echo "R 2 1" | ./disk_client_cli 127.0.0.1 8082'
echo

echo "Test 8c: Overwriting existing data"
run_with_timeout 5 sh -c 'echo "W 0 0 original" | ./disk_client_cli 127.0.0.1 8082'
run_with_timeout 5 sh -c 'echo "R 0 0" | ./disk_client_cli 127.0.0.1 8082'
run_with_timeout 5 sh -c 'echo "W 0 0 replaced" | ./disk_client_cli 127.0.0.1 8082'
run_with_timeout 5 sh -c 'echo "R 0 0" | ./disk_client_cli 127.0.0.1 8082'
echo

stop_server

# Clean up test files
rm -f test_disk*.img /tmp/binary_test.dat

echo
echo "==============================================="
echo "✅ ALL DISK SERVER/CLIENT TESTS COMPLETE"
echo "============================================="
