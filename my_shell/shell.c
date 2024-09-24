#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>
#include <time.h>

/* Constants */
#define ARG_MAX_COUNT    1024      /* max number of arguments to a command */
#define HISTORY_MAXITEMS 100       /* max number of elements in the history */

/* Type declarations */
struct command {                  
	int argc;                  /* number of arguments in the command */
	char *name;                /* name of the command */
	char *argv[ARG_MAX_COUNT]; /* the arguments themselves */
};

struct commands {                  
	int cmd_count;             /* number of commands in the pipeline */
	struct command *cmds[];    /* the commands themselves */
};

/* Global variables */
extern char **history;           /* Array to store history of commands */
extern int history_len;          /* Length of the command history */
extern pid_t *pids;              /* Array to store process IDs of commands */
extern time_t *start_times;      /* Array to store start times of commands */
extern double *durations;        /* Array to store execution durations of commands */

/* Function Prototypes */

/* Initializes the history storage */
void init_history(void);                                

/* Adds a command to the history along with its PID and execution duration */
void add_to_history(char *cmd, pid_t pid, double duration);

/* Prints the command history with associated PIDs and durations */
void print_history(void);

/* Launches an external command by forking and executing */
void launch_command(char *cmd);

/* Checks if a string input is blank (contains only whitespace) */
int is_blank(char *input);

/* Handles built-in commands such as 'exit', 'history', and 'cd'. 
 * Returns -1 for 'exit', 0 for a successful built-in command execution, 
 * and 1 if the input is not a built-in command.
 */
int handle_builtin(char *input);

void execute_single_command(char *cmd);
void execute_piped_commands(char *cmd_parts[], int num_parts);


/* Global variables */
char **history;
int history_len = 0;
pid_t *pids;
time_t *start_times;
double *durations;

#define ARG_MAX_COUNT 1024  /* Match the definition in shell.h */
#define MAX_BACKGROUND_PROCESSES 100

/* Struct for background processes */
typedef struct {
    pid_t pid;
    char *cmd;
} BackgroundProcess;

BackgroundProcess background_processes[MAX_BACKGROUND_PROCESSES];
int bg_process_count = 0;

/* Initialize history array */
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

