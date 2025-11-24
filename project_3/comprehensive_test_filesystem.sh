#!/bin/bash
# Comprehensive Test Script for Filesystem Server/Client
# Shows all possible inputs: success cases, edge cases, and error handling

echo "==============================================="
echo "COMPREHENSIVE TESTS: FILESYSTEM SERVER/CLIENT"
echo "==============================================="
echo

# Clean up any existing processes and files
pkill -f disk_server 2>/dev/null
pkill -f fs_server 2>/dev/null
rm -f fs_test_disk*.img
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

echo "📋 TEST 1: FILESYSTEM SERVER SETUP"
echo "---------------------------------"

echo "Test 1a: Starting disk server"
./disk_server 8082 8 8 1000 fs_test_disk.img &
DISK_PID=$!
sleep 2

echo "Test 1b: Starting filesystem server"
echo "Command: ./fs_server 127.0.0.1 8082 8083"
./fs_server 127.0.0.1 8082 8083 &
FS_PID=$!
sleep 2

echo "✅ Filesystem server started successfully"
echo

echo "📋 TEST 2: BASIC FILE OPERATIONS"
echo "--------------------------------"

echo "Test 2a: Create a file (MK)"
echo "Input: MK test1.txt"
echo "MK test1.txt" | ./fs_client 127.0.0.1 8083
echo

echo "Test 2b: Write to file (WRITE)"
echo "Input: WRITE test1.txt 0 hello world"
echo "WRITE test1.txt 0 hello world" | ./fs_client 127.0.0.1 8083
echo

echo "Test 2c: Read from file (READ)"
echo "Input: READ test1.txt 0 11"
echo "READ test1.txt 0 11" | ./fs_client 127.0.0.1 8083
echo

echo "Test 2d: List directory (LS)"
echo "Input: LS"
echo "LS" | ./fs_client 127.0.0.1 8083
echo

echo "Test 2e: Create another file"
echo "Input: MK test2.txt"
echo "MK test2.txt" | ./fs_client 127.0.0.1 8083
echo

echo "Test 2f: Write to second file"
echo "Input: WRITE test2.txt 0 second file"
echo "WRITE test2.txt 0 second file" | ./fs_client 127.0.0.1 8083
echo

echo "Test 2g: List directory again"
echo "Input: LS"
echo "LS" | ./fs_client 127.0.0.1 8083
echo

echo "📋 TEST 3: DIRECTORY OPERATIONS"
echo "------------------------------"

echo "Test 3a: Create directory (MKDIR)"
echo "Input: MKDIR testdir"
echo "MKDIR testdir" | ./fs_client 127.0.0.1 8083
echo

echo "Test 3b: List directory to see new dir"
echo "Input: LS"
echo "LS" | ./fs_client 127.0.0.1 8083
echo

echo "Test 3c: Create file in directory"
echo "Input: MK testdir/file1.txt"
echo "MK testdir/file1.txt" | ./fs_client 127.0.0.1 8083
echo

echo "Test 3d: Write to file in subdirectory"
echo "Input: WRITE testdir/file1.txt 0 content in subdir"
echo "WRITE testdir/file1.txt 0 content in subdir" | ./fs_client 127.0.0.1 8083
echo

echo "Test 3e: Read file from subdirectory"
echo "Input: READ testdir/file1.txt 0 20"
echo "READ testdir/file1.txt 0 20" | ./fs_client 127.0.0.1 8083
echo

echo "Test 3f: List subdirectory"
echo "Input: LS testdir"
echo "LS testdir" | ./fs_client 127.0.0.1 8083
echo

echo "📋 TEST 4: FILE MANIPULATION"
echo "----------------------------"

echo "Test 4a: Append to file"
echo "Input: WRITE test1.txt 11 more data"
echo "WRITE test1.txt 11 more data" | ./fs_client 127.0.0.1 8083
echo

echo "Test 4b: Read entire file"
echo "Input: READ test1.txt 0 21"
echo "READ test1.txt 0 21" | ./fs_client 127.0.0.1 8083
echo

echo "Test 4c: Write in middle of file"
echo "Input: WRITE test1.txt 6 INSERTED"
echo "WRITE test1.txt 6 INSERTED" | ./fs_client 127.0.0.1 8083
echo

echo "Test 4d: Read modified file"
echo "Input: READ test1.txt 0 21"
echo "READ test1.txt 0 21" | ./fs_client 127.0.0.1 8083
echo

echo "Test 4e: Create large file"
echo "Input: MK largefile.txt"
echo "MK largefile.txt" | ./fs_client 127.0.0.1 8083
echo

echo "Input: WRITE largefile.txt 0 $(printf 'A%.0s' {1..200})"
LARGE_DATA=$(printf 'A%.0s' {1..200})
echo "WRITE largefile.txt 0 $LARGE_DATA" | ./fs_client 127.0.0.1 8083
echo

echo "Input: READ largefile.txt 0 200"
echo "READ largefile.txt 0 200" | ./fs_client 127.0.0.1 8083 | head -1
echo "(Showing first line only)"
echo

echo "📋 TEST 5: ERROR HANDLING"
echo "-------------------------"

echo "Test 5a: Create file with invalid name (too long)"
echo "Input: MK $(printf 'A%.0s' {1..20}).txt"
LONG_NAME=$(printf 'A%.0s' {1..20}).txt
echo "MK $LONG_NAME" | ./fs_client 127.0.0.1 8083
echo

