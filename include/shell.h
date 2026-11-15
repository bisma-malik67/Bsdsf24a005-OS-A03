#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 30
#define ARGLEN 128
#define MAX_CMDS 10
#define MAX_SEGMENTS 32
#define PROMPT "FCIT> "
#define HISTORY_SIZE 20

// One command of a pipeline, e.g.  sort -r < in.txt > out.txt
typedef struct {
    char* argv[MAXARGS + 1];  // NULL-terminated; points into arglist (not owned)
    char* infile;             // file after '<', or NULL
    char* outfile;            // file after '>', or NULL
} command_t;

// A whole pipeline:  cmd1 | cmd2 | cmd3
typedef struct {
    command_t cmds[MAX_CMDS];
    int count;                // number of commands
} pipeline_t;

// One piece of a command line that was split at ';' or '&'
typedef struct {
    char* text;               // the command text (points into the command line)
    int background;           // 1 if it was ended by '&'
} segment_t;

// Parsing
int split_commands(char* line, segment_t* segs, int max);
char** tokenize(char* cmdline);
int parse_pipeline(char** arglist, pipeline_t* pl);

// Execution
int execute_pipeline(pipeline_t* pl, const char* cmdtext, int background);
int handle_builtin(char** arglist);

// Background jobs
void job_add(pid_t pid, const char* cmd);
void jobs_reap(void);
void jobs_print(void);

// Own history (used by the history built-in and !n)
void hist_add(const char* cmdline);
char* hist_get(int n);   // n is 1-based, returns NULL if out of range
void hist_print(void);

// Tab completion (hooked into readline)
char** shell_completion(const char* text, int start, int end);

#endif // SHELL_H
