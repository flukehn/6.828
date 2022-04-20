// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct page_info {
  int _count;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  struct page_info info[PHYSTOP / PGSIZE];
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  acquire(&kmem.lock);
  for (int i = 0; i < PHYSTOP / PGSIZE; ++i)
    kmem.info[i]._count = 1;
  release(&kmem.lock);
  freerange(end, (void*)PHYSTOP);
}
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.

  r = (struct run*)pa;
  acquire(&kmem.lock);
  if (kmem.info[(uint64)pa/PGSIZE]._count < 1) {
    panic("kfree : reference count < 1\n");
  }
  if ((--kmem.info[(uint64)pa / PGSIZE]._count) == 0) {
    memset(pa, 1, PGSIZE);
    r->next = kmem.freelist;
    kmem.freelist = r;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    kmem.info[(uint64)r/PGSIZE]._count = 1;
    //printf("kalloc %p\n", r);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
kpage_count_add(void *pa, int v) {
  acquire(&kmem.lock);
  //printf("cow page %p: from %d, add  %d\n", pa, kmem.info[(uint64)pa/PGSIZE]._count, v);
  kmem.info[(uint64)pa/PGSIZE]._count+=v;
  release(&kmem.lock);
}

int
kpage_cow(void *pa) {
  acquire(&kmem.lock);
  int c=kmem.info[(uint64)pa/PGSIZE]._count;
  release(&kmem.lock);
  return c;
}
