# My Shell (OS Assignment 03)

A UNIX-like command-line shell written in C. It was built step by step, one
feature per Git branch, and every stage is tagged and released.

## Build and run

    make
    ./bin/myshell

Needs GNU Readline:  sudo apt install libreadline-dev

## Features

| Release | Feature |
|---------|---------|
| v1.0-base | Base shell: fork, exec, wait |
| v2 | Built-ins: exit, cd, help, jobs |
| v3 | Command history: history, !n |
| v4 | GNU Readline: tab completion and arrow-key history |
| v5 | I/O redirection (<, >) and pipes (|) |
| v6 | Command chaining (;), background jobs (&), zombie reaping |
| v7 | if / then / else / fi |
| v8 | Shell variables: NAME=value, $NAME, set |

## Examples

    FCIT> ls -l > files.txt
    FCIT> sort < files.txt
    FCIT> cat /etc/passwd | grep root
    FCIT> echo one ; sleep 2 ; echo two
    FCIT> sleep 10 &
    FCIT> jobs
    FCIT> MSG="Hello"
    FCIT> echo $MSG
    FCIT> if grep root /etc/passwd > /dev/null
    > then
    > echo found
    > else
    > echo not found
    > fi

## Project structure

    src/main.c        main loop
    src/shell.c       built-ins, history, tab completion
    src/parser.c      tokenizer, ; and & splitting, pipeline parser
    src/execute.c     fork/exec, pipes, redirection
    src/jobs.c        background jobs and zombie handling
    src/control.c     if-then-else-fi
    src/variables.c   shell variables
    include/shell.h   shared declarations
    Makefile
