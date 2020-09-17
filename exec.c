//
// Created by kannav on 9/1/20.
//

#include "exec.h"

const char *builtins[] = {
        "cd",
        "pwd",
        "echo",
        "pinfo",
        "ls",
        "exit"
};

int (*builtin_functions[])(list_node *arg) = {
    change_directory,
    print_current_working_directory,
    echo,
    get_process_info,
    list_files_internal,
    (int (*)(list_node *)) exit_successfully
};

int execute_system_command(node *command, list_node *arg, int flag) {
    pid_t pid_1;
    char **argv;
    int status = 0;

    switch ((pid_1 = fork())) {

        case -1: // definitely parent
            perror("fork");
            return -1;
        case 0: // is the child
            argv = generate_argv(command, arg, 0);
            if (flag == 1) { // if background
                setpgid(0, 0);
            }
            execvp(command->text, argv);
            perror("sheldon : command");
            free(argv);
            exit_abruptly(1);
    }

    if (pid_1 > 0) {
        if (flag == 1) {
            int res = add_process(pid_1, command->text);
            if (res) {
                printf("\n[%d] %s %d\n", number_of_bg_processes, command->text, pid_1);
            } else {
                return -1;
            }
        } else {
            waitpid(0, &status, 0);
            return 0;
        }
    }

    return -1;
}

int execute_command(simple_command *cc) {

    node *command = cc->name;
    list_node *arg = cc->args;

    int ret = 0;

    if (command == NULL || command->text == NULL || strlen(command->text) == 0) {
        printf("(null) command does not exist\n");
        return -1;
    }

    register int found = 0;

    int len = sizeof(builtins) / sizeof(char *);

    for (int i = 0; i < len; i++) {
        if (strcmp(command->text, builtins[i]) == 0) {
            found = 1;
            builtin_functions[i](arg);
            break;
        }
    }

    if (!found) {
        ret = execute_system_command(cc->name, cc->args, cc->flag);
    }

    return ret;
}


