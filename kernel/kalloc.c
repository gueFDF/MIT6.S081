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
} kmem[NCPU];

void kinit()
{
  for (int i = 0; i < NCPU; i++) //给每一个CPU的锁进行初始化
  {
    initlock(&kmem[i].lock, "kmem");
  }
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  int id = 0;
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) //循环给每个CPU平均分配page
  // kfree(p);
  {
    void *pa = p;
    struct run *r;
    if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
      panic("kfree");
    memset(pa, 1, PGSIZE);

    r = (struct run *)pa;

    acquire(&kmem[id].lock);
    r->next = kmem[id].freelist;
    release(&kmem[id].lock);
    kmem[id].freelist = r;
    ++id;
    if (id == NCPU)
      id = 0;
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;
  push_off();
  int id = cpuid(); //获取当前CPU的id
  pop_off();
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int id = cpuid(); //获取当前CPU的id
  pop_off();
  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if (r)
    kmem[id].freelist = r->next;
  release(&kmem[id].lock);
  if (!r)
  {
    //从其他CPU那或获取
    for (int i = 0; i < NCPU; i++)
    {
      acquire(&kmem[i].lock);
      r = kmem[i].freelist;
      if (r)
        kmem[i].freelist = r->next;
      release(&kmem[i].lock);
      if(r)
        break;
    }
  }
  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
