#include "shell.h"

static int at_exit_set = 0;
static struct termios original;

static char *prompt = NULL;
static int prompt_len = 0;

static char *prev_input_line = NULL;
static int prev_input_len;

static char *next_input_line = NULL;
static int next_input_len;

static char *input_line = NULL;
static int input_len;
static int input_cur;

static built_in_cmd commands[] = {
        {cmd_cd,   0, 1, "cd"},
        {cmd_exit, 0, 1, "exit"},
        {cmd_pwd,  0, 0, "pwd"},
        {cmd_test, 0, 2, "test"}
};

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

void update_prev_and_next_cmd(void) {
    prev_input_len = 0;
    next_input_line = 0;
    if (prev_input_line != NULL) {
        free(prev_input_line);
    }
    if(next_input_line != NULL) {
        free(next_input_line);
    }
    if (input_line != NULL) {
        prev_input_line = strdup(input_line);
        prev_input_len = input_len;
    }
}

void new_input() {
    update_prev_and_next_cmd();
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


void update_cur_input_after_arrow(void) {
    input_cur = input_len + prompt_len + 1;
}

void up() {
    if (prev_input_line != NULL) {
        next_input_line = input_line;
        next_input_len = input_len;
        input_line = prev_input_line;
        input_len = prev_input_len;
        prev_input_line = NULL;
        prev_input_len = 0;
        update_cur_input_after_arrow();
    }
}


void down() {
    if (next_input_line != NULL) {
        prev_input_line = input_line;
        prev_input_len = input_len;
        input_line = next_input_line;
        input_len = next_input_len;
        next_input_line = NULL;
        next_input_len = 0;
        update_cur_input_after_arrow();
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


int main() {
    if (!isatty(STDIN_FILENO)) {
        printf("Primitive Shell works only on TTY\n");
        return -1;
    }
    {
        char *tmp = getenv("TERM");
        if (tmp != NULL && strcasestr(tmp, "xterm") == NULL) {
            printf("Primitive Shell works only with xterm shells\n");
            return -1;
        }
    }

    while (1) {
        char *line = NULL;
        int in_loop = 1;
        if (raw_mode_on() == -1) {
            perror("raw_mode_on() failed, edit disabled");
            return -1;
        }
        prepare_prompt();
        while (in_loop) {
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
            //printf("gotchar: %d\n", c);
            switch (c) {
                case 3: // CTRL+C
                    new_input();
                    break;
                case 4: // CTRL+D
                    if (input_len == 0) {
                        line = strdup("exit");
                        in_loop = 0;
                    }
                    break;
                case 13: // ENTER
                    if (input_len == 0) {
                        line = strdup("");
                    } else {
                        line = strdup(input_line);
                    }
                    in_loop = 0;
                    break;
                case 8:
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
                                case 'A':
                                    up();
                                    break;
                                case 'B':
                                    down();
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
                    break;
                default:
                    if (isprint(c)) {
                        append(c);
                    }
            }
        }
        new_input();
        refresh();
        raw_mode_off();
        if (line != NULL && strlen(line) > 0) {
            printf("%s\n", line);
            exec_or_run(line);
        } else {
            printf("\n");
        }
        if (line != NULL) {
            free(line);
        }
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
}

int cmd_cd(const char **args) {
    char *where = (char *) args[1];
    if (where == NULL) {
        where = getenv("HOME");
    }
    if (chdir(where) < 0) {
        perror("chdir");
        return -1;
    }
    return 0;
}

int cmd_exit(const char **args) {
    if (args[1] != NULL) {
        int code = atoi(args[1]);
        exit(code);
    } else {
        exit(0);
    }
}

int cmd_test(const char **args) {
    if (args[1] != NULL) {
        const char *flag = args[1];
        if (strcmp(flag, "-e") == 0) {
            if (args[2] != NULL) {
                const char *file = args[2];
                char cwd[FILENAME_MAX];
                if (getcwd(cwd, FILENAME_MAX) == NULL) {
                    perror("getcwd failed");
                    return -1;
                }
                if (access(file, F_OK) != -1) {
                    return 0;
                } else {
                    return 1;
                }
            } else { return 1; }
        }
        // Could be other flags
    } else { return 1; }
    return 0;
}

int cmd_pwd(const char **arg) {
    if (arg == NULL)
        return -1;
    char cwd[FILENAME_MAX];
    if (getcwd(cwd, FILENAME_MAX) == NULL) {
        perror("getcwd failed");
        return -1;
    }
    printf("%s\r\n", cwd);
    return 0;
}


int exec_or_run(char *line) {
    char **sline = &line;
    parsed_command **cmds = NULL;
    int cmds_len = 0;
    while (*sline != NULL) {
        parsed_command *cmd = parse_command(sline);
        if (cmd == NULL) {
            break;
        }
        update_parsed_command(cmd);
        cmds = realloc(cmds, sizeof(parsed_command *) * (cmds_len + 1));
	if (cmds == NULL) {
	    return -1;
	}
        cmds[cmds_len++] = cmd;
    }
    if (cmds == NULL) {
        return -1;
    }
    for (int i = 0; i < cmds_len; i++) {
        if (i + 1 < cmds_len) { // we are not last command
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                goto out;
            }
            if (cmds[i] != NULL) {
                if (cmds[i]->stdout != STDOUT_FILENO)
                    close(cmds[i]->stdout);
                cmds[i]->stdout = pipefd[1];
                if (cmds[i + 1] != NULL) {
                    if (cmds[i + 1]->stdin != STDIN_FILENO)
                        close(cmds[i + 1]->stdin);
                    cmds[i + 1]->stdin = pipefd[0];
                }
            }
        }
    }

    for (int i = 0; i < cmds_len; i++) {
        if (do_fork_exec(cmds[i]) == -1) {
            fprintf(stderr, "command %s returned -1\n", cmds[i]->cmd);
            goto out; // we don't continue if something is broken
        } else {
            fprintf(stderr, "command %s returned 0\n", cmds[i]->cmd);
        }
    }

    out:
    for (int i = 0; i < cmds_len; i++) {
        if (cmds[i] != NULL)
            parsed_command_cleanup(cmds[i]);
    }
    free(cmds);
    return 0;
}

int do_fork_exec(parsed_command *cmd) {
    int (*func)(const char **) = NULL;
    for (size_t i = 0; i < sizeof(commands) / sizeof(built_in_cmd); i++) {
        if (strcmp(cmd->cmd, commands[i].command) == 0) {
            if (cmd->max_args - 1 > commands[i].max_args || cmd->max_args - 1 < commands[i].min_args) {
                fprintf(stderr, "Insuffient number of arguments (%d) for command %s\r\n", cmd->max_args,
                        commands[i].command);
                return -1;
            } else {
                func = commands[i].func;
                break;
            }
        }
    }
    fprintf(stderr, "command = [%s] ", cmd->cmd);
    fprintf(stderr, "args = %d ", cmd->max_args);
    fprintf(stderr, "stdin = %d ", cmd->stdin);
    fprintf(stderr, "stdout =%d ", cmd->stdout);
    fprintf(stderr, "\n\n");
    if (func != NULL) {
        return func((const char **) cmd->args);
    }
    int pid = fork();
    if (pid == -1) {
        perror("fork()");
        return -1;
    }
    if (pid == 0) {
        printf("execing %s\n", cmd->cmd);
        if (cmd->stdin != STDIN_FILENO) {
            dup2(cmd->stdin, STDIN_FILENO);
        }
        if (cmd->stdout != STDOUT_FILENO) {
            dup2(cmd->stdout, STDOUT_FILENO);
        }
        if (cmd->stderr != STDERR_FILENO) {
            dup2(cmd->stderr, STDERR_FILENO);
        }
        if (cmd->nargs > cmd->max_args) {
            free(cmd->args[cmd->max_args]);
            cmd->args[cmd->max_args] = NULL;
        }
        execvp(cmd->cmd, cmd->args);
        perror("execvp");
        exit(-1);
    } else {
        if (cmd->stdin != STDIN_FILENO) {
            close(cmd->stdin);
        }
        if (cmd->stdout != STDOUT_FILENO) {
            close(cmd->stdout);
        }
        if (cmd->stderr != STDERR_FILENO) {
            close(cmd->stderr);
        }
        if (cmd->background) {
            return 0;
        }
        int wstatus = 0;
        if (waitpid(pid, &wstatus, 0) == -1) {
            perror("waitpid");
            return -1;
        }
        if (WIFEXITED(wstatus)) {
            return WEXITSTATUS(wstatus);
        } else {
            return -1;
        }
    }
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
    prompt = (char *) malloc(FILENAME_MAX * 2);
    memset(prompt, 0, FILENAME_MAX * 2);
    snprintf(prompt, FILENAME_MAX * 2, "[%s %s]%c ",
             user, &cwd[cwd_idx], uid == 0 ? '#' : '$');
    prompt_len = strlen(prompt);
}
