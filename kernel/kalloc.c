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
extern struct cpu cpus[NCPU];

struct run {
  struct run *next;
};

// only cpu0 will call this, so there is no need to lock
// after kinit is called, 
void
kinit()
{
  for(int i = 0; i < NCPU; i++)
    initlock(&cpus[i].kmem.lock, "kmem");
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
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  struct cpu *c = mycpu();
  acquire(&c->kmem.lock);
  r->next = c->kmem.freelist;
  c->kmem.freelist = r;
  release(&c->kmem.lock);
  pop_off();
}

// cpu C steal memory from other cpus
void* steal(struct cpu *C)
{
  struct run *r = 0;
  for(struct cpu *c = cpus; c < cpus + NCPU; c++)
  {
    if(c != C)
    {
      acquire(&c->kmem.lock);
      if(c->kmem.freelist)
      {
        r = c->kmem.freelist;
        c->kmem.freelist = r->next;// remove r from the free list of c
        // there is no need to put r into the free list of C
        // because r is allocated, when we kfree r
        release(&c->kmem.lock);
        break;
      }
      release(&c->kmem.lock);
    }
  }
  return r; // in case NCPU = 1
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  struct cpu *c = mycpu();
  acquire(&c->kmem.lock);
  r = c->kmem.freelist;
  if(r)
    c->kmem.freelist = r->next;
  else r = steal(c);
  release(&c->kmem.lock);
  pop_off();
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
