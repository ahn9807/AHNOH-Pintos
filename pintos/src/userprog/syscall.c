#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t* esp = f->esp;
  check_address(esp);
  int arg[100];
  //get_argument(f->esp, arg, 4);
  switch (*esp)
  {
  case SYS_HALT:
    /* code */
    break;
  
  default:
    break;
  }
  printf ("system call!\n");
  thread_exit ();
}

void check_address(void *addr) {
  if((uint32_t)0x8048000 < (uint32_t)&addr || (uint32_t)0xc0000000 > (uint32_t)&addr) {
    printf("Out of user memory area [0x%x]!\n", (uint32_t *)addr);
    exit(-1);
  }
}

void get_argument(void *esp, int *arg, int count) {
  int i=0;
  uint32_t* base_esp = esp;
  for(i=0;i<count;i++) {
    arg[i] = (uint32_t *)base_esp;
    base_esp += 4;
    printf("%d\n",arg[i]);
  }
}

void halt(void) {

}

void exit(int status) {
  printf("%s: exit(%d)\n", "sdf", status);
}