#include "shell.h"

#define MAX_JOBS 64

typedef struct {
    pid_t pid;      // 0 means this slot is free
    char* cmd;      // the command text, for display
} job_t;

static job_t jobs[MAX_JOBS];

// Remember a new background job and announce it, e.g.  [1] 4321
void job_add(pid_t pid, const char* cmd) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].cmd = strdup(cmd);
            printf("[%d] %d\n", i + 1, pid);
            return;
        }
    }
    fprintf(stderr, "too many background jobs, this one is not tracked\n");
}

// Collect every finished child WITHOUT blocking (this removes zombies).
// WNOHANG makes waitpid() return 0 at once if nobody has finished yet.
void jobs_reap(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].pid == pid) {
                printf("[%d]  Done    %s\n", i + 1, jobs[i].cmd);
                free(jobs[i].cmd);
                jobs[i].cmd = NULL;
                jobs[i].pid = 0;
                break;
            }
        }
    }
}

// The "jobs" built-in: list the background jobs that are still running
void jobs_print(void) {
    jobs_reap();    // so that jobs which already finished are not listed
    int any = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid != 0) {
            printf("[%d]  %d  Running  %s &\n", i + 1, jobs[i].pid, jobs[i].cmd);
            any = 1;
        }
    }
    if (!any) {
        printf("No background jobs.\n");
    }
}
