#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "
#define HISTORY_SIZE 20

// Function prototypes
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** arglist);

// Own history (used by the history built-in and !n)
void hist_add(const char* cmdline);
char* hist_get(int n);   // n is 1-based, returns NULL if out of range
void hist_print(void);

// Tab completion (hooked into readline)
char** shell_completion(const char* text, int start, int end);

#endif // SHELL_H
