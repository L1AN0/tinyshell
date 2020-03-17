#include <cctype>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <tsh_global.h>
#include <wait.h>
#include <tsh_parse.h>
#include <stdlib.h>
#include <memory.h>
#include <tsh_job.h>
#include <iostream>
#include <tsh_init.h>
#include <tsh_builtin.h>

// Run a command with parsed single command using fork and exec. Return -1 on error. 0 on success.
int run_simple_command(char **parsed_single_command, char *command_line) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        return -1;
    } else if (pid == 0) { // child
        setpgid(0, 0);                              // set pid and pgid to its own pid

        if (is_background != 1) { // foreground
            tcsetpgrp(tsh_terminal_fd, getpid());       // let child process gets the terminal if foreground
        }

        // let the child process gets the default behaviour of signals that were ignored in parent
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        // EXECUTE THE COMMAND!
        int err = execvp(parsed_single_command[0], parsed_single_command);
        if (err < 0) {
            perror("execvp error");
            _exit(-1);
        }
        _exit(0);
    } else if (pid > 0) { // parent
        setpgid(pid, pid); // to avoid race condition, also let child process gets its own pgid in parent
        if (is_background != 1) { // foreground
            tcsetpgrp(tsh_terminal_fd, pid);   // to avoid race condition, also let child process gets the terminal in parent
            add_job(pid, pid, strdup(command_line), 1);
            fgpgid = pid;
            int status;
            int err = waitpid(pid, &status, WUNTRACED);              // wait without until stopped or exited
            if (err == -1) {
                perror("waitpid error");
                return -1;
            }

            if (!WIFSTOPPED(status))   // stopped not stopped by SIGSTOP then delete the job
            {
                delete_job(pid);
            } else {
                std::cerr << parsed_single_command[0] << " with pid " << pid << " has stopped";
            }
            tcsetpgrp(tsh_terminal_fd, tsh_pgid);       // tsh gets back the control of tsh_terminal_fd
        } else if (is_background == 1) { // background, just add job
            add_job(pid, pid, strdup(command_line), 1);
            return 0;
        }
    }
}

// Run a general single command. This includes builtin command or ordinary system commands without pipes
void run_general_single_command(int count_parts, char **parsed_single_command, char *single_command_line) {
    char *command_name = parsed_single_command[0];
    if (count_parts > 0) { // user input is not empty
        if (strcmp(command_name, "fg\0") == 0) tsh_fg(count_parts, parsed_single_command); // fg
        else if (strcmp(command_name, "bg\0") == 0) tsh_bg(count_parts, parsed_single_command); // bg
        else if (strcmp(command_name, "jobs\0") == 0) tsh_jobs();  // jobs
        else if (strcmp(command_name, "cd\0") == 0) tsh_cd(parsed_single_command); // cd
        else if (strcmp(command_name, "exit\0") == 0) { // exit
            _exit(0);
        } else if (strcmp(parsed_single_command[count_parts - 1], "&\0") == 0) { // background command
            parsed_single_command[count_parts - 1] = nullptr;
            is_background = 1;
            run_simple_command(parsed_single_command, single_command_line);        // for running background process
        } else { // foreground command (default)
            run_simple_command(parsed_single_command, single_command_line);
        }
    } else {
        if (verbose) {
            perror("get empty command");
        }
    }
}

