#include "vm/page.h"
#include "vm/frame.h"
#include <hash.h>
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "devices/block.h"
#include "filesys/file.h"
#include <string.h>


/* Hash function for the hash table */
static unsigned page_hash(const struct hash_elem *e, void *aux UNUSED) {
    const struct page *p = hash_entry(e, struct page, hash_elem);
    return hash_bytes(&p->va, sizeof p->va);
}

/* Hash comparison function for the hash table */
static bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
    const struct page *p1 = hash_entry(a, struct page, hash_elem);
    const struct page *p2 = hash_entry(b, struct page, hash_elem);
    return p1->va < p2->va;
}

/* Initialize supplemental page table. */
void
supp_page_table_init (struct hash *spt)
{
    hash_init (spt, page_hash, page_less, NULL);
}

/* Find the page that corresponds to the given virtual address */
struct page *spt_find_page(struct supplemental_page_table *spt, void *va) {
    struct page p;
    struct hash_elem *e;
    p.va = va;
    e = hash_find(&spt->page_table, &p.hash_elem);
    return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert a page into the supplemental page table */
bool spt_insert_page(struct supplemental_page_table *spt, struct page *page) {
    struct hash_elem *result = hash_insert(&spt->page_table, &page->hash_elem);
    return result == NULL;
}

/* Additional necessary code for handling hash table destruction */
void supplemental_page_table_destroy(struct supplemental_page_table *spt) {
    hash_destroy(&spt->page_table, NULL);
}

/* Helper function to copy a page */
static bool copy_page(struct supplemental_page_table *dst, struct page *src_page) {
    // Allocate a new page structure
    struct page *new_page = malloc(sizeof(struct page));
    if (new_page == NULL) {
        return false;
    }

    // Copy the contents of the source page to the new page
    memcpy(new_page, src_page, sizeof(struct page));
    
    // Insert the new page into the destination supplemental page table
    return spt_insert_page(dst, new_page);
}

/* Copies the supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst, struct supplemental_page_table *src) {
    struct hash_iterator i;

    hash_first(&i, &src->page_table);
    while (hash_next(&i)) {
        struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);

        if (!copy_page(dst, src_page)) {
            return false;
        }
    }
    return true;
}

/* Destroys each page in the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt) {
    struct hash_iterator i;

    hash_first(&i, &spt->page_table);
    while (hash_next(&i)) {
        struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
        // Assume destroy_page function exists to clean up the page
        destroy_page(p);
    }

    // Destroy the hash table itself
    hash_destroy(&spt->page_table, NULL);
}


/* Destroys a page and frees its resources */
void destroy_page(struct page *p) {
    if (p == NULL) {
        return;
    }

    // Free the physical memory associated with the page, if any
    if (p->frame != NULL) {
        palloc_free_page(p->frame->kva);
    }

    // Free the page structure itself
    free(p);
}



/* Helper function to fetch data into the frame. */
static bool fetch_page_data(struct page *p, void *frame) {
    switch (p->type) {
        case PAGE_ZERO:
            memset(frame, 0, PGSIZE);
            return true;
        case PAGE_FILE:
            if (file_read_at(p->file, frame, p->read_bytes, p->file_offset) != (int)p->read_bytes) {
                return false;
            }
            memset(frame + p->read_bytes, 0, p->zero_bytes);
            return true;
        case PAGE_SWAP:
            // Handle swap in (not implemented here).
            return false;
        default:
            return false;
    }
}

/* Handles page faults by fetching the required page into memory. */
static bool vm_try_handle_fault(struct supplemental_page_table *spt, void *fault_addr, bool write) {
    /* Locate the page that faulted in the supplemental page table. */
    struct page *p = spt_find_page(spt, fault_addr);
    if (p == NULL) {
        return false;
    }

    /* Check if the memory access is valid. */
    if (!p->writable && write) {
        return false;
    }

    /* Allocate a frame to store the page. */
    void *frame = allocate_frame();
    if (frame == NULL) {
        return false;
    }

    /* Fetch the data into the frame. */
    if (!fetch_page_data(p, frame)) {
        palloc_free_page(frame);
        return false;
    }

    /* Point the page table entry for the faulting address to the physical page. */
    if (!pagedir_set_page(thread_current()->pagedir, fault_addr, frame, p->writable)) {
        palloc_free_page(frame);
        return false;
    }

    /* Update the page's frame pointer. */
    p->frame = frame;
    return true;
}
