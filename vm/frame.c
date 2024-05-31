#include "vm/frame.h"
#include "threads/palloc.h"

/* Initialize the frame table. */
void frame_init(void) {
    // Initialize frame-related data structures
    // For example, initialize a list of frames or any other data structure you need
}

/* Allocate a frame. */
void *allocate_frame(void) {
    void *frame = palloc_get_page(PAL_USER);
    if (frame == NULL) {
        PANIC("Out of memory!");
    }
    return frame;
}

/* Free a frame. */
void free_frame(void *frame) {
    palloc_free_page(frame);
}

