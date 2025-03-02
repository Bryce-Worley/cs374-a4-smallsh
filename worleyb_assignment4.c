/**
 * Program Name:	Smallsh
 * Author:		Bryce Worley
 * Description: 	The program runs a smallsh shell written in C. It defines three built-in commands: exit, cd, status.
 * 			Other commands are executed by creating new processes via the exec() family of functions.
 * 			Smallsh supports input and output redirection as well as execution of foreground and background processes.
 **/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> // getenv
#include <string.h>
#include <unistd.h> //getpid, chdir
#include <sys/types.h> // pid_t
#include <sys/wait.h> // wait, waitpid
#include <fcntl.h>
#include <signal.h>

#define INPUT_LENGTH	2048
#define MAX_ARGS	512

// Struct for command_line adapted from sample_parser.c provided in assignment4 materials
struct command_line{
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};


// parse_input adapted from sample_parse.c provided in assignment4 materials
struct command_line *parse_input(){
	char input[INPUT_LENGTH];
	struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

	// Get input
	printf(": ");
	fflush(stdout);
	fgets(input, INPUT_LENGTH, stdin);

	// Tokenize the input
	char *token = strtok(input, " \n"); // may need to switch up to strtok_r if mem leaks occur
	while(token){
		if(!strcmp(token, "<")){
			curr_command->input_file = strdup(strtok(NULL, " \n"));
		} else if(!strcmp(token, ">")){
			curr_command->output_file = strdup(strtok(NULL, " \n"));
		} else if(!strcmp(token, "&")){
			curr_command->is_bg = true;
		} else{
			curr_command->argv[curr_command->argc++] = strdup(token);
		}
		token = strtok(NULL, " \n");
	}
	return curr_command;
}


// Signal handler to check on background processes
void handle_SIGCHLD(int signo){
	int childStatus;
	pid_t childPid;

	while((childPid = waitpid(-1, &childStatus, WNOHANG)) > 0){
		if(WIFEXITED(childStatus)){
			char msg[50];
			int len = snprintf(msg, sizeof(msg), "\nbackground pid %d is done: exit value %d\n: ", childPid, WEXITSTATUS(childStatus));

			write(STDOUT_FILENO, msg, len);
		//	printf("\nbackground pid %d is done: exit value %d\n", childPid, WEXITSTATUS(childStatus));
		//	fflush(stdout);
		//	printf(": ");
		//	fflush(stdout);
		} else{
			char msg[60];
			int len = snprintf(msg, sizeof(msg), "\nbackground pid %d is done: terminated by signal %d\n: ", childPid, WTERMSIG(childStatus));
			write(STDOUT_FILENO, msg, len);
		//	printf("\nbackground pid %d is done: terminated by signal %d\n", childPid, WTERMSIG(childStatus));
		//	fflush(stdout);
		//	printf(": ");
		//	fflush(stdout);
		}
	}
}

// Signal handler to ignore SIGINT
void handle_SIGINT(int signo){
	char msg[33];
	int len = snprintf(msg, sizeof(msg), "\nterminated by signal %d\n", signo);
	write(STDOUT_FILENO, msg, len);
}

int main(){
	struct command_line *curr_command;
	int childStatus;
	bool initialized = false;

	struct sigaction SIGCHLD_action = {0}, SIGINT_action = {0}, ignore_action = {0}, default_action = {0};
	SIGCHLD_action.sa_handler = handle_SIGCHLD;
	sigfillset(&SIGCHLD_action.sa_mask);
	SIGCHLD_action.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &SIGCHLD_action, NULL);

	SIGINT_action.sa_handler = handle_SIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);

	ignore_action.sa_handler = SIG_IGN;
	
	default_action.sa_handler = SIG_DFL;

	while(true){
		curr_command = parse_input();

		// Handle blank lines and comments
		if(curr_command->argc == 0) continue;
		if(!strncmp(curr_command->argv[0], "#", 1)) continue;
		
		// Built-in commands
		// Built-in command: exit
		if(!strcmp(curr_command->argv[0], "exit") && curr_command->argc == 1){
			break;
		}
		// Built-in command: cd
		else if(!strcmp(curr_command->argv[0], "cd") && curr_command->argc == 1){
			if(chdir(getenv("HOME")) != 0){	
				perror("chdir() to HOME failed");
				exit(1);
				fflush(stdout);
			} else{	
				setenv("PWD", getenv("HOME"), 1);
			}
			continue;
		} else if(!strcmp(curr_command->argv[0], "cd") && curr_command->argc == 2){
			if(chdir(curr_command->argv[1]) != 0){	
				perror("chdir() failed");
				printf("The current working directory is %s\n", getenv("PWD"));
				fflush(stdout);
			} else{	
				setenv("PWD", curr_command->argv[1], 1);
				printf("You are now in %s\n", getenv("PWD"));
				fflush(stdout);
			}
			continue;
		}
		// Built-in command: status
		else if(!strcmp(curr_command->argv[0], "status") && curr_command->argc == 1){
			if(initialized){
				if(WIFEXITED(childStatus)){
					printf("exit value %d\n", WEXITSTATUS(childStatus));
					fflush(stdout);
				} else{
					printf("Abnormal termination due to signal %d\n", WTERMSIG(childStatus));
					fflush(stdout);
				}
			} else{
				return 0;
			}
			continue;
		}


		// Execution of other commands
		pid_t spawnPid = fork();
		initialized = true;


		switch(spawnPid){
			case -1:
				perror("fork()");
				exit(1);
				break;
			case 0: // Child Process
				//SIGINT handling
				if(curr_command->is_bg){
					sigaction(SIGINT, &ignore_action, NULL);
				} else{
					sigaction(SIGINT, &default_action, NULL);
				}

				// Check for input redirection
				if(curr_command->input_file){
					//inputRedirect(curr_command->input_file);
					// Open source file
					int sourceFD = open(curr_command->input_file, O_RDONLY);
					if(sourceFD == -1){
						printf("cannot open %s for input\n", curr_command->input_file);
						fflush(stdout);
						exit(1);
					}
					// Redirect stdin to source file
					int result = dup2(sourceFD, 0);
					if(result == -1){
						perror("source dup2()");
						exit(1);
					}
					close(sourceFD);
				} else if(curr_command->is_bg){
					//Redirect stdin to /dev/null/ for bg processes
					int bgDefIn = open("/dev/null", O_RDONLY);
					dup2(bgDefIn, 0);
					close(bgDefIn);
				}

				// Check for output redirection
				if(curr_command->output_file){
					//outputRedirect(curr_command->output_file);
					int targetFD = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if(targetFD == -1){
						printf("cannot open %s for output\n", curr_command->output_file);
						fflush(stdout);
						exit(1);
					}
					// Redirect stdouut to target file
					int result = dup2(targetFD, 1);
					if(result == -1){
						perror("target dup2()");
						exit(1);
					}
					close(targetFD);
				} else if(curr_command->is_bg){
					// Redirect stdout to /dev/null for bg process
					int bgDefOut = open("/dev/null", O_WRONLY);
					dup2(bgDefOut, 1);
					close(bgDefOut);
				}

				execvp(curr_command->argv[0], curr_command->argv);
				perror(curr_command->argv[0]);
				exit(1);
				break;
			default://Parent process
				if(curr_command->is_bg){
					printf("background pid is %d\n", spawnPid);
					fflush(stdout);
				} else{
					waitpid(spawnPid, &childStatus, 0);
				}
				break;
		}
	
	}
	return EXIT_SUCCESS;
}
