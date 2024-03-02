#include "shell.h"

static int at_exit_set = 0;
static struct termios original;

static char *prompt = NULL;
static int prompt_len = 0;

static char *input_line = NULL;
static int input_len;
static int input_cur;


void refresh() {
    write(STDOUT_FILENO, "\x1b[2K\x1b[0G", 8);
    if (prompt != NULL)
        write(STDOUT_FILENO, prompt, prompt_len);
    write(STDOUT_FILENO, input_line, input_len);
    if (input_cur > input_len + prompt_len + 1) {
        input_cur = input_len + prompt_len + 1;
    }
    if (input_cur <= prompt_len) {
        input_cur = prompt_len + 1;
    }
    printf("\x1b[%dG", input_cur);
    fflush(stdout);
}

void new_input() {
    if (input_line != NULL) {
        free(input_line);
    }
    input_line = malloc(LINE_MAX_SIZE);
    memset(input_line, 0, LINE_MAX_SIZE);
    input_len = 0;
    input_cur = prompt_len + 1;
}

void append(char c) {
    if (input_line == NULL) {
        new_input();
    }
    int real_cur = input_cur - prompt_len;
    // magic here!
    if (real_cur > input_len) {
        input_line[input_len++] = c;
        input_cur++;
    } else { // this is middle insert
        char *tmp = strdup(&input_line[real_cur]);
        input_line[real_cur] = c;
        strncpy(&input_line[real_cur + 1], tmp, strlen(tmp));
        input_len++;
        input_cur++;
        free(tmp);
    }
}

void left() {
    input_cur--;
}

void right() {
    input_cur++;
}

void end() {
    input_cur = prompt_len + input_len + 1;
}

void home() {
    input_cur = prompt_len + 1;
}

void backspace() {
    int real_cur = input_cur - prompt_len;
    if (real_cur <= 1) {
        return;
    }
    char *tmp = strdup(&input_line[real_cur - 1]);
    memset(&input_line[real_cur - 2], 0, strlen(tmp) + 1);
    strncpy(&input_line[real_cur - 2], tmp, strlen(tmp));
    free(tmp);
    input_len--;
    input_cur--;
}

void delete() {
    int real_cur = input_cur - prompt_len;
    if (real_cur > input_len) {
        return;
    }
    char *tmp = strdup(&input_line[real_cur]);
    memset(&input_line[real_cur - 1], 0, strlen(tmp) + 1);
    strncpy(&input_line[real_cur - 1], tmp, strlen(tmp));
    free(tmp);
    input_len--;
    input_cur--;
}

int main(int argc, char **argv, char **env) {
    if (!isatty(STDIN_FILENO)) {
        printf("Primitive Shell works only on TTY\n");
        return -1;
    }
    if (strncmp(getenv("TERM"), "xterm", 5) != 0) {
        printf("Primitive Shell works only with xterm shells\n");
        return -1;
    }

    while (1) {
        char *line = NULL;
        if (raw_mode_on() == -1) {
            perror("raw_mode_on() failed, edit disabled");
            return -1;
        }
        prepare_prompt();
        while (1) {
            char c;
            ssize_t n;
            refresh();
            if ((n = read(STDIN_FILENO, &c, 1)) < 0) {
                perror("read()");
                return -1;
            }
            if (n == 0) {
                continue;
            }
            switch (c) {
                case 3: // CTRL+C
                    new_input();
                    break;
                case 4: // CTRL+D
                    if (input_len == 0) {
                        line = strdup("exit");
                        goto line_ready;
                    }
                    break;
                case 13: // ENTER
                    if(input_len == 0) {
                      line = strdup("");
                    } else {
                      line = strdup(input_line);
                    }
                    goto line_ready;
                case 127: // BACKSPACE
                    backspace();
                    break;
                case 27: // ESCAPE STUFF
                {
                    char code1, code2;
                    if (read(STDIN_FILENO, &code1, 1) != 1) break;
                    if (read(STDIN_FILENO, &code2, 1) != 1) break;
                    if (code1 == '[') {
                        if (code2 >= '0' && code2 <= '9') {
                            if (read(STDIN_FILENO, &code1, 1) != 1) break;
                            if (code1 == '~' && code2 == '3') {
                                delete();
                            }
                        } else {
                            switch (code2) {
                                case 'C': // Right
                                    right();
                                    break;
                                case 'D':
                                    left();
                                    break;
                                case 'H':
                                    home();
                                    break;
                                case 'F':
                                    end();
                                    break;
                                default:
                                    break;
                            }
                        }
                    } else if (code1 == 'O') {
                        switch (code2) {
                            case 'H':
                                home();
                                break;
                            case 'F':
                                end();
                                break;
                            default:
                                break;
                        }
                    }
                }

                default:
                    if (isprint(c)) {
                        append(c);
                    }
            }
        }
        line_ready:
        new_input();
        refresh();
        raw_mode_off();
        printf("%s\n", line);
        if(strlen(line) > 0) {
          exec_or_run(line);
        }
        free(line);
    }
}

