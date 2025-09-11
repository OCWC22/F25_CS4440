#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define PROMPT "moreshell> "
#define MAX_CMD_LEN 256
#define MAX_ARGS 32

void parse_args(char *cmd, char **args) {
    int i = 0;
    char *token = strtok(cmd, " \t\n");
    
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i] = token;
        i++;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL; // Null-terminate the argument list
}

int main() {
    char cmd[MAX_CMD_LEN];
    char *args[MAX_ARGS];

    while (1) {
        printf("%s", PROMPT);
        fflush(stdout);
        
        if (fgets(cmd, MAX_CMD_LEN, stdin) == NULL) {
            printf("\n");
            break;
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

        parse_args(cmd, args);

        if (args[0] == NULL) {
            continue; // Empty command after parsing
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execvp(args[0], args);
            // If execvp returns, the command was not found
            fprintf(stderr, "moreshell: %s: command not found\n", args[0]);
            exit(1);
        } else if (pid > 0) {
            // Parent process
            int status;
            wait(&status);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                // Command failed, execvp already showed error
            }
        } else {
            perror("fork failed");
        }
    }
    
    printf("Exiting moreshell.\n");
    return 0;
}