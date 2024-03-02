#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>

#define LINE_MAX_SIZE 65535
#define MAX_LINE_SIZE 4096
#define MAX_TOKENS 128

typedef struct {
    int (*func)(const char **args);

    int min_args;
    int max_args;
    char *command;
} built_in_cmd;

typedef struct {
    char *cmd;
    char **args;
    int nargs;
    int max_args;
    int stdout;
    int stderr;
    int stdin;
    int background;
} parsed_command;

int cmd_cd(const char **);

int cmd_exit(const char **);

int cmd_test(const char **);

int cmd_pwd(const char **);

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

parsed_command *parse_command(char **);

void update_parsed_command(parsed_command *);

void parsed_command_cleanup(parsed_command *);

int do_fork_exec(parsed_command *cmd);