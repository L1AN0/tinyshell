#ifndef TSH_TSH_SIGNAL_H
#define TSH_TSH_SIGNAL_H

typedef void handler_t(int);
void Signal(int signum, handler_t *handler);

#endif //TSH_TSH_SIGNAL_H
