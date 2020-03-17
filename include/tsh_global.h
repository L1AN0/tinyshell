#ifndef TSH_TSH_GLOBAL_H
#define TSH_TSH_GLOBAL_H

#include <signal.h>

#define MAX_BUFFER 2048 // Unified constant for the volume of all the bufferes of tsh (jobs, command_line length, number of processes in a job etc.)
#define COMMAND_DELIMITER " \t\n" // use space \t \n to delimitate different parts of a command

struct sub_pid { // substruct for tsh_job that records pids in a job
    int pid; // the pid of a process
    int active; // whether it is still alive
};

typedef struct sub_pid sub_pid;

struct tsh_job {
    int pid, pgid; // the pid and pgid of the leader process (actually should be the same)
    char *command_line; // the command line associated with the job
    int active; // whether the job is still alive
    int remaining_alive_process; // the number of alive processes in this job
    sub_pid pids[MAX_BUFFER]; // contain all processes in the process group
    int num_pids; // current number of processes in the job, either dead or alive (initially 0)
};

typedef struct tsh_job tsh_job;

extern int num_jobs; // current number of jobs
extern tsh_job tsh_jobs_table[MAX_BUFFER]; // global table of jobs
extern char *pipe_cmds[MAX_BUFFER]; // global variable for executing commands with pipe, each entry contains one command in a pipe command
extern int count_pipe_commands; // global variable keeping number of commands in the command with pipes
extern int is_pipe; // whether current command is a command with pipes
extern int is_background; // whether current command should be run in background

extern int tsh_terminal_fd; // terminal's file descriptor

extern int verbose; // whether output debug messages. 1 to print. 0 not to print.
extern int emit_prompt; // whether emit prompt. 1 to emit. 0 not to emit.
extern int kill_pipe; /* kill all commands in the pipe if someone fails */

extern pid_t tsh_pid, tsh_pgid; // this shell's pid and pgid
extern pid_t fgpgid; // fgpgid records the pgid of foreground job. it is 0 if there is no foreground job

#endif //TSH_TSH_GLOBAL_H
