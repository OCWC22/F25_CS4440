#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define PROMPT "dupshell> "
#define MAX_CMD_LEN 256
#define MAX_ARGS 32

int parse_pipe_command(char *cmd, char **cmd1_args, char **cmd2_args) {
    char *pipe_pos = strchr(cmd, '|');
    if (pipe_pos == NULL) {
        return 0; // No pipe found
    }

    *pipe_pos = '\0'; // Split the command at the pipe

    // Parse first command (before pipe)
    int i = 0;
    char *token = strtok(cmd, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        cmd1_args[i] = token;
        i++;
        token = strtok(NULL, " \t\n");
    }
    cmd1_args[i] = NULL;

    // Parse second command (after pipe)
    i = 0;
    token = strtok(pipe_pos + 1, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        cmd2_args[i] = token;
        i++;
        token = strtok(NULL, " \t\n");
    }
    cmd2_args[i] = NULL;

    return 1; // Pipe found and parsed successfully
}

int main(void) {
    char cmd[MAX_CMD_LEN];
    char *cmd1_args[MAX_ARGS], *cmd2_args[MAX_ARGS];

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

        // Try to parse as a piped command
        if (!parse_pipe_command(cmd, cmd1_args, cmd2_args)) {
            printf("This shell only handles piped commands. Usage: command1 | command2\n");
            continue;
        }

        // Check if both commands are valid
        if (cmd1_args[0] == NULL || cmd2_args[0] == NULL) {
            printf("Invalid piped command format. Usage: command1 | command2\n");
            continue;
        }

        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            perror("pipe failed");
            continue;
        }

        pid_t pid1 = fork();
        if (pid1 == 0) {
            // First child - will execute command1
            close(pipe_fd[0]); // Close read end of pipe
            dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe write end
            close(pipe_fd[1]);
            
            execvp(cmd1_args[0], cmd1_args);
            fprintf(stderr, "dupshell: %s: command not found\n", cmd1_args[0]);
            exit(1);
        } else if (pid1 < 0) {
            perror("fork failed for first command");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            continue;
        }

        pid_t pid2 = fork();
        if (pid2 == 0) {
            // Second child - will execute command2
            close(pipe_fd[1]); // Close write end of pipe
            dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin from pipe read end
            close(pipe_fd[0]);
            
            execvp(cmd2_args[0], cmd2_args);
            fprintf(stderr, "dupshell: %s: command not found\n", cmd2_args[0]);
            exit(1);
        } else if (pid2 < 0) {
            perror("fork failed for second command");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            continue;
        }

        // Parent process
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        // Wait for both children to complete
        int status1, status2;
        waitpid(pid1, &status1, 0);
        waitpid(pid2, &status2, 0);
    }

    printf("Exiting dupshell.\n");
    return 0;
}
