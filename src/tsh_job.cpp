#include <cctype>
#include <cstring>
#include <unistd.h>
#include <tsh_global.h>
#include <stdlib.h>
#include <wait.h>
#include <iostream>

void add_job(int pid, int pgid, char *command_line, int processes);
void delete_job(int pgid);
int get_job_remaining_processes(int pgid);
int get_job_index_from_pid(int pid);

// list jobs in the format %jobid : %command_line
void tsh_jobs() {
    int i;
    for (i = 0; i < num_jobs; i++) {
        if (tsh_jobs_table[i].active == 1) {
            std::cout << i << ": " << tsh_jobs_table[i].command_line << std::endl;
        }
    }
}

// bring a job foreground
void tsh_fg(int count_parts, char **parsed_single_command) {
    if (count_parts != 2) {
        std::cerr << "fg should be invoked with jobid directly" << std::endl;
        return;
    }

    int job_number = atoi(parsed_single_command[1]);
    int status;
    if (tsh_jobs_table[job_number].active == 0) {
        std::cerr << "no such job" << std::endl;
        return;
    }
    if (tsh_jobs_table[job_number].active == 1) {
        int pgid = tsh_jobs_table[job_number].pgid;
        tcsetpgrp(tsh_terminal_fd, pgid);                  // give control of terminal to the job's group
        fgpgid = pgid;                            // the foreground pgid global variable
        if (killpg(pgid, SIGCONT) < 0)            // send SIGCONT
            perror("cannot send SIGCONT to the job");
        int remaining_processes = get_job_remaining_processes(pgid);
        if (verbose) {
            std::cout << "remaining processes: " << remaining_processes << std::endl;
            std::cout << "wait for pgid " << pgid << std::endl;
        }
        for (int i = 0; i < remaining_processes; i++) {  // continue to wait for all processes in the group
            int child_pid = waitpid(-pgid, &status, WUNTRACED);       // wait
            if ((child_pid != -1) && (!WIFSTOPPED(status))) {
                if ((!WIFSTOPPED(status)) && (child_pid != -1)) {
                    if (WEXITSTATUS(status) != 0) { // child exits with error, for example an error in a pipe, we then termiante all processes in the command with pipe
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
                                  << std::endl;
                    }
                }
                int jobid = get_job_index_from_pid(child_pid);
                if (tsh_jobs_table[jobid].remaining_alive_process == 1) { // if no process remains in the group then delete the job and set foreground pgid to 0
                    delete_job(tsh_jobs_table[jobid].pgid);
                } else { // otherwise decline the remaining processes in the table by 1 and set the corresponding pid to inactive
                    tsh_jobs_table[jobid].remaining_alive_process--;
                    for (int j = 0; j < tsh_jobs_table[jobid].num_pids; j++) {
                        if ((tsh_jobs_table[jobid].pids[j].pid == child_pid) &&
                            (tsh_jobs_table[jobid].pids[j].active == 1)) {
                            tsh_jobs_table[jobid].pids[j].active = 0;
                        }
                    }
                }
            }
            if (child_pid == -1) { // wait until no process to address
                break;
            }
        }
        fgpgid = 0; // reset fgpgid after wait for all processes in a job
        tcsetpgrp(tsh_terminal_fd, tsh_pid);                // Give control of terminal back
    } else {
        std::cerr << "found no job" << std::endl;
    }
}

// bring a job background (also turn stopped jobs into running state)
void tsh_bg(int count_parts, char **parsed_single_command) {
    if (count_parts != 2) {
        std::cerr << "bg should be invoked with jobid directly" << std::endl;
        return;
    }

    int i, job_number = atoi(parsed_single_command[1]), status;
    if (tsh_jobs_table[job_number].active == 0) {
        std::cerr << "no such job" << std::endl;
        return;
    }
    if (tsh_jobs_table[job_number].active == 1) { // if there is an active job corresponding to the jobid then bring it to background
        int pid = tsh_jobs_table[job_number].pid;
        int pgid = tsh_jobs_table[job_number].pgid; // get pgid from jobid
        if (killpg(pgid, SIGCONT) < 0)            // send SIGCONT to the process ground (in case that they are stopped)
            perror("cannot send SIGCONT to the job");
        if (pgid == fgpgid) {
            fgpgid = 0;                            // reset fgpgid
        }
        tcsetpgrp(tsh_terminal_fd, tsh_pid);                // Give control of terminal back
    } else {
        std::cerr << "found no job" << std::endl;
    }
}

// Add a child process to job control
// if there is already a job for the pgid then add the pid to that job
void add_job(int pid, int pgid, char *command_line, int processes) {
    if (num_jobs >= MAX_BUFFER){
        std::cerr << "too many jobs added, please restart tsh to reinitialize the joblist";
        return;
    }
    for (int i = 0; i < num_jobs; i++) {
        if (tsh_jobs_table[i].pgid == pgid && tsh_jobs_table[i].active == 1) {
            tsh_jobs_table[i].pids[tsh_jobs_table[i].num_pids].pid = pid;
            tsh_jobs_table[i].pids[tsh_jobs_table[i].num_pids].active = true;
            tsh_jobs_table[i].num_pids++;
            return;
        }
    }
    tsh_jobs_table[num_jobs].pid = pid;
    tsh_jobs_table[num_jobs].pgid = pgid;
    tsh_jobs_table[num_jobs].command_line = strdup(command_line);
    tsh_jobs_table[num_jobs].active = 1;
    tsh_jobs_table[num_jobs].remaining_alive_process = processes;
    tsh_jobs_table[num_jobs].pids[tsh_jobs_table[num_jobs].num_pids].pid = pid;
    tsh_jobs_table[num_jobs].pids[tsh_jobs_table[num_jobs].num_pids].active = true;
    tsh_jobs_table[num_jobs].num_pids++;
    num_jobs++;
}

// delete a job
void delete_job(int pgid) {
    for (int i = 0; i < num_jobs; i++) {
        if (tsh_jobs_table[i].pgid == pgid) {
            tsh_jobs_table[i].active = 0;
            tsh_jobs_table[i].pgid = 0;
            tsh_jobs_table[i].pid = 0;
            free(tsh_jobs_table[i].command_line);
            tsh_jobs_table[i].remaining_alive_process = 0;
            tsh_jobs_table[i].num_pids = 0;
            break;
        }
    }
    if (fgpgid == pgid) { // reset fgpgid if the corresponding job is deleted
        fgpgid = 0;
    }
}

// get the jobid from pid
int get_job_index_from_pid(int pid) {
    for (int i = 0; i < num_jobs; i++) {
        if (tsh_jobs_table[i].active == 1) {
            for (int j = 0; j < tsh_jobs_table[i].num_pids; j++) {
                if (tsh_jobs_table[i].pids[j].active == true && tsh_jobs_table[i].pids[j].pid == pid) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// get the number of remaining active processes ( including stopped )
int get_job_remaining_processes(int pgid) {
    for (int i = 0; i < num_jobs; i++) {
        if (tsh_jobs_table[i].pgid == pgid) {
            return tsh_jobs_table[i].remaining_alive_process;
        }
    }
}
