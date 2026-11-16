#include "shell.h"
#include <dirent.h>

// ---------- History storage ----------
static char* hist_buf[HISTORY_SIZE];
static int hist_count = 0;

void hist_add(const char* cmdline) {
    if (hist_count == HISTORY_SIZE) {
        // History is full: drop the oldest, shift everything up by one
        free(hist_buf[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            hist_buf[i - 1] = hist_buf[i];
        }
        hist_count--;
    }
    hist_buf[hist_count++] = strdup(cmdline);
}

char* hist_get(int n) {
    if (n < 1 || n > hist_count) {
        return NULL;
    }
    return hist_buf[n - 1];
}

void hist_print(void) {
    for (int i = 0; i < hist_count; i++) {
        printf("%d  %s\n", i + 1, hist_buf[i]);
    }
}

// ---------- Tab completion ----------
static const char* builtin_names[] = {
    "exit", "cd", "help", "jobs", "history", "set", NULL
};

#define MAX_MATCHES 4096
static char* matches[MAX_MATCHES];
static int match_count, match_index;

static int already_have(const char* name) {
    for (int i = 0; i < match_count; i++) {
        if (strcmp(matches[i], name) == 0) return 1;
    }
    return 0;
}

// readline calls this again and again. state == 0 means "new completion".
// Each call returns the next match, or NULL when there are no more.
static char* command_generator(const char* text, int state) {
    if (state == 0) {
        match_count = 0;
        match_index = 0;
        size_t len = strlen(text);

        // 1) built-in commands
        for (int i = 0; builtin_names[i] != NULL; i++) {
            if (strncmp(builtin_names[i], text, len) == 0 && match_count < MAX_MATCHES) {
                matches[match_count++] = strdup(builtin_names[i]);
            }
        }

        // 2) executables found in every directory of $PATH
        char* path = getenv("PATH");
        if (path != NULL) {
            char* pathcopy = strdup(path);   // strtok modifies its input
            char* dir = strtok(pathcopy, ":");
            while (dir != NULL) {
                DIR* d = opendir(dir);
                if (d != NULL) {
                    struct dirent* e;
                    while ((e = readdir(d)) != NULL && match_count < MAX_MATCHES) {
                        if (strncmp(e->d_name, text, len) != 0) continue;
                        if (already_have(e->d_name)) continue;

                        char full[1024];
                        snprintf(full, sizeof(full), "%s/%s", dir, e->d_name);
                        if (access(full, X_OK) == 0) {
                            matches[match_count++] = strdup(e->d_name);
                        }
                    }
                    closedir(d);
                }
                dir = strtok(NULL, ":");
            }
            free(pathcopy);
        }
    }

    if (match_index < match_count) {
        return matches[match_index++];   // readline will free() it
    }
    return NULL;
}

// Called by readline when TAB is pressed.
//  start == 0 and no '/' in the word -> we are completing a COMMAND name
//  otherwise -> return NULL so readline does its default FILENAME completion
char** shell_completion(const char* text, int start, int end) {
    (void)end;
    if (start == 0 && strchr(text, '/') == NULL) {
        rl_attempted_completion_over = 1; // don't fall back to filenames
        return rl_completion_matches(text, command_generator);
    }
    return NULL;
}

// ---------- Built-in commands ----------
// Returns 1 if arglist[0] was a built-in (and it has been handled),
// returns 0 if it is not a built-in (so main should call execute()).
int handle_builtin(char** arglist) {
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        // "cd" with no argument goes to the HOME directory
        char* target = arglist[1];
        if (target == NULL) {
            target = getenv("HOME");
            if (target == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
        }
        if (chdir(target) != 0) {
            perror("cd failed");
        }
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  exit        - Terminate the shell\n");
        printf("  cd <dir>    - Change the current working directory\n");
        printf("  help        - Display this help message\n");
        printf("  jobs        - List running background jobs\n");
        printf("  history     - Show the last %d commands\n", HISTORY_SIZE);
        printf("  !n          - Re-run the n-th command from history\n");
        printf("  set         - Show all shell variables\n");
        printf("  NAME=value  - Create or change a shell variable\n");
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        jobs_print();
        return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        hist_print();
        return 1;
    }

    return handle_variable_builtin(arglist);  // "set" and NAME=value, otherwise 0
}
