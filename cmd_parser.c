#include "shell.h"

parsed_command *parse_command(char **line) {
    if (line == NULL || *line == NULL)
        return NULL;
    int line_len = strlen(*line);
    char *current = *line;
    char buf[MAX_LINE_SIZE];
    int bufc = 0;
    parsed_command *command = malloc(sizeof(parsed_command));
    memset(buf, 0, MAX_LINE_SIZE);
    memset(command, 0, sizeof(parsed_command));
    if (command == NULL) {
        return NULL;
    }
    command->stdout = STDOUT_FILENO;
    command->stderr = STDERR_FILENO;
    command->stdin = STDIN_FILENO;
    command->max_args = 0;

    int in_dquote = 0, in_squote = 0, in_slash = 0;
    int in_loop = 1;
    int i = 0;
    for (; i < line_len && in_loop; i++) {
        char c = current[i];
        switch (c) {
            case ' ':
                if (in_slash) {
                    buf[bufc++] = ' ';
                    in_slash = 0;
                } else if (in_dquote || in_squote) {
                    buf[bufc++] = ' ';
                } else {
                    if (bufc != 0) {
                        if (command->cmd == NULL) {
                            command->cmd = strdup(buf);
                        }
                        // zero arg is program itself
                        command->args = realloc(command->args, sizeof(char *) * (command->nargs + 1));
                        command->args[command->nargs++] = strdup(buf);
                        memset(buf, 0, MAX_LINE_SIZE);
                        bufc = 0;
                    }
                }
                break;
            case '|':
                in_loop = 0;
                break;
            case '"':
                if (in_slash) {
                    buf[bufc++] = '"';
                    in_slash = 0;
                } else if (in_squote) {
                    buf[bufc++] = '"';
                } else if (in_dquote) {
                    in_dquote = 0;
                } else {
                    in_dquote = 1;
                }
                break;
            case '\\':
                if (in_slash) {
                    buf[bufc++] = '\\';
                    in_slash = 0;
                } else {
                    in_slash = 1;
                }
                break;
            case '\'':
                if (in_slash) {
                    buf[bufc++] = '\'';
                    in_slash = 0;
                } else if (in_dquote) {
                    buf[bufc++] = '\'';
                } else if (in_squote) {
                    in_squote = 0;
                } else {
                    in_squote = 1;
                }
                break;
            default:
                buf[bufc++] = c;
        }
    }
    if (i == line_len) {
        *line = NULL;
    } else {
        *line = &current[i + 1];
    }
    if (bufc > 0) {
        if (command->cmd == NULL) {
            command->cmd = strdup(buf);
        } else {
            command->args = realloc(command->args, sizeof(char *) * (command->nargs + 1));
            command->args[command->nargs++] = strdup(buf);
        }
    }
    command->args = realloc(command->args, sizeof(char *) * (command->nargs + 1));
    command->args[command->nargs] = NULL;
    return command;
}

void update_parsed_command(parsed_command *cmd) {
    int wait_stdout = 0, wait_stderr = 0, wait_stdin = 0;
    int started_special = 0;
    int mode = 0; // 0 - write, 1 = append
    for (int i = 0; i < cmd->nargs; i++) {
        const char *arg = cmd->args[i];
        if (arg == NULL)
            continue;
        if (strcmp(arg, "&") == 0) {
            cmd->background = 1;
            started_special = 1;
        } else if (strncmp(arg, ">>", 2) == 0) {
            started_special = 1;
            mode = O_CREAT | O_APPEND | O_WRONLY;
            if (strlen(arg) == 2) {
                wait_stdout = 1;
            } else {
                int fd = open(&arg[2], mode);
                if (fd > 0) {
                    cmd->stdout = fd;
                } else {
                    perror("open");
                }
            }
        } else if (strncmp(arg, ">", 1) == 0) {
            started_special = 1;
            mode = O_CREAT | O_WRONLY;
            if (strlen(arg) == 1) {
                wait_stdout = 1;
            } else {
                int fd = open(&arg[1], mode);
                if (fd > 0) {
                    cmd->stdout = fd;
                } else {
                    perror("open");
                }
            }

        } else if (strncmp(arg, "2>>", 3) == 0) {
            started_special = 1;
            mode = O_CREAT | O_APPEND | O_WRONLY;
            if (strlen(arg) == 3) {
                wait_stderr = 1;
            } else {
                int fd = open(&arg[3], mode);
                if (fd > 0) {
                    cmd->stderr = fd;
                } else {
                    perror("open");
                }
            }

        } else if (strncmp(arg, "2>", 2) == 0) {
            started_special = 1;
            mode = O_CREAT | O_WRONLY;
            if (strlen(arg) == 2) {
                wait_stderr = 1;
            } else {
                int fd = open(&arg[2], mode);
                if (fd > 0) {
                    cmd->stderr = fd;
                } else {
                    perror("open");
                }
            }

        } else if (strncmp(arg, "<", 1) == 0) {
            started_special = 1;
            mode = O_RDONLY;
            if (strlen(arg) == 1) {
                wait_stdin = 1;
            } else {
                int fd = open(&arg[1], mode);
                if (fd > 0) {
                    cmd->stdin = fd;
                } else {
                    perror("open");
                }
            }

        } else {
            if (wait_stdin) {
                int fd = open(arg, mode);
                if (fd > 0) {
                    cmd->stdin = fd;
                } else {
                    perror("open");
                }
                wait_stdin = 0;

            } else if (wait_stdout) {
                int fd = open(arg, mode);
                if (fd > 0) {
                    cmd->stdout = fd;
                } else {
                    perror("open");
                }
                wait_stdout = 0;

            } else if (wait_stderr) {
                int fd = open(arg, mode);
                if (fd > 0) {
                    cmd->stderr = fd;
                } else {
                    perror("open");
                }
                wait_stderr = 0;
            } else if (!started_special) {
                cmd->max_args = i + 1;
            }
        }
    }
}

void parsed_command_cleanup(parsed_command *cmd) {
    if (cmd->stdout != STDOUT_FILENO) {
        close(cmd->stdout);
    }
    if (cmd->stderr != STDERR_FILENO) {
        close(cmd->stderr);
    }
    if (cmd->stdin != STDIN_FILENO) {
        close(cmd->stdin);
    }
    if (cmd->args != NULL) {
        for (int i = 0; i < cmd->nargs; i++) {
            char *arg = cmd->args[i];
            if (arg != NULL) {
                free(arg);
            }
        }
        free(cmd->args);
    }
    if (cmd->cmd != NULL) {
        free(cmd->cmd);
    }
    free(cmd);
}