#include "shell.h"

// Runs  cmd1 | cmd2 | ... | cmdN  (N may be 1, i.e. no pipe at all)
// Returns the exit status of the LAST command.
int execute_pipeline(pipeline_t* pl) {
    int n = pl->count;
    pid_t pids[MAX_CMDS];
    int prev_read = -1;   // read end of the previous pipe (-1 = none)

    for (int i = 0; i < n; i++) {
        command_t* cmd = &pl->cmds[i];

        // Every command except the last one gets a pipe to the next command
        int fd[2] = {-1, -1};
        if (i < n - 1) {
            if (pipe(fd) < 0) {
                perror("pipe failed");
                exit(1);
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pid == 0) {   // ---------- Child ----------
            // Input comes from the previous pipe
            if (prev_read != -1) {
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            // Output goes into the new pipe
            if (fd[1] != -1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                close(fd[0]);   // the child never reads from its own output pipe
            }
            // '<' and '>' override the pipe
            if (cmd->infile != NULL) {
                int f = open(cmd->infile, O_RDONLY);
                if (f < 0) {
                    perror(cmd->infile);
                    exit(1);
                }
                dup2(f, STDIN_FILENO);
                close(f);
            }
            if (cmd->outfile != NULL) {
                int f = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (f < 0) {
                    perror(cmd->outfile);
                    exit(1);
                }
                dup2(f, STDOUT_FILENO);
                close(f);
            }

            execvp(cmd->argv[0], cmd->argv);
            perror("Command not found");   // runs only if execvp fails
            exit(1);
        }

        // ---------- Parent ----------
        pids[i] = pid;
        if (prev_read != -1) close(prev_read);   // done with the old pipe
        if (fd[1] != -1) close(fd[1]);           // parent must not hold the write end
        prev_read = fd[0];                       // next child reads from here
    }

    // Wait for every child; remember the status of the last one
    int status, last_status = 0;
    for (int i = 0; i < n; i++) {
        waitpid(pids[i], &status, 0);
        if (i == n - 1) {
            last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    }
    return last_status;
}
