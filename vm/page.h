#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "threads/thread.h"
#include "filesys/file.h"
#include "devices/block.h"

/* Enum to represent the source of the page's data. */
enum page_type {
    PAGE_ZERO,       /* Zeroed page. */
    PAGE_FILE,       /* Page that is read from a file. */
    PAGE_SWAP        /* Page that is read from swap. */
};

/* Page structure to represent a page in the supplemental page table. */
struct page {
    void *va;                      /* Virtual address of the page. */
    struct hash_elem hash_elem;    /* Hash table element. */
    struct frame *frame;           /* Physical frame associated with the page.*/
    enum page_type type;           /* Type of page data source. */
    struct file *file;             /* File to read from (if type is PAGE_FILE).*/
    off_t file_offset;             /* Offset in the file. */
    size_t read_bytes;             /* Number of bytes to read from file. */
    size_t zero_bytes;             /* Number of bytes to zero. */
    bool writable;                 /* Is the page writable? */
};

/* Supplemental page table structure. */
struct supplemental_page_table {
    struct hash page_table;        /* Hash table to store pages. */
};

/* Function prototypes for supplemental page table management. */
void supplemental_page_table_init(struct supplemental_page_table *spt);
struct page *spt_find_page(struct supplemental_page_table *spt, void *va);
bool spt_insert_page(struct supplemental_page_table *spt, struct page *page);
void supplemental_page_table_destroy(struct supplemental_page_table *spt);
bool supplemental_page_table_copy(struct supplemental_page_table *dst, struct supplemental_page_table *src);
void supplemental_page_table_kill(struct supplemental_page_table *spt);
void destroy_page(struct page *p);

/* Function prototype for page fault handling. */
bool vm_try_handle_fault(struct supplemental_page_table *spt, void *fault_addr, bool write);

/* Function prototype for supplemental page table initialization */
void supp_page_table_init(struct hash *spt);

#endif /* vm/page.h */
