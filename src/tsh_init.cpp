#include <tsh_global.h>
#include <cstring>
#include <stdio.h>
#include <signal.h>
#include <wait.h>
#include <stdlib.h>
#include <tsh_signal.h>
#include <iostream>
#include <zconf.h>
#include "tsh_job.h"

// show the prompt
void show_prompt() {
    if (emit_prompt) {
        std::cout << "tsh" << "> ";
    }
}

// handle sigquit (simply exit)
void sigquit_handler(int sig) {
    std::cout << "terminating after receipt of SIGQUIT signal" << std::endl;
    exit(1);
}

// handle other signals
void signal_handler(int signum) {
    if (verbose) {
        std::cout << "Received signal " << strsignal(signum) << std::endl;
    }
    if (signum == SIGINT) {
        signal(SIGINT, SIG_IGN);              // ignore ctrl-c
        signal(SIGINT, signal_handler);       // reset signal handler
    } else if (signum == SIGCHLD) {
        int status, child_pid;
        while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) { // process all processes without hanging
            if (verbose) {
                std::cout << "get child pid " << child_pid << std::endl;
            }
            // Check the child_pid's status
            if(verbose) {
                if (WIFSIGNALED(status)) {  // terminated by other signals
                    std::cout << "child" << " with pid " << child_pid
                              << " signaled" << std::endl;
                }
            }

            if (WEXITSTATUS(status) != 0) { // child exits with error, for example an error in a pipe, we then termiante all processes in the command with pipe
                if (kill_pipe) {
                    if (verbose) {
                        std::cerr << "child exited with error. status: " << WEXITSTATUS(status)
                                  << ". killing all children in the job" << std::endl;
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

            if ((child_pid != -1) && (!WIFSTOPPED(status))) { // if the process is terminated (not by ctrl-z)
                // get the jobid associated with the child_pid and update the job table accordingly
                int jobid = get_job_index_from_pid(child_pid);
                if (jobid == -1) {
                    std::cerr << "get unknown child pid" << std::endl;
                    break;
                }
                if (tsh_jobs_table[jobid].remaining_alive_process == 1) { // if no process remains in the group then delete the job
                    delete_job(tsh_jobs_table[jobid].pgid);
                } else { // otherwise decline the remaining processes in the table by 1 and set the corresponding pid to inactive
                    tsh_jobs_table[jobid].remaining_alive_process--;
                    for (int j=0; j<tsh_jobs_table[jobid].num_pids; j++) {
                        if((tsh_jobs_table[jobid].pids[j].pid == child_pid && tsh_jobs_table[jobid].pids[j].active == 1)) {
                            tsh_jobs_table[jobid].pids[j].active = 0;
                        }
                    }
                }
            }
        }
    }
}

// initialize an array of char* with length of length_array
char **initialize_array_of_string(int length_array) {
    char **x = new char *[length_array];
    if (x == nullptr) {
        perror("command array initialization error");
        return nullptr;
    }
    for (int j = 0; j < length_array; j++) {
        x[j] = nullptr;
    }
    return x;
}

// initiate job table
void init_jobs() {
    num_jobs = 0;
    for (int i = 0; i < MAX_BUFFER; i++) {
        tsh_jobs_table[i].pid = 0;
        tsh_jobs_table[i].pgid = 0;
        tsh_jobs_table[i].command_line = nullptr;
        tsh_jobs_table[i].active = 0;
        tsh_jobs_table[i].remaining_alive_process = 0;
        for (int j = 0; j < MAX_BUFFER; j++) {
            tsh_jobs_table[i].pids[j].active = 0;
            tsh_jobs_table[i].pids[j].pid = 0;
        }
        tsh_jobs_table[i].num_pids = 0;
    }
}

// Terminal initialization and global variables initialization
void initialize_tsh() {
    dup2(1, 2);
    // initialize job table
    init_jobs();
    // initialize terminal
    tsh_terminal_fd = STDERR_FILENO;

    // Initialize signals. First ignore the signals then add my handlers
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    Signal(SIGCHLD, signal_handler); /* Terminated or stopped child */
    Signal(SIGINT, signal_handler);   /* ctrl-c */
    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    // set the pgid of tsh to itself
    int err = setpgid(0, 0);
    if (err == -1) {
        perror("setting the pgid of tsh failed");
    }

    // initialize tsh's pid and pgid global variables
    tsh_pid = tsh_pgid = getpid();
    tcsetpgrp(tsh_terminal_fd, tsh_pgid);  // give the control of tsh_terminal_fd to tsh
}
