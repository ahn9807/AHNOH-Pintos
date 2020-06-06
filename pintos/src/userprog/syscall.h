#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void check_address(void *addr);
void get_argument(void *esp, int *arg, int count);

void syscall_init (void);
void halt (void);
void exit (int status);

//pid_t exec
int exec(const char *cmd_line);
int wait (int pid);
//bool create
int create (const char *file, unsigned initial_size);
//bool remove
int remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

#endif /* userprog/syscall.h */
