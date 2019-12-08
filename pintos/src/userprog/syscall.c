#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "devices/shutdown.h" /*for shutdown_power_off */


static void syscall_handler (struct intr_frame *);
void check_address(const void *ptr_to_check);
void check_buffer(void *buffer_to_check, unsigned size);
void get_arguments(struct intr_frame *f, int *args, int args_num);
struct lock lock_filesys;
struct thread_file{
  struct list_elem file_elem;
  struct file *file_address;
  int file_descriptor;
};

void
syscall_init (void) 
{
  lock_init(&lock_filesys);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{

  int args[3];
  void *page_ptr;
  check_address((const void*) f->esp);
  switch(*(int*)f->esp){
    case SYS_HALT:
      halt();  break;
    case SYS_EXIT:
      get_arguments(f,&args[0], 1);
      exit(args[0]);
      break;
    case SYS_EXEC:
      get_arguments(f,&args[0],1);
      page_ptr = (void *) pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
      if(page_ptr ==NULL)
        exit(-1);
      args[0] = (int) page_ptr;
      f->eax=exec((const char *)args[0]); break;
    case SYS_WAIT:
      get_arguments(f,&args[0],1);
      f->eax=wait((pid_t)args[0]); break;
    case SYS_CREATE:
      get_arguments(f,&args[0],2);
      check_buffer((void*)args[0], args[1]);
      page_ptr = pagedir_get_page(thread_current()->pagedir,(const void*) args[0]);
      if(page_ptr==NULL)
        exit(-1);
      args[0] = (int) page_ptr;
      f->eax=create((const char*)args[0], (unsigned)args[1]);break;
    case SYS_REMOVE:
      get_arguments(f,&args[0],1);
      page_ptr = pagedir_get_page(thread_current()->pagedir,(const void*) args[0]);
      if(page_ptr==NULL)
        exit(-1);
      args[0] = (int) page_ptr;
      f->eax=remove((const char*) args[0]); break;
    case SYS_OPEN:
     get_arguments(f,&args[0],1);
      page_ptr = pagedir_get_page(thread_current()->pagedir,(const void*) args[0]);
      if(page_ptr==NULL)
        exit(-1);
      args[0] = (int) page_ptr;
      f->eax=open((const char*) args[0]); break;
    case SYS_FILESIZE:

      get_arguments(f, &args[0],1);
      f->eax=filesize(args[0]); break;
    case SYS_READ:
      get_arguments(f,&args[0],3);
      check_buffer((void*)args[1],args[2]);
      page_ptr = pagedir_get_page(thread_current()->pagedir,(const void*) args[1]);
      if(page_ptr==NULL)
        exit(-1);
      args[1] = (int) page_ptr;
      f->eax=read(args[0], (void*) args[1], (unsigned) args[2]); break;
    case SYS_WRITE:
      get_arguments(f,&args[0],3);
      check_buffer((void*)args[1],args[2]);
      page_ptr=pagedir_get_page(thread_current()->pagedir,(const void*)args[1]);
      if(page_ptr==NULL)
        exit(-1);
      args[1] = (int)page_ptr;
      
      f->eax=write(args[0], (void*) args[1], (unsigned) args[2]); break;
    case SYS_SEEK:
      get_arguments(f, &args[0],2);
      seek(args[0], (unsigned) args[1]); break;
    case SYS_TELL:

      get_arguments(f, &args[0],1);
      f->eax=tell(args[0]); break;
    case SYS_CLOSE:

      get_arguments(f, &args[0],1);
      close(args[0]); break;
    default:
      exit(-1); break; 

  }
}

void halt (void){
  shutdown_power_off();
}
void exit (int status){
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", thread_current()->name, status);  
  thread_exit();
}
pid_t exec (const char *cmd_line){
  if(!cmd_line)
    return -1;
  lock_acquire(&lock_filesys);
  pid_t child = process_execute(cmd_line);
  lock_release(&lock_filesys);
  return child;
}
int wait(pid_t pid){
  return process_wait(pid);
}
bool create(const char *file, unsigned initial_size){
  lock_acquire(&lock_filesys);
  bool create_status=filesys_create(file, initial_size);
  lock_release(&lock_filesys);
  return create_status;
}
bool remove(const char *file){
  lock_acquire(&lock_filesys);
  bool remove_status=filesys_remove(file);
  lock_release(&lock_filesys);
  return remove_status;
}
int open(const char *file){
  lock_acquire(&lock_filesys);
  struct file *temp = filesys_open(file);
  if(temp==NULL){
    lock_release(&lock_filesys);
    return -1;
  }
  struct thread_file *new_file = malloc(sizeof(struct thread_file));
  new_file->file_address = temp;
  int fd=thread_current()->cur_f++;
  new_file->file_descriptor= fd;
  list_push_front(&thread_current()->file_list, &new_file->file_elem);
  lock_release(&lock_filesys);
  return fd;
}
int filesize(int fd){
  struct list_elem *temp;
  lock_acquire(&lock_filesys);
  if(list_empty(&thread_current()->file_list)){
    lock_release(&lock_filesys);
    return -1;
  }
  for(temp = list_front(&thread_current()->file_list);temp!=NULL;temp=temp->next){
    struct thread_file *temp_file = list_entry(temp, struct thread_file, file_elem);
    if(temp_file->file_descriptor == fd){
      lock_release(&lock_filesys);
      return (int) file_length(temp_file->file_address);
    }
  }
  lock_release(&lock_filesys);
  return -1;
}
int read (int fd, void *buffer, unsigned length){
  struct list_elem *temp;
  lock_acquire(&lock_filesys);
  if(fd==0){
    lock_release(&lock_filesys);
    return (int) input_getc();
  }
  if(fd==1 || list_empty(&thread_current()->file_list)){
    lock_release(&lock_filesys);
    return 0;
  }
  for(temp=list_front(&thread_current()->file_list);temp!=NULL;temp=temp->next){
    struct thread_file *temp_file = list_entry(temp, struct thread_file, file_elem);
    if(temp_file->file_descriptor == fd){
      int bytes_read = (int) file_read(temp_file->file_address, buffer, length);
      lock_release(&lock_filesys);
      return bytes_read;
    }
  }
  lock_release(&lock_filesys);
  return -1;

}
int write(int fd, const void *buffer, unsigned length){
  struct list_elem *temp;
  lock_acquire(&lock_filesys);
  if(fd==1){
    putbuf(buffer,length);
    lock_release(&lock_filesys);
    return length;
  }
  if(fd==0 || list_empty(&thread_current()->file_list)){
    lock_release(&lock_filesys);
    return 0;
  }
  for(temp=list_front(&thread_current()->file_list);temp!=NULL;temp=temp->next){
    struct thread_file *temp_file = list_entry(temp, struct thread_file, file_elem);
    if(temp_file->file_descriptor == fd){
      int bytes_write = (int) file_write(temp_file->file_address,buffer,length);
      lock_release(&lock_filesys);
      return bytes_write;
    }
  }
  lock_release(&lock_filesys);
  return 0;

}
void seek(int fd, unsigned position){
  struct list_elem *temp;
  lock_acquire(&lock_filesys);
  if(list_empty(&thread_current()->file_list)){
    lock_release(&lock_filesys);
    return;
  }
  for(temp=list_front(&thread_current()->file_list);temp!=NULL;temp=temp->next){
    struct thread_file *temp_file = list_entry(temp, struct thread_file, file_elem);
    if(temp_file->file_descriptor == fd){
      file_seek(temp_file->file_address, position);      
      lock_release(&lock_filesys);
      return;
    }
  }
  lock_release(&lock_filesys);
  return;
}
unsigned tell(int fd){
  struct list_elem *temp;
  lock_acquire(&lock_filesys);
  if(list_empty(&thread_current()->file_list)){
    lock_release(&lock_filesys);
    return -1;
  }
  for(temp=list_front(&thread_current()->file_list);temp!=NULL;temp=temp->next){
    struct thread_file *temp_file = list_entry(temp, struct thread_file, file_elem);
    if(temp_file->file_descriptor == fd){
      unsigned position = (unsigned) file_tell(temp_file->file_address);
      lock_release(&lock_filesys);
      return position;
    }
  }
  lock_release(&lock_filesys);
  return -1;
}
void close(int fd){
  struct list_elem *temp;
  lock_acquire(&lock_filesys);
  if(list_empty(&thread_current()->file_list)){
    lock_release(&lock_filesys);
    return;
  }
  for(temp=list_front(&thread_current()->file_list);temp!=NULL;temp=temp->next){
    struct thread_file *temp_file = list_entry(temp, struct thread_file, file_elem);
    if(temp_file->file_descriptor == fd){
      file_close(temp_file->file_address);
      list_remove(&temp_file->file_elem);
      lock_release(&lock_filesys);
      return;
    }
  }
  lock_release(&lock_filesys);
  return;
}
void check_address(const void *ptr_to_check){
  if(!is_user_vaddr(ptr_to_check) || ptr_to_check==NULL || ptr_to_check<(void*) 0x08048000){
    exit(-1);
  }
}
void check_buffer (void *buff_to_check, unsigned size){
  char *ptr = (char *) buff_to_check;
  for(unsigned int i=0;i<size;i++){
    check_address((const void*)ptr);
    ptr++;
  }
}
void get_arguments(struct intr_frame *f, int *args, int args_num){
  int *ptr;
  for (int i=0;i<args_num;i++){
    ptr=(int *)f->esp+i+1;
    check_address((const void*)ptr);
    args[i]=*ptr;
  }
}
