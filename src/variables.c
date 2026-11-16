#include "shell.h"
#include <ctype.h>

// Shell variables are kept in a singly linked list of name/value pairs.
typedef struct var {
    char* name;
    char* value;
    struct var* next;
} var_t;

static var_t* var_head = NULL;

static int is_name_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int is_name_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

// Create the variable, or change its value if it already exists
void var_set(const char* name, const char* value) {
    for (var_t* v = var_head; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) {
            free(v->value);
            v->value = strdup(value);
            return;
        }
    }

    var_t* v = malloc(sizeof(var_t));
    v->name = strdup(name);
    v->value = strdup(value);
    v->next = NULL;

    // Add at the end, so "set" lists variables in the order they were created
    if (var_head == NULL) {
        var_head = v;
    } else {
        var_t* last = var_head;
        while (last->next != NULL) last = last->next;
        last->next = v;
    }
}

// Look in our own variables first, then in the environment ($HOME, $PATH ...).
// Returns NULL if the name is not defined anywhere.
const char* var_get(const char* name) {
    for (var_t* v = var_head; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) return v->value;
    }
    return getenv(name);
}

// The "set" built-in
void var_print_all(void) {
    for (var_t* v = var_head; v != NULL; v = v->next) {
        printf("%s=%s\n", v->name, v->value);
    }
}

// Is this word of the form NAME=value ?  (NAME = letters, digits, '_'; not starting with a digit)
static int is_assignment(const char* tok) {
    if (!is_name_start(tok[0])) return 0;
    int i = 1;
    while (is_name_char(tok[i])) i++;
    return tok[i] == '=';
}

// Handles "set" and NAME=value. Returns 1 if the command was handled here,
// 0 if it is not one of these (so the shell will fork+exec it).
int handle_variable_builtin(char** arglist) {
    if (strcmp(arglist[0], "set") == 0) {
        var_print_all();
        return 1;
    }

    // Every word must be an assignment (so  A=1 B=2  works, but  A=1 ls  does not)
    for (int i = 0; arglist[i] != NULL; i++) {
        if (!is_assignment(arglist[i])) return 0;
    }

    for (int i = 0; arglist[i] != NULL; i++) {
        char* eq = strchr(arglist[i], '=');
        char* name = strndup(arglist[i], eq - arglist[i]);
        var_set(name, eq + 1);
        free(name);
    }
    return 1;
}

// Builds a new string in which every $NAME is replaced by its value.
// An undefined variable becomes an empty string. A '$' that is not followed
// by a valid name (like "$5" or a lone "$") is kept as it is.
static char* expand_token(const char* tok) {
    size_t cap = 64, len = 0;
    char* res = malloc(cap);

    const char* p = tok;
    while (*p != '\0') {
        const char* piece;
        size_t plen;

        if (*p == '$' && is_name_start(p[1])) {
            char name[64];
            int k = 0;
            p++;                                   // skip the '$'
            while (is_name_char(*p) && k < 63) {
                name[k++] = *p++;
            }
            name[k] = '\0';

            piece = var_get(name);
            if (piece == NULL) piece = "";
            plen = strlen(piece);
        } else {
            piece = p;                             // copy one ordinary character
            plen = 1;
            p++;
        }

        while (len + plen + 1 > cap) {             // make room, then append
            cap *= 2;
            res = realloc(res, cap);
        }
        memcpy(res + len, piece, plen);
        len += plen;
    }
    res[len] = '\0';
    return res;
}

// Runs after tokenizing and before executing: goes through every word and
// replaces $NAME with the variable's value. A word that was only a variable
// which expands to nothing (e.g. $UNDEFINED) is removed from the list.
void expand_variables(char** arglist) {
    int out = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        char* tok = arglist[i];

        if (strchr(tok, '$') == NULL) {            // nothing to expand
            arglist[out++] = tok;
            continue;
        }

        char* expanded = expand_token(tok);
        free(tok);                                 // old word is no longer needed
        if (expanded[0] == '\0') {
            free(expanded);                        // drop empty word
        } else {
            arglist[out++] = expanded;
        }
    }
    arglist[out] = NULL;
}