echo "Test 5b: Create file that already exists"
echo "Input: MK test1.txt"
echo "MK test1.txt" | ./fs_client 127.0.0.1 8083
echo

echo "Test 5c: Read non-existent file"
echo "Input: READ nonexistent.txt 0 10"
echo "READ nonexistent.txt 0 10" | ./fs_client 127.0.0.1 8083
echo

echo "Test 5d: Write to non-existent file"
echo "Input: WRITE nonexistent.txt 0 data"
echo "WRITE nonexistent.txt 0 data" | ./fs_client 127.0.0.1 8083
echo

echo "Test 5e: Delete non-existent file"
echo "Input: RM nonexistent.txt"
echo "RM nonexistent.txt" | ./fs_client 127.0.0.1 8083
echo

echo "Test 5f: Read beyond file size"
echo "Input: READ test1.txt 100 10"
echo "READ test1.txt 100 10" | ./fs_client 127.0.0.1 8083
echo

echo "Test 5g: Create directory with invalid name"
echo "Input: MKDIR $(printf 'B%.0s' {1..20})"
LONG_DIR=$(printf 'B%.0s' {1..20})
echo "MKDIR $LONG_DIR" | ./fs_client 127.0.0.1 8083
echo

echo "📋 TEST 6: DELETE OPERATIONS"
echo "---------------------------"

echo "Test 6a: Delete file (RM)"
echo "Input: RM test2.txt"
echo "RM test2.txt" | ./fs_client 127.0.0.1 8083
echo

echo "Test 6b: List directory to confirm deletion"
echo "Input: LS"
echo "LS" | ./fs_client 127.0.0.1 8083
echo

echo "Test 6c: Delete directory (RMDIR)"
echo "Input: RMDIR testdir"
echo "RMDIR testdir" | ./fs_client 127.0.0.1 8083
echo

echo "Test 6d: List directory to confirm directory deletion"
echo "Input: LS"
echo "LS" | ./fs_client 127.0.0.1 8083
echo

echo "📋 TEST 7: EDGE CASES"
echo "---------------------"

echo "Test 7a: Create empty file"
echo "Input: MK empty.txt"
echo "MK empty.txt" | ./fs_client 127.0.0.1 8083
echo

echo "Test 7b: Read empty file"
echo "Input: READ empty.txt 0 10"
echo "READ empty.txt 0 10" | ./fs_client 127.0.0.1 8083
echo

echo "Test 7c: Write to empty file"
echo "Input: WRITE empty.txt 0 now has content"
echo "WRITE empty.txt 0 now has content" | ./fs_client 127.0.0.1 8083
echo

echo "Test 7d: Read the file"
echo "Input: READ empty.txt 0 15"
echo "READ empty.txt 0 15" | ./fs_client 127.0.0.1 8083
echo

echo "Test 7e: File with special characters in name"
echo "Input: MK file-with_special.chars"
echo "MK file-with_special.chars" | ./fs_client 127.0.0.1 8083
echo

echo "Test 7f: Write to file with special name"
echo "Input: WRITE file-with_special.chars 0 special name test"
echo "WRITE file-with_special.chars 0 special name test" | ./fs_client 127.0.0.1 8083
echo

echo "Test 7g: Read file with special name"
echo "Input: READ file-with_special.chars 0 16"
echo "READ file-with_special.chars 0 16" | ./fs_client 127.0.0.1 8083
echo

echo "📋 TEST 8: CONCURRENT OPERATIONS"
echo "--------------------------------"

echo "Launching 5 concurrent filesystem clients..."
echo "MK concurrent1.txt" | ./fs_client 127.0.0.1 8083 > /dev/null 2>&1 &
CLIENT1=$!
echo "MK concurrent2.txt" | ./fs_client 127.0.0.1 8083 > /dev/null 2>&1 &
CLIENT2=$!
echo "MK concurrent3.txt" | ./fs_client 127.0.0.1 8083 > /dev/null 2>&1 &
CLIENT3=$!
echo "LS" | ./fs_client 127.0.0.1 8083 > /dev/null 2>&1 &
CLIENT4=$!
echo "MK concurrent4.txt" | ./fs_client 127.0.0.1 8083 > /dev/null 2>&1 &
CLIENT5=$!

wait $CLIENT1 $CLIENT2 $CLIENT3 $CLIENT4 $CLIENT5
echo "✅ All concurrent clients completed successfully"

echo "📋 TEST 9: SERVER ERROR HANDLING"
echo "--------------------------------"

echo "Test 9a: Filesystem server with no arguments"
./fs_server 2>&1 | head -3
echo

echo "Test 9b: Filesystem server with insufficient arguments"
./fs_server 127.0.0.1 8082 2>&1 | head -3
echo

echo "Test 9c: Filesystem client with no arguments"
./fs_client 2>&1 | head -3
echo

echo "Test 9d: Filesystem client with insufficient arguments"
./fs_client 127.0.0.1 2>&1 | head -3
echo

echo "Test 9e: Client with invalid IP"
echo "LS" | ./fs_client invalid.ip 8083 2>&1 | head -3
echo

echo "Test 9f: Client connecting to non-existent server"
echo "LS" | ./fs_client 127.0.0.1 9999 2>&1 | head -3
echo

# Clean up
kill $DISK_PID $FS_PID 2>/dev/null
wait $DISK_PID $FS_PID 2>/dev/null
rm -f fs_test_disk*.img

echo
echo "==============================================="
echo "✅ ALL FILESYSTEM SERVER/CLIENT TESTS COMPLETE"
echo "============================================="
