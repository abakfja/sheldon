//
// Created by kannav on 9/1/20.
//

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "exec.h"
#include "builtins.h"
#include "utils.h"
#include "jobs.h"


const char *builtins[] = {
	"cd",
	"pwd",
	"echo",
	"pinfo",
	"ls",
	"exit",
	"setenv",
	"unsetenv",
	"getenv",
	"jobs",
	"kjob"
};

int (*builtin_functions[])(word_list *arg) = {
	change_directory,
	print_current_working_directory,
	echo,
	get_process_info,
	list_files_internal,
	(int (*)(word_list *)) exit_successfully,
	set_env,
	unset_env,
	getenv_internal,
	print_jobs,
	kill_job
};

pid_t last_child = -1;

static int execute_system_command(word *command, word_list *arg, int flag) {
	pid_t pid_1;
	char **argv;
	int status = 0;

	if ((pid_1 = fork()) == -1) {
		/*the fork failed*/
		perror("fork");
		return -1;
	} else if (pid_1 == 0) {
		/*This is the child*/
		argv = generate_argv(command, arg, 0);
		if (flag == 1) { // if background
			setpgid(0, 0);
		}
		errno = 0;
		execvp(command->_text, argv);
		if (errno == ENOENT) {
			printf("sheldon: command not found %s\n", command->_text);
		} else {
			perror("sheldon: command");
		}
		free(argv);
		exit_abruptly(1);
	}
	/*what the parent should do*/
	if (pid_1 > 0) {
		/*if what we forked was a background process, stop*/
		if (flag == 1) {
			if (add_job(pid_1, get_complete_command(command, arg))) {
				return 0;
			} else {
				return -1;
			}
		} else {
			last_child = pid_1;
			waitpid(-1, &status, WUNTRACED);
			/*grab back control of the shell*/
			tcsetpgrp(shell_terminal, shell_pgid);

			if (WIFSTOPPED(status)) {
				put_job_in_bg(pid_1, 0);
			}
			return 0;
		}
	}

	return -1;
}

static int execute_simple_command(simple_command *cc, int flag) {
	word *command = cc->_name;
	word_list *arg = cc->_args;

	int ret = 0;

	if (command == NULL || command->_text == NULL || strlen(command->_text) == 0) {
		printf("(null) command does not exist\n");
		return -1;
	}

	register int found = 0;

	int len = sizeof(builtins) / sizeof(char *);

	for (int i = 0; i < len; i++) {
		if (strcmp(command->_text, builtins[i]) == 0) {
			found = 1;
			builtin_functions[i](arg);
			break;
		}
	}

	if (!found) {
		ret = execute_system_command(cc->_name, cc->_args, flag);
	}

	return ret;
}

int execute_compound_command(compound_command *cc) {
	int saved_stdin = dup(STDIN_FILENO);
	int saved_stdout = dup(STDOUT_FILENO);

	int input_fd;
	if (cc->_inputFile != NULL) {
		input_fd = open(cc->_inputFile, O_RDONLY);
	} else {
		input_fd = dup(saved_stdin);
	}

	int ret = 0;
	int output_fd;
	for (simple_command_list *curr = cc->_simple_commands; curr != NULL; curr = curr->_next) {
		dup2(input_fd, STDIN_FILENO); // set 0 to correspond to input fd
		close(input_fd); // corresponds to no file now

		current_simple_command = curr->_command;

		if (curr->_next == NULL) {
			if (cc->_outFile != NULL) {
				if (!(cc->_append_input)) {
					output_fd = open(cc->_outFile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
					if (output_fd == -1) {
						perror("sheldon: open");
					}
				} else {
					output_fd =
						open(cc->_outFile, O_CREAT | O_APPEND, 0644);
					if (output_fd == -1) {
						perror("sheldon: open");
					}
				}
			} else {
				output_fd = dup(saved_stdout);
			}
		} else {
			int pipe_fd[2];
			if (pipe(pipe_fd) < 0) {
				perror("pipe");
				return -1;
			}
			output_fd = pipe_fd[1];
			input_fd = pipe_fd[0];
		}

		dup2(output_fd, 1); // set 1 to correspond to output fd
		close(output_fd); // corresponds to no file now

		if (execute_simple_command(curr->_command, cc->_background) != 0) {
			ret = -1;
			break;
		}
	}

	dup2(saved_stdin, STDIN_FILENO); // set 0 to the actual stdin
	dup2(saved_stdout, STDOUT_FILENO); // set 1 to the actual stdout

	close(saved_stdin);
	close(saved_stdout);
	return ret;
}



