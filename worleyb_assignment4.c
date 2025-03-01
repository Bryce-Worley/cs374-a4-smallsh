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

//

int main(){
	struct command_line *curr_command;
	int childStatus;
	bool initialized = 0;

	while(true){
		curr_command = parse_input();

		// Handle blank lines and comments
		if(curr_command->argc == 0) continue;
		if(!strncmp(curr_command->argv[0], "#", 1)) continue;
		
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
				//printf("You are now in %s\n", getenv("HOME"));
				//printf("You are now in %s\n", getenv("PWD"));
				//fflush(stdout);
			}
		} else if(!strcmp(curr_command->argv[0], "cd") && curr_command->argc == 2){
			if(chdir(curr_command->argv[1]) != 0){	
				perror("chdir() failed");
				printf("The current working directory is %s\n", getenv("PWD"));
				fflush(stdout);
			} else{	
				setenv("PWD", curr_command->argv[1], 1);
				//printf("You are now in %s\n", getenv("HOME"));
				printf("You are now in %s\n", getenv("PWD"));
				fflush(stdout);
			}
		}

		// Built-in command: status
		else if(!strcmp(curr_command->argv[0], "status") && curr_command->argc == 1){
			if(initialized){
				if(WIFEXITED(childStatus)){
					printf("Exit status: %d\n", WEXITSTATUS(childStatus));
					fflush(stdout);
				} else{
					printf("Abnormal termination due to signal %d\n", WTERMSIG(childStatus));
					fflush(stdout);
				}
			} else{
				return 0;
			}
		}

		// Execution of other commands
		else{
			pid_t spawnPid = fork();
			initialized = 1;

			switch(spawnPid){
				case -1:
					perror("fork()\n");
					exit(1);
					break;
				case 0:
					printf("CHILD: (%d) running %s command\n", getpid(), curr_command->argv[0]);
					execvp(curr_command->argv[0], curr_command->argv);
					perror("execvp");
					exit(1);
					break;
				default:
					spawnPid = waitpid(spawnPid, &childStatus, 0);
					printf("PARENT (%d): child (%d) terminated\n", getpid(), spawnPid);
					break;
			}
		}
	}
	return EXIT_SUCCESS;
}
