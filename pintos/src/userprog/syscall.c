#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/*add for projec 2*/
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"
#include <string.h>
typedef int pid_t;


static void syscall_handler (struct intr_frame *);

/*add for project2 systemcall function*/
bool is_valid_stackpointer(void *stack_pointer);
void syscall_halt(void);
void syscall_exit(int status);
pid_t syscall_exec(const char *cmd_line);
int syscall_wait(pid_t pid);
bool syscall_create(const char *file, unsigned initial_size);
bool syscall_remove(const char *file);
int syscall_open(const char *file);
int syscall_filesize(int fd);
int syscall_read(int fd,void *buffer,unsigned size);
int syscall_write(int fd,const void *buffer,unsigned size);
void syscall_seek(int fd,unsigned position);
unsigned syscall_tell(int fd);
void syscall_close(int fd);




void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  printf ("system call!\n");
  uint32_t *stack_pointer = f->esp;

  /*check the stack pointer is valid or not, if false, exit, else go further*/
  if(!is_valid_stackpointer(stack_pointer))
    {
      syscall_exit(-1);
    }


  int syscall_number = *(stack_pointer);



  switch (syscall_number) {
    case (SYS_HALT):
                syscall_halt();
                break;
    case (SYS_EXIT):
                syscall_exit(*(stack_pointer+1));
                break;
    case (SYS_EXEC):
                break;
    case (SYS_WAIT):
                break;
    case (SYS_CREATE):
        f->eax = syscall_create((char *)*(stack_pointer+1),*(stack_pointer+2));
        break;
    case (SYS_REMOVE):
            f->eax = syscall_remove((char *)*(stack_pointer+1));
                break;
    case (SYS_OPEN):
                break;
    case (SYS_FILESIZE):
                break;
    case (SYS_READ):
                break;
    case (SYS_WRITE):
                break;
    case (SYS_SEEK):
                break;
    case (SYS_TELL):
                break;
    case (SYS_CLOSE):
                break;
  }

}

/*Each function detail*/
bool is_valid_stackpointer(void *stack_pointer)
{
   struct thread *current_thread = thread_current();

   if(is_user_vaddr(stack_pointer) && pagedir_get_page(current_thread->pagedir,stack_pointer)!= NULL)
      {return true;}
   else {return false;}
}


void syscall_halt(void)
{
  shutdown_power_off();
}
void syscall_exit(int status)
{
  thread_exit();

}
pid_t syscall_exec(const char *cmd_line);
int syscall_wait(pid_t pid);


bool syscall_create(const char *file, unsigned initial_size)
{
  bool return_value;

  /*check the file neam pointer*/
  if(!is_valid_stackpointer(file))
  {syscall_exit(-1);}

  /*created a file*/
  return_value = filesys_create(file,initial_size);

  return return_value;
}
bool syscall_remove(const char *file)
{
    bool return_value;

    return_value = filesys_remove(file);

    return return_value;

}
int syscall_open(const char *file);
int syscall_filesize(int fd);
int syscall_read(int fd,void *buffer,unsigned size);
int syscall_write(int fd,const void *buffer,unsigned size);
void syscall_seek(int fd,unsigned position);
unsigned syscall_tell(int fd);
void syscall_close(int fd);
