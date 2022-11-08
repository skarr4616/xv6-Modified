// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
  uint refC[PHYSTOP >> PGSHIFT];
} kmem;

void kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    kmem.refC[(uint64)p >> PGSHIFT] = 0;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  r = (struct run *)pa;

  acquire(&kmem.lock);
  if (kmem.refC[(uint64)r >> PGSHIFT] > 0)
    kmem.refC[(uint64)r >> PGSHIFT] -= 1;

  if (kmem.refC[(uint64)r >> PGSHIFT] == 0)
  {
    // Fill with junk to catch dangling refs.
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
  if (r)
  {
    if (kmem.refC[(uint64)r >> PGSHIFT] != 0)
      panic("kalloc: non-zero page reference counter");

    kmem.freelist = r->next;
    kmem.refC[(uint64)r >> PGSHIFT] = 1; // reference count page = one when allocated
  }
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

void inc_refC(uint64 pa)
{
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("inc_refC");

  acquire(&kmem.lock);

  kmem.refC[pa >> PGSHIFT] += 1;

  release(&kmem.lock);
}
