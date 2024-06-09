# lock

## Memory allocator

本实验要求我们实现一个页表的管理机制，以支持更好的并发性能。因此我们要将页表分散给不同的CPU，每个CPU申请并释放自己的页表，这样不同CPU之间不至于因为同时请求页表从而等待。

首先观察到NCPU=8，因此我们需要为每一个CPU维护一个freelist。

在初始化时，要初始化8把锁。

```c
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

char *kmem_lock_names[] = {
  "kmem_cpu_0",
  "kmem_cpu_1",
  "kmem_cpu_2",
  "kmem_cpu_3",
  "kmem_cpu_4",
  "kmem_cpu_5",
  "kmem_cpu_6",
  "kmem_cpu_7",
};

void
kinit()
{
  for (int i = 0; i < NCPU; i++)
    initlock(&kmem[i].lock, kmem_lock_names[i]);
  freerange(end, (void *)PHYSTOP);
}
```

在释放页表时，要获得当前cpu的id，然后将其插入到对应的freelist中。

```c
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();  //关中断
  int id = cpuid();
  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);

  pop_off();  //开中断
}

```

在请求页表时，如果遇到页表不足的情况，需要从其他list中hack（窃取）页表，我这里固定窃取64个页表

```c
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int id = cpuid();
  acquire(&kmem[id].lock);

  if(!kmem[id].freelist) { 
    int sum = 64; 
    for (int i = 0; i < NCPU; i++) {
        if (i == id) continue;
        acquire(&kmem[i].lock);
        struct run *rr = kmem[i].freelist;
        while (rr && sum) {
            kmem[i].freelist = rr->next;
            rr->next = kmem[id].freelist;
            kmem[id].freelist = rr;
            rr = kmem[i].freelist;
            sum--;
        }
        release(&kmem[i].lock);
        if (sum == 0) break;  
    }
  }

  r = kmem[id].freelist;

  if(r)
    kmem[id].freelist = r->next;
    
  release(&kmem[id].lock);
  pop_off();

  if (r)
    memset((char *)r, 5, PGSIZE);  // fill with junk
  return (void*)r;
}
```

















