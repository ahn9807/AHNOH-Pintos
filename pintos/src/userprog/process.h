#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void setup_stack_argv(void **esp, int argc, char **argv);

#endif /* userprog/process.h */