// Run command with pipe
void run_pipe_command(char *command_with_pipes) {
    int pid = 0, pgid = 0;
    count_pipe_commands = 0; // reset global variable

    // check if background
    char **temp_parsed_single_command = initialize_array_of_string(MAX_BUFFER);
    int temp_count_parts = parse_single_command(strdup(command_with_pipes), temp_parsed_single_command);
    if (temp_count_parts == -1) {
        std::cerr << "cannot parse command at line " << __LINE__ << " of " << __FUNCTION__ << std::endl;
    }
    if (strcmp(temp_parsed_single_command[temp_count_parts - 1], "&\0") == 0) {
        is_background = 1;
        if (verbose) {
            std::cout << "pipe command in background" << std::endl;
        }
    }

    // separate the commands
    parse_pipe_command_to_global_variable(command_with_pipes);


    // initialize pipe file descriptor array
    int *pipe_file_descriptors = new int[2 * count_pipe_commands - 1];
    for (int i = 0; i < 2 * count_pipe_commands - 1; i++) {
        pipe_file_descriptors[i] = 0;
    }
    if (pipe_file_descriptors == nullptr) {
        perror("pipe file descriptor array init failed");
        return;
    }

    // create pipes
    for (int i = 0; i < 2 * count_pipe_commands - 3; i += 2) {
        if (pipe(pipe_file_descriptors + i) < 0) {
            perror("pipe error");
            return;
        }
    }

    // create signal mask to block sigchld to avoid race condition (we want to wait for sigchld in signal_handler or other wait procedure after all children forked)
    sigset_t signal_block_mask;
    sigemptyset(&signal_block_mask);
    sigaddset(&signal_block_mask,SIGCHLD);
    int err = sigprocmask(SIG_BLOCK, &signal_block_mask, NULL);
    if (err == -1){
        perror("sigprocmask");
    }

    // create children
    for (int i = 0; i < count_pipe_commands; i++) {
        // parse each of the command in the pipe command
        char **parsed_pipe_commands = initialize_array_of_string(MAX_BUFFER);
        int count_parts = parse_single_command(strdup(pipe_cmds[i]), parsed_pipe_commands);
        if (count_parts == -1) {
            std::cerr << "cannot parse command at line " << __LINE__ << " of " << __FUNCTION__ << std::endl;
        }

        // remove & symbol if background
        if (i == count_pipe_commands - 1 && is_background == 1) {
            parsed_pipe_commands[count_parts - 1] = nullptr;
        }

        pid = fork(); // fork a child to execute command
        if (pid > 0) { // parent
            if (i == 0) {
                // the pid of the first child becomes the pgid of all children
                pgid = pid;
                int err = setpgid(pid, pgid);
                if (verbose) {
                    if (err == -1) {
                        perror("setpgid failed");
                    }
                    std::cout << "pipe command with pid " << pid << " got pgid set to " << pgid << std::endl;
                }
                // the job is created with the pgid to be the first child's pid
                add_job(pid, pid, command_with_pipes, count_pipe_commands);

            } else {
                int err = setpgid(pid, pgid);    // assign pgid of processes to pgid of first command
                if (verbose) {
                    if (err == -1) {
                        perror("setpgid failed");
                    }
                    std::cout << "pipe command with pid " << pid << " got pgid set to " << pgid << std::endl;
                }
                // add other jobs with pgid set to the first child
                add_job(pid, pgid, command_with_pipes, count_pipe_commands);
            }
        } else if (pid < 0) {
            perror("fork failed");
            if (i!=0 && pgid!=0) { // terminate the group of processes if already executed some
                killpg(pgid, SIGTERM);
            }
            break;
        } else if (pid == 0) {  // child
            if (i == 0) {
                int err = setpgid(0, 0); // also setpgid in child to avoid race condition
                if (verbose) {
                    if (err == -1) {
                        perror("setpgid failed");
                    }
                }
            } else if (pgid != 0){
                int err = setpgid(0, pgid); // also setpgid to child to avoid race condition
                if (verbose) {
                    if (err == -1) {
                        perror("setpgid failed");
                    }
                }
            }
            // initialize signal handlers in children
            signal(SIGCHLD, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGSTOP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            // set stdin stdout to appropriate file descriptors in pipe_file_descriptors
            if (i < count_pipe_commands - 1) {
                dup2(pipe_file_descriptors[2 * i + 1], 1);
            }
            if (i > 0) {
                dup2(pipe_file_descriptors[2 * i - 2], 0);
            }

            // clean up
            for (int j = 0; j < 2 * count_pipe_commands - 2; j++) {
                close(pipe_file_descriptors[j]);
            }

            // EXECUTE THE COMMAND!
            if (execvp(parsed_pipe_commands[0], parsed_pipe_commands) < 0) {
                perror("execvp error");
                _exit(-1);
            }
        }
    }

    for (int i = 0; i < 2 * count_pipe_commands - 2; i++) {
        close(pipe_file_descriptors[i]);
    }

    err = sigprocmask(SIG_UNBLOCK, &signal_block_mask, NULL);  // unblock SIGCHLD
    if (err == -1) {
        perror("sigprocmask");
    }

    if (is_background != 1) { // foreground
        tcsetpgrp(tsh_terminal_fd, pgid);                  // assign terminal file descriptor to the group of the commands in the pipe
        fgpgid = pgid;
        int remaining_processes = get_job_remaining_processes(fgpgid);
        for (int i = 0; i < remaining_processes; i++) {    // wait for all runnning processes in the group
            int status;
            int child_pid = waitpid(-pgid, &status, WUNTRACED);
            if ((child_pid != -1) && (!WIFSTOPPED(status))) {
                if (WEXITSTATUS(status) != 0)  { // child exits with error, for example an error in a pipe, we then termiante all processes in the command with pipe
                    if (kill_pipe) {
                        if (verbose) {
                            std::cerr << "child exited with error. status: " << WEXITSTATUS(status) << ". killing all children in the job" << std::endl;
                        }
                        int jobid = get_job_index_from_pid(child_pid);
                        int err = killpg(tsh_jobs_table[jobid].pgid, SIGTERM);
                        if (err == -1) {
                            perror("killpg error");
                            std::cerr << __LINE__ << __FUNCTION__ << std::endl;
                        }
                        if (verbose) {
                            std::cerr << "children in the job killed" << std::endl;
                        }
                    }
                }
                if (verbose) {
                    std::cout << "child with pid " << child_pid << " exited with status " << WEXITSTATUS(status)
                              << " at line " << __LINE__ << " of " << __FUNCTION__ << std::endl;
                }
                int jobid = get_job_index_from_pid(child_pid);
                if (tsh_jobs_table[jobid].remaining_alive_process == 1) { // if no process remains in the group then delete the job and set foreground pgid to 0
                    delete_job(tsh_jobs_table[jobid].pgid);
                } else { // otherwise decline the remaining processes in the table by 1 and set the corresponding pid to inactive
                    tsh_jobs_table[jobid].remaining_alive_process--;
                    for (int j = 0; j < tsh_jobs_table[jobid].num_pids; j++) {
                        if ((tsh_jobs_table[jobid].pids[j].pid == child_pid) && (tsh_jobs_table[jobid].pids[j].active == 1)) {
                            tsh_jobs_table[jobid].pids[j].active = 0;
                        }
                    }
                }
            }
            if (child_pid == -1) {
                if (verbose) {
                    std::cerr << "no child to wait" << std::endl;
                }
            }
        }
        fgpgid = 0; // reset fgpgid after waiting for all processes in a foreground job
        tcsetpgrp(tsh_terminal_fd, tsh_pgid);           // tsh gets back terminal control
    }
}

