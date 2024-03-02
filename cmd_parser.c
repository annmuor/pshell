#include "string.h"

#define MAX_LINE_SIZE 4096
struct parsed_command {
    char *cmd;
    char **args;
    int nargs;
    char *stdout;
    char *stderr;
    char *stdin;
    int background;
};

int parse_line(char *line, struct parsed_command ***commands, int *ncommands) {
    char buf[MAX_LINE_SIZE]; // max line size
    int bufc = 0;
    int nline = 0;
    *ncommands = 0;
    *commands = NULL;
    // let's start with 1 command
    if (line == NULL) {
        return -1;
    }
    nline = strnlen(line, MAX_LINE_SIZE);
    memset(buf, 0, MAX_LINE_SIZE);
    for (int i = 0; i < nline; i++) {
        char c = line[i];
        if (c == ' ') {

        } else if (c == '"') {

        } else if (c == '\'') {

        } else if (c == '>') {

        } else if (c == '<') {

        } else {
            buf[bufc++] = c;
        }
    }

}