#include "shell.h"

static int is_operator(char c) {
    return c == '<' || c == '>' || c == '|';
}

// Splits the command line into tokens.
//  - words are separated by spaces/tabs
//  - '<', '>' and '|' are tokens of their own (even without spaces: ls>out)
//  - text inside "double" or 'single' quotes stays together as one word,
//    and the quote characters themselves are removed
char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace

        if (*cp == '\0') break; // Line was only whitespace

        int len = 0;
        if (is_operator(*cp)) {
            // An operator is a one-character token
            arglist[argnum][len++] = *cp++;
        } else {
            // A normal word: read until whitespace or operator
            while (*cp != '\0' && *cp != ' ' && *cp != '\t' && !is_operator(*cp)) {
                if (*cp == '"' || *cp == '\'') {
                    char quote = *cp++;                 // opening quote
                    while (*cp != '\0' && *cp != quote) {
                        if (len < ARGLEN - 1) arglist[argnum][len++] = *cp;
                        cp++;
                    }
                    if (*cp == quote) cp++;             // closing quote
                } else {
                    if (len < ARGLEN - 1) arglist[argnum][len++] = *cp;
                    cp++;
                }
            }
        }
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    // Free the slots we did not use, and mark the end of the list
    for (int i = argnum; i < MAXARGS + 1; i++) {
        free(arglist[i]);
        arglist[i] = NULL;
    }
    return arglist;
}

// Turns the flat token list into a pipeline of commands.
// Returns 0 on success, -1 on a syntax error (message already printed).
int parse_pipeline(char** arglist, pipeline_t* pl) {
    memset(pl, 0, sizeof(*pl));
    pl->count = 1;
    command_t* cmd = &pl->cmds[0];
    int argc = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        char* tok = arglist[i];

        if (strcmp(tok, "|") == 0) {
            if (argc == 0) {
                fprintf(stderr, "syntax error: missing command before '|'\n");
                return -1;
            }
            if (pl->count == MAX_CMDS) {
                fprintf(stderr, "error: too many commands in pipeline\n");
                return -1;
            }
            cmd->argv[argc] = NULL;
            cmd = &pl->cmds[pl->count++];   // start the next command
            argc = 0;
        }
        else if (strcmp(tok, "<") == 0 || strcmp(tok, ">") == 0) {
            char* next = arglist[i + 1];
            if (next == NULL || strcmp(next, "<") == 0 ||
                strcmp(next, ">") == 0 || strcmp(next, "|") == 0) {
                fprintf(stderr, "syntax error: missing file name after '%s'\n", tok);
                return -1;
            }
            if (tok[0] == '<') cmd->infile = next;
            else               cmd->outfile = next;
            i++;                            // skip the file name token
        }
        else {
            cmd->argv[argc++] = tok;
        }
    }

    if (argc == 0) {
        fprintf(stderr, "syntax error: missing command\n");
        return -1;
    }
    cmd->argv[argc] = NULL;
    return 0;
}

// Splits one input line into separate commands at ';' and '&'.
//  - A ';' or '&' inside quotes does not split.
//  - A command ended by '&' is marked as a background command.
// The line is modified in place (each separator is replaced by '\0') and
// segs[i].text points into it. Returns the number of commands, or -1 on a
// syntax error (message already printed).
int split_commands(char* line, segment_t* segs, int max) {
    int count = 0;
    char* start = line;
    char quote = 0;

    for (char* p = line; ; p++) {
        char c = *p;

        if (c != '\0') {
            if (quote != 0) {                 // inside quotes: look only for the closing quote
                if (c == quote) quote = 0;
                continue;
            }
            if (c == '"' || c == '\'') { quote = c; continue; }
            if (c != ';' && c != '&') continue;
        }

        // Here c is ';', '&' or the end of the line: [start, p) is one command
        while (start < p && (*start == ' ' || *start == '\t')) start++;
        int blank = 1;
        for (char* q = start; q < p; q++) {
            if (*q != ' ' && *q != '\t') { blank = 0; break; }
        }

        if (blank) {
            if (c != '\0') {                  // e.g. "; ls"  or  "ls ;; ls"
                fprintf(stderr, "syntax error near unexpected token '%c'\n", c);
                return -1;
            }
        } else {
            if (count == max) {
                fprintf(stderr, "error: too many commands on one line\n");
                return -1;
            }
            // remove spaces at the end of the command text
            char* e = p;
            while (e > start && (e[-1] == ' ' || e[-1] == '\t')) e--;
            *e = '\0';

            segs[count].text = start;
            segs[count].background = (c == '&');
            count++;
        }

        if (c == '\0') break;
        *p = '\0';                            // cut the string here
        start = p + 1;
    }
    return count;
}
