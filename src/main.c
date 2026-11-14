#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    // Tell readline to use our completion function when TAB is pressed
    rl_attempted_completion_function = shell_completion;

    // readline() prints the prompt, handles editing/arrow keys, returns the line
    // It returns NULL on Ctrl+D (EOF)
    while ((cmdline = readline(PROMPT)) != NULL) {

        // Handle !n BEFORE tokenizing and BEFORE adding to history
        if (cmdline[0] == '!') {
            int n = atoi(cmdline + 1);
            char* old = hist_get(n);
            if (old == NULL) {
                printf("history: !%s: event not found\n", cmdline + 1);
                free(cmdline);
                continue;
            }
            free(cmdline);
            cmdline = strdup(old);   // replace "!n" with the stored command
            printf("%s\n", cmdline); // show what is going to run
        }

        // Store non-empty commands in both histories:
        //  - hist_add()    : our own array (history built-in, !n)
        //  - add_history() : readline's list (Up/Down arrow keys)
        if (cmdline[0] != '\0') {
            hist_add(cmdline);
            add_history(cmdline);
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            // Run external command only if it is NOT a built-in
            if (handle_builtin(arglist) == 0) {
                execute(arglist);
            }

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);   // readline() uses malloc, so free() is correct
    }

    printf("\nShell exited.\n");
    return 0;
}
