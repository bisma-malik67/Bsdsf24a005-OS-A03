#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    // Tell readline to use our completion function when TAB is pressed
    rl_attempted_completion_function = shell_completion;

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

        // Store non-empty commands in both histories
        if (cmdline[0] != '\0') {
            hist_add(cmdline);
            add_history(cmdline);
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            pipeline_t pl;

            if (parse_pipeline(arglist, &pl) == 0) {
                // A built-in runs inside the shell itself, but only when it
                // is a single plain command (no pipe). Otherwise fork+exec.
                if (pl.count == 1 && handle_builtin(pl.cmds[0].argv) == 1) {
                    // handled
                } else {
                    execute_pipeline(&pl);
                }
            }

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
