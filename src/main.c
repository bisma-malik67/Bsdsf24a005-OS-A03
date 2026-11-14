#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

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

        // Store non-empty commands in history
        if (cmdline[0] != '\0') {
            hist_add(cmdline);
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
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
