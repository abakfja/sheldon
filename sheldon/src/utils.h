//
// Created by kannav on 9/1/20.
//

#ifndef SHELDON_SRC_UTILS_H
#define SHELDON_SRC_UTILS_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define EPRINTF(...) fprintf(stderr,__VA_ARGS__),fflush(stderr)

extern pid_t shell_pgid;

extern int shell_terminal;

extern char *home;

extern char *pwd;

extern char *inp;

extern struct winsize terminal;

extern char **input_argv;

void exit_successfully(void);

void exit_safely(int return_code);

void exit_abruptly(int return_code);

#endif //SHELDON_SRC_UTILS_H
