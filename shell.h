#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <termios.h>

#define LINE_MAX_SIZE 65535

#define MAX_TOKENS 128

typedef struct {
    int (*func)(const char *args);

    int min_args;
    int max_args;
    char *command;
} built_in_cmd;

int cmd_cd(const char *);

int cmd_exit(const char *);

int cmd_pwd(const char *);

static built_in_cmd commands[] = {
        {cmd_cd,   0, 1, "cd"},
        {cmd_exit, 0, 1, "exit"},
        {cmd_pwd,  0, 0, "pwd"}
};

int exec_or_run(char *line);

void prepare_prompt(void);

int raw_mode_on(void);

int raw_mode_off(void);

void exit_function(void);

void refresh();

void new_input();

void append(char c);

void left();

void right();

void end();

void home();

void backspace();

void delete();

