#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "filesys/inode.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/input.h"

struct lock file_lock;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t* esp = f->esp;
  uint32_t arg[3];
  //hex_dump(f->esp, f->esp, 1000, 1); 
  switch (*esp)
  {
  case SYS_HALT:
    /* code */
    halt();
    break;
  case SYS_EXIT:
    get_argument(esp, arg, 1);
    exit(arg[0]);
    break;
  case SYS_WRITE:
    get_argument(esp, arg, 3);
    f->eax = write(arg[0], arg[1], arg[2]);
    break;
  case SYS_EXEC:
    get_argument(esp, arg, 1);
    f->eax = exec(arg[0]);
    break;
  case SYS_WAIT:
    get_argument(esp, arg, 1);
    f->eax = wait(arg[0]);
    break;
  case SYS_CREATE:
    get_argument(esp, arg, 2);
    f->eax = create(arg[0], arg[1]);
    break;
  case SYS_REMOVE:
    get_argument(esp, arg, 1);
    f->eax = remove(arg[0]);
    break;
  case SYS_OPEN:
    get_argument(esp, arg, 1);
    f->eax = open(arg[0]);
    break;
  case SYS_FILESIZE:
    get_argument(esp, arg, 1);
    f->eax = filesize(arg[0]);
    break;
  case SYS_READ:
    get_argument(esp, arg, 3);
    f->eax = read(arg[0], arg[1], arg[2]);
    break;
  case SYS_SEEK:
    get_argument(esp, arg, 2);
    seek(arg[0], arg[1]);
    break;
  case SYS_TELL:
    get_argument(esp, arg, 1);
    f->eax = tell(arg[0]);
    break;
  case SYS_CLOSE:
    get_argument(esp, arg, 1);
    close(arg[0]);
    break; 
  default:
    break;
  }

  //printf ("system call!\n");
  //thread_exit ();
}

void check_address(void *addr) {
  if((uint32_t)0x8048000 > (uint32_t *)addr || (uint32_t)0xc0000000 < (uint32_t *)addr) {
    printf("Out of user memory area [0x%x]!\n", (uint32_t *)addr);
    exit(-1);
  }
}

void get_argument(void *esp, int *arg, int count) {
  int i=0;
  uint32_t* base_esp = (uint32_t *)esp + 1;
  for(i=0;i<count;i++) {
    check_address(base_esp);
    arg[i] = *base_esp;
    base_esp += 1;
  }
}

void halt(void) {
  shutdown_power_off ();
}

void exit(int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

//pid_t exec
int exec(const char *cmd_line) {
  return process_execute(cmd_line);
}

int wait (int pid) {
  return process_wait(pid);
}

//bool create
int create (const char *file, unsigned initial_size) {
  int returnVal;
  
  lock_acquire(&file_lock);
  returnVal = filesys_create(file, initial_size);
  lock_release(&file_lock);

  return returnVal;
}

//bool remove
int remove (const char *file) {
  int returnVal;
  lock_acquire(&file_lock);
  returnVal = filesys_remove(file);
  lock_release(&file_lock);

  return returnVal;
}

int open (const char *file) {
  struct file* file_pointer;
  int returnVal;
  lock_acquire(&file_lock);
  file_pointer = filesys_open(file);
  printf("0x%x\n", file_pointer);
  returnVal = insert_file(file_pointer);
  //printf("[%d] 0x%x\n", 3, file_from_fd(3));
  lock_release(&file_lock);

  return returnVal;
}

int filesize (int fd) {
  int returnVal;
  lock_acquire(&file_lock);
  //printf("[%d] 0x%x\n", fd, file_from_fd(fd));
  returnVal = file_length(file_from_fd(fd));
  lock_release(&file_lock);

  return returnVal;
}

int read (int fd, void *buffer, unsigned size) {
  if(fd == STDIN) {
    *(uint32_t *)buffer = input_getc();
    size++;
    return size;
  }

  int returnVal;

  lock_acquire(&file_lock);
  returnVal = file_read(file_from_fd(fd), buffer, size);
  lock_release(&file_lock);

  return returnVal;
}

int write(int fd, const void *buffer, unsigned size) {
  if(fd == STDOUT) {
    putbuf(buffer, size);
    return size;
  }

  int returnVal;

  lock_acquire(&file_lock);
  returnVal = file_write(file_from_fd(fd), buffer, size);
  lock_release(&file_lock);

  return returnVal;
}

void seek (int fd, unsigned position) {
  lock_acquire(&file_lock);
  file_seek(file_from_fd(fd), position);
  lock_release(&file_lock);
}

unsigned tell (int fd) {
  return file_tell(file_from_fd(fd)) + 1;
}

void close (int fd) {
  lock_acquire(&file_lock);
  file_close(file_from_fd(fd));
  lock_release(&file_lock);
}