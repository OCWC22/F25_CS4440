#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int file = open("myfile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (file < 0) {
        perror("open");
        return 1;
    }

    // redirecting standard output to the file
    if (dup2(file, STDOUT_FILENO) < 0) {
        perror("dup2");
        close(file);
        return 1;
    }

    // will be redirected to the file
    printf("This will print in myfile.txt\n");

    close(file);
    return 0;
}