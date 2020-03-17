#include <tsh_init.h>
#include <signal.h>
#include <stdio.h>
#include <tsh_global.h>
#include <tsh_parse.h>
#include <memory.h>
#include <tsh_cmd.h>
#include <malloc.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

// global variables (also in tsh_global.h)
int num_jobs;
tsh_job tsh_jobs_table[MAX_BUFFER];
char *pipe_cmds[MAX_BUFFER];
int tsh_terminal_fd;
int count_pipe_commands;
int is_pipe;
int verbose;
int emit_prompt = 1; // by default emit prompt
int kill_pipe = 0;
pid_t tsh_pid, tsh_pgid, fgpgid;
int is_background;

// helper function
void usage(void);

int main(int argc, char **argv) {
    char c;
    // parse options
    while ((c = getopt(argc, argv, "hvpk")) != EOF) {
        switch (c) {
            case 'h': /* print help message */
                usage();
                break;
            case 'v': /* emit additional diagnostic info */
                verbose = 1;
                break;
            case 'p':          /* don't print a prompt */
                emit_prompt = 0; /* handy for automatic testing */
                break;
            case 'k':          /* kill all commands in the pipe if someone fails */
                kill_pipe = 1;
                break;
            default:
                usage();
        }
    }

    // prompt, signal handler etc.
    initialize_tsh();

    // command infinite loop
    while (1) {
        int is_error = 0; // indicate error in this iteration
        show_prompt(); // show tsh> if emit_prompt is set

        // read user input
        char *cmdline = read_cmdline();
        if (cmdline == nullptr) {
            is_error = 1;
            continue;
        }

        if (feof(stdin)) { // end of file (ctrl-d)
            fflush(stdout);
            exit(0);
        }

        // initialize an array to contain the parsed user input
        char **parsed_commands = initialize_array_of_string(MAX_BUFFER);
        if (parsed_commands == nullptr) {
            is_error = 1;
        }

        if (is_error == 0) {
            // parse user input
            int count_commands = parse_cmd_line(cmdline, parsed_commands);
            if (count_commands == -1) {
                is_error = 1;
            }

            if (is_error == 0) {
                // execute commands in parsed_commands one by one
                for (int i = 0; i < count_commands; i++) {
                    is_background = 0; // by default foreground
                    count_pipe_commands = 0;
                    char *current_command = strdup(parsed_commands[i]);

                    // initialize an array of commands for current command (for pipe/redirect feature)
                    char **parsed_single_command = initialize_array_of_string(MAX_BUFFER);
                    if (parsed_single_command == nullptr) {
                        is_error = 1;
                        free(parsed_single_command);
                        break;
                    }

                    // run the command
                    if (is_pipe_p(strdup(current_command)) == -1) {
                        // parse current command
                        int count_parts = parse_single_command(strdup(current_command), parsed_single_command);
                        if (count_parts == -1) {
                            is_error = 1;
                            free(parsed_single_command);
                            break;
                        }
                        // execute the command
                        run_general_single_command(count_parts, parsed_single_command, current_command);
                    } else {
                        run_pipe_command(current_command);
                    }
                    // clean up
                    free(parsed_single_command);
                }
            }
        }

        // clean up
        if (parsed_commands) free(parsed_commands);
        if (cmdline) free(cmdline);
    }
    return 0;
}

// print the usage
void usage(void) {
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    printf("   -k   kill all processes created by a command with pipe if one of the commands fails\n");
    exit(1);
}
