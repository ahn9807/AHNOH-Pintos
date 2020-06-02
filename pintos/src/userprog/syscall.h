#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void check_address(void *addr);
void get_argument(void *esp, int *arg, int count);

void halt (void);
void exit (int status);


#endif /* userprog/syscall.h */