/* Add a command to the history */
void add_to_history(char *cmd, pid_t pid, double duration) {
    char *line = strdup(cmd);
    if (line == NULL) return;

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

/* Print the command history */
void print_history() {
    for (int i = 0; i < history_len; i++) {
        printf("%d %s (pid: %d, duration: %.2f seconds)\n", i+1, history[i], pids[i], durations[i]);
    }
}

/* Check for completed background processes */
void check_background_processes() {
    for (int i = 0; i < bg_process_count; i++) {
        int status;
        pid_t result = waitpid(background_processes[i].pid, &status, WNOHANG);
        
        if (result == background_processes[i].pid) {
            printf("[Background] PID: %d finished command: %s\n", background_processes[i].pid, background_processes[i].cmd);
            free(background_processes[i].cmd); // Free command string
            // Shift remaining background processes
            for (int j = i; j < bg_process_count - 1; j++) {
                background_processes[j] = background_processes[j + 1];
            }
            bg_process_count--;
            i--; // Adjust index since we shifted
        }
    }
}

/* Execute a command using exec, supporting pipes and background processes */
void launch_command(char *cmd) {
    char *args[ARG_MAX_COUNT];
    int tokenCount = 0;

    /* Check for pipes in the command */
    char *cmd_part = strtok(cmd, "|");
    char *cmd_parts[ARG_MAX_COUNT];
    int num_parts = 0;

    while (cmd_part != NULL) {
        cmd_parts[num_parts++] = cmd_part;
        cmd_part = strtok(NULL, "|");
    }

    /* If there's no pipe, just execute the command normally */
    if (num_parts == 1) {
        execute_single_command(cmd_parts[0]);
    } else {
        execute_piped_commands(cmd_parts, num_parts);
    }
    // Check for completed background processes
    check_background_processes();
}

/* Helper function to execute a single command */
void execute_single_command(char *cmd) {
    char *args[ARG_MAX_COUNT];
    int tokenCount = 0;
    int background = 0;

    /* Check if the command ends with & (for background) */
    size_t cmd_len = strlen(cmd);
    if (cmd[cmd_len - 1] == '&') {
        background = 1;
        cmd[cmd_len - 1] = '\0';  // Remove the & character
    }

    /* Parse the command */
    char *token = strtok(cmd, " ");
    while (token != NULL && tokenCount < ARG_MAX_COUNT) {
        args[tokenCount++] = token;
        token = strtok(NULL, " ");
    }
    args[tokenCount] = NULL;

    pid_t pid = fork();
    if (pid == 0) {
        /* Child process */
        execvp(args[0], args);
        perror("exec");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        time_t start = time(NULL);
        int status;
        
        if (!background) {
            /* Wait for the foreground process */
            waitpid(pid, &status, 0);
            double duration = difftime(time(NULL), start);
            add_to_history(cmd, pid, duration);  // Pass pid instead of &status
        } else {
            /* Background process: do not wait */
            printf("[Background] PID: %d running command: %s\n", pid, cmd);
            if (bg_process_count < MAX_BACKGROUND_PROCESSES) {
                background_processes[bg_process_count++] = (BackgroundProcess){pid, strdup(cmd)};
            }
        }
    } else {
        perror("fork");
    }
}


/* Function to handle piped commands */
void execute_piped_commands(char *cmd_parts[], int num_parts) {
    int fd[2];
    pid_t pid;
    int fd_in = 0;  // Input for the next command (initially standard input)

    for (int i = 0; i < num_parts; i++) {
        pipe(fd);  // Create a pipe

        if ((pid = fork()) == 0) {
            // Child process
            dup2(fd_in, 0);  // Set input to the previous pipe
            if (i < num_parts - 1) {
                dup2(fd[1], 1); // Redirect output if not the last command
            }

            close(fd[0]);  // Close unused read end
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
            // Parent process: close the write end of the pipe
            close(fd[1]);
            fd_in = fd[0];  // Save the input for the next command
        }
    }
    wait(NULL);  // Wait for the last child
}

/* Check if a command is blank */
int is_blank(char *input) {
    int n = strlen(input);
    for (int i = 0; i < n; i++) {
        if (!isspace(input[i]))
            return 0;
    }
    return 1;
}

/* Handle built-in commands */
int handle_builtin(char *input) {
    if (strcmp(input, "exit") == 0) {
    
        return -1;
    }
    if (strcmp(input, "history") == 0) {
        print_history();
        return 0;
    }
    if (strncmp(input, "cd", 2) == 0) {
        char *dir = strtok(input + 3, " ");
        if (chdir(dir) != 0) {
            perror("cd");
        }
        return 0;
    }
    return 1;  // Not a built-in command
}



/* Main function */
int main(void) {
    init_history();
    do {
        check_background_processes(); // Check for completed background processes
        printf("simple-shell> ");
        fflush(stdout); // Ensure prompt is displayed immediately

        char *input = NULL;
        size_t len = 0;
        ssize_t nread = getline(&input, &len, stdin);
        if (nread == -1) {
            free(input);
            break;  // Handle EOF (Ctrl+D)
        }

        input[nread - 1] = '\0';  // Remove newline
        if (is_blank(input)) {
            free(input);
            continue;
        }

        int ret = handle_builtin(input);
        if (ret == -1) {
            break;  // Exit shell
        } else if (ret == 1) {
            launch_command(input);  // Launch external command
        }

        free(input);
    } while (1);

    /* Print execution details of all commands in history */
    printf("Execution summary:\n");
    print_history();

    /* Cleanup */
    for (int i = 0; i < history_len; i++) {
        free(history[i]);
    }
    free(history);
    free(pids);
    free(start_times);
    free(durations);

    // Free background process commands
    for (int i = 0; i < bg_process_count; i++) {
        free(background_processes[i].cmd);
    }

    return 0;
}