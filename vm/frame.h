#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include <hash.h>
#include "threads/palloc.h"
#include "threads/thread.h"

/* Frame structure to represent a physical frame. */
struct frame {
    void *kva;             /* Kernel virtual address of the frame. */
    bool in_use;           /* Flag to indicate if the frame is in use. */
    // Add any other necessary fields
};

/* Function prototypes */
void frame_init(void);

void *allocate_frame(void);

void free_frame(void *frame);

#endif /* VM_FRAME_H */

