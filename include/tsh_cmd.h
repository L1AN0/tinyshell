#ifndef TSH_TSH_CMD_H
#define TSH_TSH_CMD_H

int run_simple_command(char **parsed_single_command, char *command_line);
void run_general_single_command(int count_parts, char **parsed_single_command, char *single_command_line);
void run_pipe_command(char *command_with_pipes);

#endif //TSH_TSH_CMD_H
