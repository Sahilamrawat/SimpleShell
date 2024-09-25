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

## Group Members 

- Sahil Amrawat (2023462) 
- Vikas Meena (202359)
