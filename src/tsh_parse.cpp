#include <tsh_global.h>
#include <stdio.h>
#include <cstring>

// Read a line of user input (terminated by \n) and return it. return nullptr if error
char *read_cmdline() {
    char *command_line = new char[MAX_BUFFER]; // initialze a string
    if ((fgets(command_line, MAX_BUFFER, stdin) == NULL) && ferror(stdin)) {
        perror("fgets error");
        return nullptr;
    } else {
        command_line[strlen(command_line) - 1] = '\0'; // ignore \n at the end
        return command_line;
    }
}

// Given a commandline, parse it and save it to parsed commands. Return the number of commands (separated by ;)
// return -1 on error
int parse_cmd_line(char *command_line, char **parsed_commands) {
    int count_commands = 0;
    char *next_command = strtok(command_line, ";"); // separate command line using ;
    while (next_command != nullptr) {
        parsed_commands[count_commands++] = next_command;
        next_command = strtok(nullptr, ";");
        // Check the number of commands
        if (count_commands > MAX_BUFFER - 1) {
            perror("too many commands");
            return -1;
        }
    }
    return count_commands;
}

// Parse a single command in parsed_commands, save an array of strings to parsed_single_command.
// For example ls -la -> {ls, -la}. Return number of parts in the command. Return -1 on error
int parse_single_command(char *single_command, char **parsed_single_command) {
    int count_parts = 0;
    char *parts = strtok(single_command, COMMAND_DELIMITER);
    while (parts != nullptr) {
        parsed_single_command[count_parts++] = parts;
        parts = strtok(nullptr, COMMAND_DELIMITER);
        // check boundary
        if (count_parts > MAX_BUFFER - 1) {
            perror("too long command");
            return -1;
        }
    }
    return count_parts;
}

// Check whether the command contains pipe. Return 1 if there is a pipe, otherwise return -1
int is_pipe_p(char *single_command) {
    is_pipe = -1;
    for (int i = 0; single_command[i]; i++) {
        if (single_command[i] == '|') {
            is_pipe = 1;
        }
    }
    return is_pipe;
}

// parse a command line with pipe and saved the results to global variables: count_pipe_commands and pipe_cmds
void parse_pipe_command_to_global_variable(char *command_with_pipe) {
    char *copy = strdup(command_with_pipe);
    char *single_command;
    int count_command = 0;
    single_command = strtok(copy, "|");
    while (single_command != nullptr) {
        pipe_cmds[count_command++] = single_command;
        single_command = strtok(nullptr, "|");
    }
    count_pipe_commands = count_command;
}
