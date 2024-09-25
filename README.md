# Simple Shell

## Overview

This project implements a simple Unix-like shell in C, capable of executing commands, managing background processes, and maintaining a history of executed commands. It supports both built-in commands like `exit`, `cd`, and `history`, as well as external commands using `execvp()`.

## Features

- **Command Execution**: Execute both single commands and piped commands.
- **Background Processes**: Commands can be executed in the background by appending `&` at the end.
- **Command History**: Keeps track of the last 100 executed commands, along with their process ID and execution duration.
- **Built-in Commands**:
  - `exit`: Exit the shell.
  - `history`: Display the list of executed commands.
  - `cd [directory]`: Change the current working directory.

## Installation

To compile the shell, use the following commands:

```bash
gcc -o shell shell.c
```

## Usage

Run the shell by executing the compiled binary:

```bash
./shell
```

### Supported Commands

- **Single Commands**: Execute standard commands like `ls`, `echo`, `cat`, `uniq`,  etc.
  ```bash
  ls -l
  ```
  
- **Piped Commands**: Combine multiple commands using pipes (`|`).
  ```bash
  ls -l | grep txt | wc -l
  ```

- **Background Commands**: Run a command in the background using `&`.
  ```bash
  sleep 10 &
  ```

- **Command History**: View command history with `history`.
  ```bash
  history
  ```

### Background Process Management

The shell supports running multiple commands in the background. It checks for completed background processes and notifies the user when they finish.

Example:
```bash
sleep 10 &
[Background] PID: 1234 running command: sleep 10
```

When the background process completes:
```bash
[Background] PID: 1234 finished command: sleep 10
```

### Exit the Shell

To exit the shell, type:
```bash
exit
```

## History

The shell maintains a history of up to 100 commands. Each entry in the history includes:
- Command string
- Process ID (PID) of the command
- Execution duration in seconds

Example history output:
```
1 ls -l (pid: 1234, duration: 0.01 seconds)
2 sleep 5 (pid: 1235, duration: 5.00 seconds)
```

## Error Handling

The shell handles common errors like:
- Command not found
- Directory not found for `cd` command
- Memory allocation failures

## Shell Limitations

This shell implementation provides basic functionality for executing commands, supporting both single and piped commands, as well as background processes. However, certain advanced shell features are not supported in this version. Below is a breakdown of the unsupported features and why they don't work in the current implementation:

---

### 1. **Input/Output Redirection (`>`, `>>`, `<`)**
   - **Examples**:
     - `ls > output.txt` (redirects output to a file).
     - `cat < input.txt` (reads input from a file).
     - `echo "Hello" >> output.txt` (appends output to a file).
   - **Why Unsupported**:
     - The shell doesn't have logic to parse and handle redirection operators (`>`, `>>`, `<`). These operators are typically used to manipulate file descriptors (for input or output redirection), which would require additional parsing and system calls like `dup2()` to redirect standard input/output. 
---

### 2. **Background Process Control (`jobs`, `fg`, `bg`)**
   - **Examples**:
     - `jobs` (lists all background jobs).
     - `fg %1` (brings a background process to the foreground).
     - `bg %1` (resumes a suspended process in the background).
   - **Why Unsupported**:
     - This shell lacks advanced job control mechanisms. While it supports launching background processes using the `&` operator, there’s no functionality for tracking, listing, or controlling background jobs beyond checking if they have completed using `waitpid()`.
---

### 3. **Command Substitution (`$()`, Backticks)**
   - **Examples**:
     - `echo $(date)` (runs a command and substitutes its output).
     - `` echo `date` `` (runs a command in backticks and substitutes the output).
   - **Why Unsupported**:
     - The shell does not parse and replace command substitution syntax (`$()` or backticks) within a command string. Handling this requires an additional parsing phase where embedded commands are executed and their output is substituted into the original command.
---

### 4. **Environment Variable Expansion**
   - **Examples**:
     - `echo $HOME` (prints the home directory path).
     - `cd $MY_CUSTOM_PATH` (changes the directory to a path stored in a custom environment variable).
   - **Why Unsupported**:
     - The shell doesn’t expand environment variables (like `$HOME` or custom variables like `$MY_CUSTOM_PATH`). Environment variable expansion requires parsing commands to detect `$`-prefixed variables and substituting them with their values from the environment (retrieved using `getenv()`).
---

### 5. **Shell Scripting Features**
   - **Unsupported Features**:
     - Conditional execution (`&&`, `||`): `command1 && command2`, `command1 || command2`.
     - Loops and conditionals (e.g., `for`, `while`, `if`).
     - Alias handling (`alias ls='ls -la'`).
   - **Why Unsupported**:
     - This shell is designed as a simple command executor without support for scripting features like conditionals or loops. Implementing such features would require an extensive shell parser and evaluator.

## Group Members 

- Sahil Amrawat (2023462) 
- Vikas Meena (202359)
