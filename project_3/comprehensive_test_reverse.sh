#!/bin/bash
# Comprehensive Test Script for Reverse Server/Client
# Shows all possible inputs: success cases, edge cases, and error handling

echo "==============================================="
echo "COMPREHENSIVE TESTS: REVERSE SERVER/CLIENT"
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
pkill -f reverse_server 2>/dev/null
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

echo "📋 TEST 1: NORMAL OPERATION - Various string inputs"
echo "---------------------------------------------------"
./reverse_server 8080 &
SERVER_PID=$!
sleep 1
echo "Reverse server listening on port 8080"

echo "Test 1a: Simple string"
echo "hello" | ./reverse_client 127.0.0.1 8080
echo

echo "Test 1b: String with numbers"
echo "abc123xyz" | ./reverse_client 127.0.0.1 8080
echo

echo "Test 1c: Empty string"
echo "" | ./reverse_client 127.0.0.1 8080
echo

echo "Test 1d: String with special characters"
echo "hello@world#123!" | ./reverse_client 127.0.0.1 8080
echo

echo "Test 1e: Long string (near limit)"
echo "this is a very long string that should still be reversed properly by the server" | ./reverse_client 127.0.0.1 8080
echo

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "📋 TEST 2: ERROR HANDLING - Invalid inputs"
echo "-------------------------------------------"

echo "Test 2a: Server with no arguments"
run_with_timeout 5 ./reverse_server 2>&1 || echo "Program exited correctly"
echo

echo "Test 2b: Server with invalid port"
run_with_timeout 5 ./reverse_server 99999 2>&1 || echo "Program exited correctly"
echo

echo "Test 2c: Client with no arguments"
run_with_timeout 5 ./reverse_client 2>&1 || echo "Program exited correctly"
echo

echo "Test 2d: Client with insufficient arguments"
run_with_timeout 5 ./reverse_client 127.0.0.1 2>&1 || echo "Program exited correctly"
echo

echo "Test 2e: Client with invalid IP"
run_with_timeout 5 bash -c 'echo "test" | ./reverse_client invalid.ip 8080' 2>&1 || echo "Program exited correctly"
echo

echo "Test 2f: Client connecting to non-existent server"
run_with_timeout 5 bash -c 'echo "test" | ./reverse_client 127.0.0.1 9999' 2>&1 || echo "Program exited correctly"
echo

echo "📋 TEST 3: EDGE CASES - Boundary conditions"
echo "--------------------------------------------"

./reverse_server 8080 &
SERVER_PID=$!
sleep 1

echo "Test 3a: Single character"
echo "a" | ./reverse_client 127.0.0.1 8080
echo

echo "Test 3b: String with only spaces"
echo "   " | ./reverse_client 127.0.0.1 8080
echo

echo "Test 3c: String with newlines and tabs"
echo $'hello\nworld\ttest' | ./reverse_client 127.0.0.1 8080
echo

echo "Test 3d: Unicode characters"
echo "héllo wörld" | ./reverse_client 127.0.0.1 8080
echo

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "📋 TEST 4: CONCURRENT CLIENTS"
echo "-------------------------------"

./reverse_server 8080 &
SERVER_PID=$!
sleep 1

echo "Launching 3 concurrent clients..."
echo "client1" | ./reverse_client 127.0.0.1 8080 &
CLIENT1=$!
echo "client2" | ./reverse_client 127.0.0.1 8080 &
CLIENT2=$!
echo "client3" | ./reverse_client 127.0.0.1 8080 &
CLIENT3=$!

wait $CLIENT1 $CLIENT2 $CLIENT3
echo "✅ All concurrent clients completed successfully"

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "📋 TEST 5: STRESS TESTING"
echo "-------------------------"

./reverse_server 8080 &
SERVER_PID=$!
sleep 1
echo "Reverse server listening on port 8080"

echo "Sending 100 rapid requests..."
success=0
fail=0
for i in {1..100}; do
    msg="test$i"
    printf '➡️  Request %03d (%s)\n' "$i" "$msg"
    response=$(printf "%s\n" "$msg" | ./reverse_client 127.0.0.1 8080 2>&1)
    status=$?
    printf '    ↳ Exit: %s | Response: %s\n' "$status" "$response"
    if [ $status -eq 0 ]; then
        success=$((success + 1))
    else
        fail=$((fail + 1))
    fi
done
echo "✅ Stress test completed - Success: $success, Failures: $fail"

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo
echo "==============================================="
echo "✅ ALL REVERSE SERVER/CLIENT TESTS COMPLETE"
echo "============================================="
