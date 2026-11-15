#include "shell.h"

int main() {
    char* cmdline;

    // Tell readline to use our completion function when TAB is pressed
    rl_attempted_completion_function = shell_completion;

    while (1) {
        jobs_reap();                     // collect finished background jobs before the prompt
        cmdline = readline(PROMPT);
        if (cmdline == NULL) break;      // Ctrl+D
        jobs_reap();                     // ...and again, in case one finished while we waited

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

        // Store non-empty commands in both histories (the whole line, once)
        if (cmdline[0] != '\0') {
            hist_add(cmdline);
            add_history(cmdline);
        }

        // A normal line, or a multi-line if ... fi block
        process_input(cmdline);

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
