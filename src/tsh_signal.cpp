#include <signal.h>
#include <stdio.h>
#include <tsh_signal.h>

/*
 * Signal - wrapper for the sigaction function
 */
void Signal(int signum, handler_t* handler) {
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        perror("Signal error");
    return;
}

