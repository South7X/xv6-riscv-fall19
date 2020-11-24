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
int get_cpuid();

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};
struct kmem kmems[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++)
    initlock(&kmems[i].lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  // PGROUNDUP确保空闲内存4K对齐 *
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    //将end～PHYSTOP之间的地址空间按照页面大小切分并调用kfree()将页面从头部插入到链表kmem.freelist中进行管理 *
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
  int kmemid = get_cpuid();
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  // Free mem to current CPU's freelist *
  acquire(&kmems[kmemid].lock);
  r->next = kmems[kmemid].freelist;
  kmems[kmemid].freelist = r;
  release(&kmems[kmemid].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  int kmemid = get_cpuid();
  // Allocate current cpu mem in its freelist *
  acquire(&kmems[kmemid].lock);
  r = kmems[kmemid].freelist;
  if(r)
    kmems[kmemid].freelist = r->next;
  release(&kmems[kmemid].lock);
  // Current cpu freelist is null *
  if(!r){
    for (int i = 0; i < NCPU; i ++) {   // Find in other cpu's freelist *
      acquire(&kmems[i].lock);
      r = kmems[i].freelist;
      if(r)
        kmems[i].freelist = r->next;
      release(&kmems[i].lock);

      if (r)  break;      // Already stolen one mem from other cpu freelist *
    }
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int get_cpuid()
{
  push_off();
  int id = cpuid();
  pop_off();
  return id;
}