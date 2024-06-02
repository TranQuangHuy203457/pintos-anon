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
  printf ("system call!\n");
  thread_exit ();
}

void exit(int status) {
    struct thread *cur = thread_current();
    struct thread *parent = thread_get_by_id(cur->parent_id);
    struct list_elem *e;
    struct child_status *child;

    if (parent != NULL) {
        for (e = list_begin(&parent->children); e != list_end(&parent->children); e = list_next(e)) {
            child = list_entry(e, struct child_status, elem);
            if (child->child_id == cur->tid) {
                lock_acquire(&parent->lock_child);
                child->is_exit_called = true;
                child->child_exit_status = status;
                lock_release(&parent->lock_child);
                break;
            }
        }
    }
    thread_exit();

}

