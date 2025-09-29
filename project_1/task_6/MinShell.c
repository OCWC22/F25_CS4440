#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define PROMPT "minishell> "
#define MAX_CMD_LEN 100

int main(void) {
    char cmd[MAX_CMD_LEN];

    while (1) {
        printf("%s", PROMPT);
        fflush(stdout);
        
        if (fgets(cmd, MAX_CMD_LEN, stdin) == NULL) {
            printf("\n");
            break; // Exit on EOF (Ctrl+D)
        }

        // Remove trailing newline
        cmd[strcspn(cmd, "\n")] = 0;

        // Skip empty commands
        if (strlen(cmd) == 0) {
            continue;
        }

        if (strcmp(cmd, "exit") == 0) {
            break;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execlp(cmd, cmd, NULL);
            // If execlp returns, the command was not found
            fprintf(stderr, "minishell: %s: command not found\n", cmd);
            exit(1);
        } else if (pid > 0) {
            // Parent process
            int status;
            wait(&status);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                // Command failed, but we don't print anything extra as execlp already handled it
            }
        } else {
            perror("fork failed");
        }
    }

    printf("Exiting minishell.\n");
    return 0;
}
