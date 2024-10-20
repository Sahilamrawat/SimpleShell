#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>

#define ARG_MAX_COUNT    1024  /* Maximum number of arguments to a command */
#define HISTORY_MAXITEMS 100   /* Maximum number of elements in the history */
#define MAX_BACKGROUND_PROCESSES 100  /* Max number of background processes */

/* Global variables to store command history and related data */
char **history;
int history_len = 0;
pid_t *pids;
time_t *start_times;
double *durations;

/* Struct for handling background processes */
typedef struct {
    pid_t pid;
    char *cmd;
} BackgroundProcess;

BackgroundProcess background_processes[MAX_BACKGROUND_PROCESSES];
int bg_process_count = 0;

/* Initializes the history data structures.
 * Allocates memory for history, PIDs, start times, and durations.
 */
void init_history() {
    history = calloc(HISTORY_MAXITEMS, sizeof(char *));
    pids = calloc(HISTORY_MAXITEMS, sizeof(pid_t));
    start_times = calloc(HISTORY_MAXITEMS, sizeof(time_t));
    durations = calloc(HISTORY_MAXITEMS, sizeof(double));
    if (!history || !pids || !start_times || !durations) {
        fprintf(stderr, "error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
}

/* Adds a command to the history along with its PID and execution duration.
 * If the history exceeds its limit, it shifts older entries out.
 */
void add_to_history(char *cmd, pid_t pid, double duration) {
    char *line = strdup(cmd);  // Duplicate command to store in history
    if (line == NULL) return;

    // If history is full, remove the oldest entry
    if (history_len == HISTORY_MAXITEMS) {
        free(history[0]);
        memmove(history, history + 1, sizeof(char *) * (HISTORY_MAXITEMS - 1));
        memmove(pids, pids + 1, sizeof(pid_t) * (HISTORY_MAXITEMS - 1));
        memmove(start_times, start_times + 1, sizeof(time_t) * (HISTORY_MAXITEMS - 1));
        memmove(durations, durations + 1, sizeof(double) * (HISTORY_MAXITEMS - 1));
        history_len--;
    }

    history[history_len] = line;
    pids[history_len] = pid;
    start_times[history_len] = time(NULL);
    durations[history_len] = duration;
    history_len++;
}

/* Prints the command history without additional details (basic view). */
void print_history() {
    for (int i = 0; i < history_len; i++) {
        printf("%d %s \n", i + 1, history[i]);
    }
}

/* Prints the command history with PID and execution duration details. */
void print_history1() {
    for (int i = 0; i < history_len; i++) {
        printf("%d %s (pid: %d, duration: %.2f seconds)\n", i + 1, history[i], pids[i], durations[i]);
    }
}

/* Checks for background processes that have finished execution.
 * If a process is completed, it removes the process from the tracking array.
 */
void check_background_processes() {
    for (int i = 0; i < bg_process_count; i++) {
        int status;
        pid_t result = waitpid(background_processes[i].pid, &status, WNOHANG);
        
        if (result == background_processes[i].pid) {
            printf("[Background] PID: %d finished command: %s\n", background_processes[i].pid, background_processes[i].cmd);
            free(background_processes[i].cmd);
            for (int j = i; j < bg_process_count - 1; j++) {
                background_processes[j] = background_processes[j + 1];
            }
            bg_process_count--;
            i--;  // Decrement index to avoid skipping the next entry
        }
    }
}

/* Executes a single command without pipes.
 * Supports background execution (indicated by a trailing '&').
 */
void execute_single_command(char *cmd) {
    char *args[ARG_MAX_COUNT];
    int tokenCount = 0;
    int background = 0;

    size_t cmd_len = strlen(cmd);
    if (cmd[cmd_len - 1] == '&') {
        background = 1;  // Command should run in the background
        cmd[cmd_len - 1] = '\0';
    }

    // Tokenize the command into arguments
    char *token = strtok(cmd, " ");
    while (token != NULL && tokenCount < ARG_MAX_COUNT) {
        args[tokenCount++] = token;
        token = strtok(NULL, " ");
    }
    args[tokenCount] = NULL;

    // Record the start time before forking
    time_t start = time(NULL);  // Start measuring time

    pid_t pid = fork();
    if (pid == 0) {  // Child process
        execvp(args[0], args);  // Execute the command
        perror("exec");  // If exec fails
        exit(EXIT_FAILURE);
    } else if (pid > 0) {  // Parent process
        int status;

        if (!background) {  // Foreground process
            waitpid(pid, &status, 0);  // Wait for the process to complete
            time_t end = time(NULL);  // End measuring time
            double duration = difftime(end, start);  // Calculate duration
            add_to_history(cmd, pid, duration);  // Add to history with actual duration
        } else {  // Background process
            printf("[Background] PID: %d running command: %s\n", pid, args[0]);
            add_to_history(cmd, pid, 0.0);  // Store background process with 0 duration for now
            if (bg_process_count < MAX_BACKGROUND_PROCESSES) {
                background_processes[bg_process_count++] = (BackgroundProcess){pid, strdup(args[0])};
            }
        }
    } else {
        perror("fork");  // Fork failed
    }
}


/* Executes a series of piped commands by creating multiple processes.
 * Uses pipes to connect the output of one process to the input of another.
 */
void execute_piped_commands(char *cmd_parts[], int num_parts) {
    int fd[2];
    pid_t pid;
    int fd_in = 0;

    for (int i = 0; i < num_parts; i++) {
        pipe(fd);

        if ((pid = fork()) == 0) {  // Child process
            dup2(fd_in, 0);  // Use the previous process's output as input
            if (i < num_parts - 1) {
                dup2(fd[1], 1);  // Redirect output to the next pipe
            }
            close(fd[0]);  // Close the read end of the pipe
            char *args[ARG_MAX_COUNT];
            int tokenCount = 0;
            char *token = strtok(cmd_parts[i], " ");
            while (token != NULL && tokenCount < ARG_MAX_COUNT) {
                args[tokenCount++] = token;
                token = strtok(NULL, " ");
            }
            args[tokenCount] = NULL;

            execvp(args[0], args);  // Execute the command
            perror("exec");
            exit(EXIT_FAILURE);
        } else {
            close(fd[1]);  // Close the write end of the pipe
            fd_in = fd[0];  // Save the read end for the next command
        }
    }
    wait(NULL);  // Wait for the last process to complete
}

/* Parses a command and determines whether to execute it as a single command
 * or a series of piped commands.
 */
void launch_command(char *cmd) {
    char original_cmd[ARG_MAX_COUNT]; 
    strncpy(original_cmd, cmd, ARG_MAX_COUNT);

    // Split the command on pipes (|) if they exist
    char *cmd_part = strtok(cmd, "|");
    char *cmd_parts[ARG_MAX_COUNT];
    int num_parts = 0;

    while (cmd_part != NULL) {
        cmd_parts[num_parts++] = cmd_part;
        cmd_part = strtok(NULL, "|");
    }

    if (num_parts == 1) {
        execute_single_command(cmd_parts[0]);  // Single command execution
    } else {
        execute_piped_commands(cmd_parts, num_parts);  // Piped command execution
    }


    check_background_processes();  // Check for any completed background processes
}

/* Checks if a given input string is blank (i.e., contains only whitespace). */
int is_blank(char *input) {
    int n = strlen(input);
    for (int i = 0; i < n; i++) {
        if (!isspace(input[i]))
            return 0;
    }
    return 1;
}

/* Handles built-in shell commands like 'exit', 'history', and 'cd'.
 * Returns -1 if 'exit' is called to terminate the shell, 
 * 0 for successful built-in command execution,
 * and 1 if it's not a built-in command.
 */
int handle_builtin(char *input) {
    if (strcmp(input, "exit") == 0) {
        return -1;  // Exit command
    }
    if (strcmp(input, "history") == 0) {
        print_history();  // Display command history
        return 0;
    }
    if (strncmp(input, "cd", 2) == 0) {  // Change directory
        char *dir = strtok(input + 3, " ");
        if (chdir(dir) != 0) {
            perror("cd");  // Error in changing directory
        }
        return 0;
    }
    return 1;  // Not a built-in command
}

/* Main loop for the shell.
 * Continuously prompts the user for input, processes commands, 
 * and handles built-in commands, background processes, and history.
 */
int main(void) {
    init_history();  // Initialize history storage
    do {
        check_background_processes();  // Check for completed background processes
        printf("simple-shell>$$> ");
        fflush(stdout);

        char *input = NULL;
        size_t len = 0;
        ssize_t nread = getline(&input, &len, stdin);  // Read user input
        if (nread == -1) {
            free(input);
            break;
        }

        input[nread - 1] = '\0';  // Remove the newline character
        if (is_blank(input)) {
            free(input);
            continue;
        }

        int ret = handle_builtin(input);  // Handle built-in commands
        if (ret == -1) {
            break;  // Exit shell
        } else if (ret == 1) {
            launch_command(input);  // Execute external command
        }

        free(input);
    } while (1);

    printf("Execution summary:\n");
    print_history1();  // Print detailed execution history

    // Clean up allocated memory
    for (int i = 0; i < history_len; i++) {
        free(history[i]);
    }
    free(history);
    free(pids);
    free(start_times);
    free(durations);

    for (int i = 0; i < bg_process_count; i++) {
        free(background_processes[i].cmd);
    }

    return 0;
}
