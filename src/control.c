#include "shell.h"

#define MAX_BLOCK_LINES 100

// ---------- small helpers ----------

// Skip leading spaces/tabs
static const char* skip_ws(const char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static int is_blank(const char* s) {
    return *skip_ws(s) == '\0';
}

// Is the whole line (ignoring spaces around it) exactly the keyword?
// Used for "then", "else", "fi" which must stand alone on their line.
static int is_keyword(const char* line, const char* kw) {
    line = skip_ws(line);
    size_t n = strlen(kw);
    if (strncmp(line, kw, n) != 0) return 0;
    return is_blank(line + n);
}

// Does the line start with the word "if"?
static int is_if_line(const char* line) {
    line = skip_ws(line);
    if (strncmp(line, "if", 2) != 0) return 0;
    return line[2] == '\0' || line[2] == ' ' || line[2] == '\t';
}

// ---------- running ordinary lines ----------

// Runs one line (it may contain ';' and '&') using the normal
// fork-exec-wait logic. Returns the exit status of the last command
// (0 = success, non-zero = failure).
int execute_line(char* line) {
    segment_t segs[MAX_SEGMENTS];
    int nseg = split_commands(line, segs, MAX_SEGMENTS);
    if (nseg < 0) return 1;                      // syntax error

    int status = 0;
    for (int s = 0; s < nseg; s++) {
        char** arglist = tokenize(segs[s].text);
        if (arglist == NULL) continue;
        expand_variables(arglist);                   // replace $NAME by its value
        if (arglist[0] == NULL) {                    // everything expanded to nothing
            free(arglist);
            continue;
        }

        pipeline_t pl;
        if (parse_pipeline(arglist, &pl) != 0) {
            status = 1;
        } else if (pl.count == 1 && handle_builtin(pl.cmds[0].argv) == 1) {
            status = 0;                          // built-in handled in the shell itself
        } else {
            status = execute_pipeline(&pl, segs[s].text, segs[s].background);
        }

        for (int i = 0; arglist[i] != NULL; i++) free(arglist[i]);
        free(arglist);
    }
    return status;
}

// ---------- if / then / else / fi ----------

static int run_if(char** lines, int n);

// Runs n lines one after another. A nested "if" is found, its matching
// "fi" is located, and the whole inner block is handed to run_if().
static int run_block(char** lines, int n) {
    int status = 0;
    for (int i = 0; i < n; i++) {
        if (is_if_line(lines[i])) {
            int depth = 0, j = i;
            for (; j < n; j++) {
                if (is_if_line(lines[j])) depth++;
                else if (is_keyword(lines[j], "fi") && --depth == 0) break;
            }
            status = run_if(lines + i, j - i + 1);
            i = j;
        } else {
            status = execute_line(lines[i]);
        }
    }
    return status;
}

// lines[0] is "if <command>", lines[n-1] is "fi".
// In between:  then / commands / [else / commands]
static int run_if(char** lines, int n) {
    const char* cond = skip_ws(lines[0]) + 2;    // text after the word "if"
    cond = skip_ws(cond);
    if (*cond == '\0') {
        fprintf(stderr, "syntax error: 'if' needs a command\n");
        return 1;
    }
    if (n < 3 || !is_keyword(lines[1], "then")) {
        fprintf(stderr, "syntax error: expected 'then' after the if command\n");
        return 1;
    }

    // Find the "else" that belongs to THIS if (not to a nested one)
    int else_idx = -1, depth = 0;
    for (int i = 2; i < n - 1; i++) {
        if (is_if_line(lines[i])) depth++;
        else if (is_keyword(lines[i], "fi")) depth--;
        else if (depth == 0 && is_keyword(lines[i], "else")) {
            else_idx = i;
            break;
        }
    }

    int then_start = 2;
    int then_end   = (else_idx == -1) ? n - 1 : else_idx;   // end is exclusive
    int else_start = else_idx + 1;
    int else_end   = n - 1;

    // Run the command after "if" and look at its exit status
    int status = execute_line((char*)cond);

    if (status == 0) {
        return run_block(lines + then_start, then_end - then_start);
    } else if (else_idx != -1) {
        return run_block(lines + else_start, else_end - else_start);
    }
    return 0;    // condition failed and there is no else block
}

// Reads the remaining lines of an if-block (with the "> " prompt) until the
// matching "fi". The first line is already known. Returns the number of
// lines stored in lines[], or -1 on error. The caller must free() each line.
static int read_if_block(const char* first, char** lines) {
    int n = 0, depth = 1;
    lines[n++] = strdup(first);

    while (depth > 0) {
        char* line = readline("> ");
        if (line == NULL) {                      // Ctrl+D before "fi"
            fprintf(stderr, "syntax error: unexpected end of input (missing 'fi')\n");
            goto fail;
        }
        if (is_blank(line)) {                    // ignore empty lines
            free(line);
            continue;
        }
        if (n == MAX_BLOCK_LINES) {
            fprintf(stderr, "error: if-block is too long\n");
            free(line);
            goto fail;
        }
        if (is_if_line(line)) depth++;
        else if (is_keyword(line, "fi")) depth--;
        lines[n++] = line;
    }
    return n;

fail:
    for (int i = 0; i < n; i++) free(lines[i]);
    return -1;
}

// Entry point used by main(): runs a normal line, or reads and runs a
// whole if-block when the line starts with "if".
int process_input(char* line) {
    if (is_if_line(line)) {
        char* lines[MAX_BLOCK_LINES];
        int n = read_if_block(line, lines);
        if (n < 0) return 1;

        int status = run_if(lines, n);
        for (int i = 0; i < n; i++) free(lines[i]);
        return status;
    }

    if (is_keyword(line, "then") || is_keyword(line, "else") || is_keyword(line, "fi")) {
        fprintf(stderr, "syntax error: unexpected '%s'\n", skip_ws(line));
        return 1;
    }
    return execute_line(line);
}
