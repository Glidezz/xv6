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

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char count[NPAGES];
} kmem;
/*
  这里使用两把锁可能会有资源竞争，最后导致死锁
  索性只使用一把锁管理资源，降低并发度，从而减少资源竞争
  // 前几次测试会偶尔出现bug从而无法通过usertests。但是在后来的测试中没有出现bug
  // maybe the bug is due to my environment（2核2g）😭
  // qemu 的默认参数是CPUS=3 
*/

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kmem.count[((uint64)p - KERNBASE) / PGSIZE] = 1;  // 和kfree函数搭配使用就会使得初始值初始化为0
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  desc((uint64)pa);
  if (kmem.count[((uint64)pa - KERNBASE) / PGSIZE] == 0) {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run *)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    incr((uint64)r);
  }
  return (void *)r;
}

void 
incr(uint64 pa)
{
  acquire(&kmem.lock);
  kmem.count[(pa - KERNBASE) / PGSIZE] += 1;
  release(&kmem.lock);
}

void 
desc(uint64 pa)
{
  acquire(&kmem.lock);
  kmem.count[(pa - KERNBASE) / PGSIZE] -= 1;
  release(&kmem.lock);
}