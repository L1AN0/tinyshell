#ifndef SHELL_TSH_BUILTIN_H
#define SHELL_TSH_BUILTIN_H

// bg fg are in tsh_job.h
// exit is in tsh_cmd.cpp
int tsh_cd(char **parsed_single_command);

#endif //SHELL_TSH_BUILTIN_H
