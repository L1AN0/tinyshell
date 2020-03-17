#ifndef TSH_TSH_JOB_H
#define TSH_TSH_JOB_H

void tsh_jobs();
void tsh_fg(int count_parts, char **parsed_single_command);
void tsh_bg(int count_parts, char **parsed_single_command);
void add_job(int pid, int pgid, char *command_line, int processes);
void delete_job(int pgid);
int get_job_index_from_pid(int pid);
int get_job_remaining_processes(int pgid);

#endif //TSH_TSH_JOB_H