int raw_mode_on(void) {
    if (!at_exit_set) {
        if (tcgetattr(STDIN_FILENO, &original) != 0) {
            return -1;
        }
        if (atexit(exit_function) != 0) {
            return -1;
        }
        at_exit_set = 1;
    }
    struct termios raw = original;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return -1;
    }
    return 0;
}

int raw_mode_off(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original) != 0) {
        return -1;
    }
    return 0;
}

void exit_function(void) {
    raw_mode_off();
};

int cmd_cd(const char *where) {
    if (where == NULL) {
        where = getenv("HOME");
    }
    if (chdir(where) < 0) {
        perror("chdir");
        return -1;
    }
    return 0;
}

int cmd_exit(const char *any) {
    if (any != NULL) {
        int code = atoi(any);
        exit(code);
    } else {
        exit(0);
    }
}

int cmd_pwd(const char *_any) {
    char cwd[FILENAME_MAX];
    if (getcwd(cwd, FILENAME_MAX) == NULL) {
        perror("getcwd failed");
        return -1;
    }
    printf("%s\r\n", cwd);
    return 0;
}

int exec_or_run(char *line) {
    char **tokens = malloc(sizeof(char *) * MAX_TOKENS);
    size_t token_count = 0;
    char **cur = tokens;
    char *first = strtok(line, " ");
    int ret = 0;
    memset(tokens, 0, sizeof(char *) * MAX_TOKENS);
    while (first != NULL) {
        *cur = strdup(first);
        token_count++;
        cur++;
        if (token_count == MAX_TOKENS) {
            break;
        }
        first = strtok(NULL, " ");
    }

    for (int i = 0; i < sizeof(commands) / sizeof(built_in_cmd); i++) {
        if (strncmp(*tokens, commands[i].command, strlen(*tokens)) == 0) {
            if (token_count - 1 > commands[i].max_args || token_count - 1 < commands[i].min_args) {
                fprintf(stderr, "Insuffient number of arguments (%d) for command %s\r\n", tokens - 1,
                        commands[i].command);
                ret = -1;
                goto out;
            }
            char *arg = NULL;
            for (int i = 1; i < token_count; i++) {
                int len = strlen(tokens[i]);
                if (arg == NULL) {
                    arg = malloc(len + 1);
                    memset(arg, 0, len + 1);
                    strncpy(arg, tokens[i], len);
                } else {
                    int arg_len = strlen(arg);
                    arg = realloc(arg, arg_len + len + 2);
                    arg[arg_len] = ' ';
                    strncpy(&arg[arg_len + 1], tokens[i], len);
                    arg[arg_len + len + 1] = 0;
                }
            }
            ret = commands[i].func(arg);
            free(arg);
            goto out;
        }
    }
    // now exec the command
    int pid = fork();
    if (pid == 0) {
        execvp(*tokens, tokens);
        perror("execvp failed");
        exit(-1);
    } else {
        if (pid == waitpid(pid, NULL, 0)) {
            goto out;
        }
    }
    out:
    for (int i = 0; i < token_count; i++)
        free(tokens[i]);
    free(tokens);

    return ret;
}

void prepare_prompt(void) {
    char cwd[FILENAME_MAX];
    int cwd_idx = 0;
    char user[128];
    int uid = getuid();
    if (getcwd(cwd, FILENAME_MAX) == NULL) {
        strcpy(cwd, "-");
    } else if (strcmp(getenv("HOME"), cwd) == 0) {
        strcpy(cwd, "~");
    }
    if (strlen(cwd) > 1) {
        for (cwd_idx = strlen(cwd) - 1; cwd_idx >= 0; cwd_idx--) {
            if (cwd[cwd_idx] == '/') {
                cwd_idx++;
                break;
            }
        }
    }
    if (getlogin_r(user, 128) != 0) {
        strcpy(user, "-");
    }
    if (prompt != NULL) {
        free(prompt);
    }
    prompt = (char *) malloc(FILENAME_MAX);
    memset(prompt, 0, FILENAME_MAX);
    snprintf(prompt, FILENAME_MAX, "[%s %s]%c ",
             user, &cwd[cwd_idx], uid == 0 ? '#' : '$');
    prompt_len = strlen(prompt);
}
