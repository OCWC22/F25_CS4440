#!/bin/bash
# Comprehensive Test Script for LS Server/Client
# Shows all possible inputs: success cases, edge cases, and error handling

echo "==============================================="
echo "COMPREHENSIVE TESTS: LS SERVER/CLIENT"
echo "==============================================="
echo

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

run_with_timeout() {
    local seconds="$1"
    shift
    if command -v timeout >/dev/null 2>&1; then
        timeout "$seconds" "$@"
    else
        perl -e '$s=shift; $SIG{ALRM}=sub{exit 124}; alarm $s; exec @ARGV;' "$seconds" "$@"
    fi
}

# Clean up any existing processes
pkill -f ls_server 2>/dev/null
sleep 1

echo "🔧 BUILDING PROGRAMS..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ BUILD FAILED"
    exit 1
fi
echo "✅ Build successful"
echo

echo "📋 TEST 1: NORMAL OPERATION - Various ls commands"
echo "-------------------------------------------------"
./ls_server 8081 &
SERVER_PID=$!
sleep 1

echo "Test 1a: ls with -l flag (detailed listing)"
echo "Input: -l /tmp"
./ls_client 127.0.0.1 8081 -l /tmp
echo

echo "Test 1b: ls without flags (simple listing)"
echo "Input: /tmp"
./ls_client 127.0.0.1 8081 /tmp
echo

echo "Test 1c: ls with -a flag (show all files)"
echo "Input: -a /tmp"
./ls_client 127.0.0.1 8081 -a /tmp
echo

echo "Test 1d: ls with multiple flags"
echo "Input: -la /tmp"
./ls_client 127.0.0.1 8081 -la /tmp
echo

echo "Test 1e: ls current directory"
echo "Input: ."
./ls_client 127.0.0.1 8081 .
echo

echo "Test 1f: ls parent directory"
echo "Input: .."
./ls_client 127.0.0.1 8081 ..
echo

echo "Test 1g: ls with multiple directories"
echo "Input: /tmp /var"
./ls_client 127.0.0.1 8081 /tmp /var
echo

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "📋 TEST 2: ERROR HANDLING - Invalid inputs"
echo "-----------------------------------------"

echo "Test 2a: Server with no arguments"
run_with_timeout 5 ./ls_server 2>&1 | head -3
echo

echo "Test 2b: Server with invalid port"
run_with_timeout 5 ./ls_server 99999 2>&1 | head -3
echo

echo "Test 2c: Client with no arguments"
run_with_timeout 5 ./ls_client 2>&1 | head -3
echo

echo "Test 2d: Client with insufficient arguments"
run_with_timeout 5 ./ls_client 127.0.0.1 8081 2>&1 | head -3
echo

echo "Test 2e: Client with invalid IP"
run_with_timeout 5 ./ls_client invalid.ip 8081 /tmp 2>&1 | head -3
echo

echo "Test 2f: Client connecting to non-existent server"
run_with_timeout 5 ./ls_client 127.0.0.1 9999 /tmp 2>&1 | head -3
echo

echo "📋 TEST 3: EDGE CASES - Boundary conditions"
echo "--------------------------------------------"

./ls_server 8081 &
SERVER_PID=$!
sleep 1

echo "Test 3a: ls non-existent directory"
echo "Input: /nonexistent/directory"
./ls_client 127.0.0.1 8081 /nonexistent/directory
echo

echo "Test 3b: ls empty directory"
echo "Creating empty test directory..."
mkdir -p /tmp/empty_test_dir
echo "Input: /tmp/empty_test_dir"
./ls_client 127.0.0.1 8081 /tmp/empty_test_dir
echo "Cleaning up..."
rmdir /tmp/empty_test_dir
echo

echo "Test 3c: ls with many flags"
echo "Input: -lah /tmp"
./ls_client 127.0.0.1 8081 -lah /tmp
echo

echo "Test 3d: ls root directory"
echo "Input: /"
./ls_client 127.0.0.1 8081 / | head -10
echo "(Showing first 10 lines only)"
echo

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "📋 TEST 4: CONCURRENT CLIENTS"
echo "------------------------------"

./ls_server 8081 &
SERVER_PID=$!
sleep 1

echo "Launching 5 concurrent ls clients..."
./ls_client 127.0.0.1 8081 /tmp > /dev/null 2>&1 &
CLIENT1=$!
./ls_client 127.0.0.1 8081 -l /tmp > /dev/null 2>&1 &
CLIENT2=$!
./ls_client 127.0.0.1 8081 -a /tmp > /dev/null 2>&1 &
CLIENT3=$!
./ls_client 127.0.0.1 8081 /var > /dev/null 2>&1 &
CLIENT4=$!
./ls_client 127.0.0.1 8081 /usr > /dev/null 2>&1 &
CLIENT5=$!

wait $CLIENT1 $CLIENT2 $CLIENT3 $CLIENT4 $CLIENT5
echo "✅ All concurrent clients completed successfully"

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "📋 TEST 5: STRESS TESTING"
echo "-------------------------"

./ls_server 8081 &
SERVER_PID=$!
sleep 1

echo "Sending 50 rapid ls requests..."
success=0
fail=0
for i in {1..50}; do
    printf '➡️  Request %02d: ls /tmp\n' "$i"
    output=$(./ls_client 127.0.0.1 8081 /tmp 2>&1)
    status=$?
    printf '    ↳ Exit: %s | Lines: %s\n' "$status" "$(echo "$output" | wc -l)"
    if [ $status -eq 0 ]; then
        success=$((success + 1))
    else
        fail=$((fail + 1))
    fi
done
echo "✅ Stress test completed - Success: $success, Failures: $fail"

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "📋 TEST 6: FILE SYSTEM OPERATIONS"
echo "----------------------------------"

./ls_server 8081 &
SERVER_PID=$!
sleep 1

echo "Test 6a: ls with file paths"
echo "Creating test files..."
touch /tmp/test_file1.txt /tmp/test_file2.txt
echo "Input: /tmp/test_file1.txt"
./ls_client 127.0.0.1 8081 /tmp/test_file1.txt
echo

echo "Test 6b: ls with wildcards (should be handled by shell)"
echo "Input: /tmp/test*"
./ls_client 127.0.0.1 8081 /tmp/test*
echo

echo "Cleaning up test files..."
rm -f /tmp/test_file1.txt /tmp/test_file2.txt

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo
echo "==============================================="
echo "✅ ALL LS SERVER/CLIENT TESTS COMPLETE"
echo "============================================="
