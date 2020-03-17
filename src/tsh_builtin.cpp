#include <zconf.h>
#include <cstdio>

// cd built-in command
int tsh_cd(char **parsed_single_command) {
    if (chdir(parsed_single_command[1]) == 0) {
        return 0;
    } else {
        perror("cd command error");
    }
}

