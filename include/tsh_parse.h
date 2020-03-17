#ifndef TSH_TSH_PARSE_H
#define TSH_TSH_PARSE_H

char* read_cmdline();
int parse_cmd_line(char* command_line, char** parsed_commands);
int parse_single_command(char *single_command, char **parsed_single_command);
int is_pipe_p(char *single_command);
void parse_pipe_command_to_global_variable(char *command_with_pipe);

#endif //TSH_TSH_PARSE_H
