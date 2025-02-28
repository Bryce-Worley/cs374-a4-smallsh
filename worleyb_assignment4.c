/**
 * Program Name:	Smallsh
 * Author:		Bryce Worley
 * Description: 	The program runs a smallsh shell written in C. It defines three built-in commands: exit, cd, status.
 * 			Other commands are executed by creating new processes via the exec() family of functions.
 * 			Smallsh supports input and output redirection as well as execution of foreground and background processes.
 **/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //getpid

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

	while(true){
		curr_command = parse_input();
		// Handle blank lines and comments
		if (curr_command->argc == 0) continue;
		if (!strncmp(curr_command->argv[0], "#", 1)) continue;
		// Built-in command: exit
		if(!strcmp(curr_command->argv[0], "exit")){
			break;
		}

	}
	return EXIT_SUCCESS;
}
