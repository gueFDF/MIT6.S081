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

// 用于访问物理页引用计数数组
#define PA2PGREF_ID(p) (((p)-KERNBASE) / PGSIZE)
#define PGREF_MAX_ENTRIES PA2PGREF_ID(PHYSTOP)

struct spinlock pgreflock;      // 用于 pageref 数组的锁，防止竞态条件引起内存泄漏
int pageref[PGREF_MAX_ENTRIES]; // 从 KERNBASE 开始到 PHYSTOP 之间的每个物理页的引用计数

void incr(uint64 va)
{
  va = PGROUNDDOWN(va);
  int index = (va - KERNBASE) / PGSIZE;
  acquire(&pgreflock); //上锁
 // printf("page++前 %d is %d\n", index, pageref[index]);
  pageref[index]++;
  release(&pgreflock); //解锁
 // printf("page++ %d is %d\n", index, pageref[index]);
}

void decr(uint64 va)
{
  va = PGROUNDDOWN(va);
  int index = (va - KERNBASE) / PGSIZE;
  acquire(&pgreflock); //上锁
  pageref[index]--;
  release(&pgreflock); //解锁
                       // printf("page-- %d is %d\n",index,pageref[index]);
}

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
} kmem;

void kinit()
{
  printf("%d\n", PGREF_MAX_ENTRIES);
  initlock(&pgreflock, "pgref"); // 初始化锁
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
  for (int i = 0; i < PGREF_MAX_ENTRIES; i++)
  {
    pageref[i] = 0;
  }
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;
  int index=(PGROUNDDOWN((uint64)pa) - KERNBASE) / PGSIZE;
  if(--pageref[index]>0)
    return;
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  // release(&pgreflock);
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
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r)
  {
    memset((char *)r, 5, PGSIZE); // fill with junk
    incr((uint64)r);
  }
  return (void *)r;
}
