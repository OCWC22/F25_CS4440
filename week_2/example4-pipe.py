#!/usr/bin/env python3
"""
Python equivalent of the C pipe/fork example
Demonstrates IPC using multiprocessing.Pipe()
"""

import multiprocessing as mp
import os

def child_process(pipe_end, output_file):
    """Child process: reads from pipe and converts uppercase to lowercase"""
    print(f"Child process PID: {os.getpid()}")

    with open(output_file, 'w') as dest:
        while True:
            # Read one character at a time from pipe
            data = pipe_end.recv()
            if data == 'EOF':  # End of file marker
                break

            ch = data
            # Convert uppercase to lowercase (same logic as C code)
            if 'A' <= ch <= 'Z':
                ch = chr(ord(ch) + 32)  # Add 32 to ASCII value

            dest.write(ch)

    pipe_end.close()
    print("Child process finished")

def parent_process(pipe_end, input_file):
    """Parent process: reads from file and writes to pipe"""
    print(f"Parent process PID: {os.getpid()}")

    with open(input_file, 'r') as source:
        while True:
            ch = source.read(1)  # Read one character
            if not ch:  # End of file
                break

            # Send character through pipe
            pipe_end.send(ch)

    # Send end-of-file marker
    pipe_end.send('EOF')
    pipe_end.close()
    print("Parent process finished")

def main():
    """Main function - equivalent to C main()"""
    input_file = "source.txt"
    output_file = "destination.txt"

    print("Python Pipe IPC Demo")
    print("===================")

    # Create pipe (equivalent to pipe() system call)
    parent_conn, child_conn = mp.Pipe()

    # Create child process (equivalent to fork())
    child = mp.Process(target=child_process, args=(child_conn, output_file))
    child.start()

    # Parent process work
    parent_process(parent_conn, input_file)

    # Wait for child to finish (equivalent to wait())
    child.join()

    print("Both processes completed!")
    print(f"Check '{output_file}' for the converted text")

if __name__ == "__main__":
    main()
