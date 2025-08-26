# CEO guide: C++ fundamentals (in plain English) and what your two C examples do

This note explains, at a high level, how C/C++ programs work and what your two files do:
- `week_1/example1-ls.c`
- `week_1/example2-copy.c`

Keep this mental model: source code is a recipe; compiling bakes it into an app; running executes the app.

---

## 1) Big picture: how C/C++ works

- __Compiled, not interpreted__
  - You write `.c` or `.cpp` files.
  - A compiler (e.g., `clang`/`gcc`) turns them into a machine‑code program (an executable file like `example1-ls`).
  - You then run it: `./example1-ls`.

- __C vs C++ (quick distinction)__
  - C is minimal and close to the OS; you manually manage memory and call OS functions directly.
  - C++ = C + higher‑level features (classes, generics/templates, safer standard library). You can still call the same OS functions if needed, but you often prefer safer standard library tools.

- __Headers and libraries__
  - `#include <...>` pulls in function declarations (e.g., file I/O, process APIs).
  - The standard library provides common building blocks. C++ adds richer versions (`<string>`, `<vector>`, `<filesystem>`, streams, etc.).

- __Execution model (processes)__
  - Your program is a process. It can:
    - do work itself,
    - start another program (spawn), or
    - even replace itself with another program (exec).

---

## 2) What creates those extra files?

- __Source files__: `example1-ls.c`, `example2-copy.c` (the recipes).
- __Executables__: `example1-ls`, `example2-copy` (baked apps created by the compiler).
- __Data files__: `source.txt` (sample input we created), `destination.txt` (output created by your copy program).

---

## 3) Program A: `week_1/example1-ls.c` (run `ls -l` by replacing yourself)

```c
#include <stdio.h>
#include <unistd.h>

int main() {
    char *args[] = {"ls", "-l", NULL};
    execvp("ls", args);
}
```

- __Goal__: run the shell command `ls -l`.
- __How__: `execvp("ls", args)` tells the OS: "Stop running me; run `ls -l` in my place." If it succeeds, your code does not continue; it literally becomes `ls`.
- __What you saw__: it printed a directory listing. It did not create files.

- __C++ ways to do similar__
  - Simple but less safe: `std::system("ls -l");` (spawns a shell to run the command).
  - Better: write native code to list files (no external command) using C++17 `std::filesystem::directory_iterator`.

- __Why this matters for OpenCV__
  - For image pipelines, you don’t need to exec commands. Use C++/OpenCV APIs to list files and process them inside your app (fewer moving parts, easier to debug).

---

## 4) Program B: `week_1/example2-copy.c` (copy one file to another)

```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *source, *destination;
    int ch;

    source = fopen("source.txt", "r");
    destination = fopen("destination.txt", "w");

    ch = fgetc(source);
    while (ch != EOF) {
        fputc(ch, destination);
        ch = fgetc(source);
    }

    fclose(source);
    fclose(destination);
    return 0;
}
```

- __Goal__: read from `source.txt`, write to `destination.txt`.
- __Flow__:
  - `fopen(..., "r")` opens a file for reading (creates an error if missing).
  - `fopen(..., "w")` creates (or overwrites) the output file.
  - Loop reads one byte/char (`fgetc`) and writes it out (`fputc`) until end‑of‑file.
  - `fclose` flushes and releases files.
- __What you saw__: `destination.txt` appeared with the same contents as `source.txt`.

- __Production notes__
  - Always check errors: if `source` or `destination` is `NULL`, print error and exit.
  - For binary data (images), use modes `"rb"`/`"wb"` so nothing gets altered.

- __C++ ways to do similar__
  - One‑liner: `std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);`
  - Streams:
    ```cpp
    #include <fstream>
    std::ifstream src("source.txt", std::ios::binary);
    std::ofstream dst("destination.txt", std::ios::binary);
    dst << src.rdbuf();
    ```

- __Why this matters for OpenCV__
  - If you need to modify an image, load it with OpenCV and re‑save:
    ```cpp
    #include <opencv2/opencv.hpp>
    cv::Mat img = cv::imread("source.png");      // decode
    cv::imwrite("destination.png", img);         // re‑encode
    ```
  - If you only need a byte‑for‑byte copy, prefer `std::filesystem::copy_file` (fast, simple).

---

## 5) How to build and run (on macOS with clang)

From the `week_1/` folder:

```bash
# Build
clang -Wall -Wextra -O2 -o example1-ls example1-ls.c
clang -Wall -Wextra -O2 -o example2-copy example2-copy.c

# Run
./example1-ls                 # prints a directory listing

# Prepare input for the copy demo (once)
printf 'Sample data\n' > source.txt
./example2-copy
ls -l source.txt destination.txt
```

---

## 6) Practical CEO takeaways (how to contribute to fixes)

- __Prefer standard library/OpenCV over shell commands__
  - Fewer external dependencies, easier to test and debug.

- __Add basic safety__
  - Check return values (file opens, system calls). Fail fast with a clear message.
  - Use binary modes for non‑text files.

- __Structure for reliability__
  - Small functions that do one thing.
  - Log just enough context (never secrets).
  - Add a tiny test or script to prove behavior (e.g., diff two files after copying).

- __For image processing__
  - Use OpenCV to load/process/save.
  - Use `std::filesystem` to find files and copy/move them as needed.

---

## 7) Quick checklist for these two programs

- `example1-ls.c`
  - [ ] Add error handling:
    ```c
    if (execvp("ls", args) == -1) { perror("execvp"); return 1; }
    ```
  - [ ] Consider replacing with native directory listing via C++ `std::filesystem` (portable, testable).

- `example2-copy.c`
  - [ ] Handle missing `source.txt` gracefully (print error, exit non‑zero).
  - [ ] Use `"rb"/"wb"` when copying binary files.
  - [ ] Optionally accept CLI args: `./example2-copy <src> <dst>`.

---

If you want, I can convert both to minimal C++ versions and add error handling plus tiny tests. That’s a small, high‑leverage improvement.
