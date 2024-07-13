# mmap

本实验要求我们实现一个mmap和munmap。具体功能可查看`man mmap`。

## 定义VMA

```c
#define VMA_MAX 16
struct VMA{
  int valid;
  uint64 addr;
  int len;
  int prot;
  int flags;
  int off;
  struct file* f;
  uint64 mapcnt;
};
```


将vma在proc中声明

```c
struct proc {
 //vma
  struct VMA vma[16];
  uint64 vma_top;
};
```


## 增加系统调用

按照实验指导，首先增加mmap和munmap系统调用。然后实现该系统调用。

mmap的系统调用就是将传递进的参数保存进`proc.h`的vma结构体中，因此需要在vma中找到一个空的vma保存参数，然后调用`filedup(f)`增加文件的引用计数。最后返回vmat->addr即可。

```c
uint64 
sys_mmap(void)
{
  uint64 addr;
  int len, prot, flags, fd, offset;
  argaddr(0, &addr), argint(1, &len), argint(2, &prot),
      argint(3, &flags), argint(4, &fd), argint(5, &offset);

  addr = PGROUNDDOWN(addr);

  len = PGROUNDUP(len);

  struct proc* p = myproc();
  struct file *f = p->ofile[fd];

  if (f->writable == 0 && (prot & PROT_WRITE) && flags == MAP_SHARED) return -1;

  struct VMA *vmat = 0;

  for (int i = 0; i < VMA_MAX;i++)
    if(p->vma[i].valid==0){
        vmat = &p->vma[i];
        break;
    }

  if (vmat == 0) return -1;

  p->vma_top -= len;
  vmat->valid = 1;
  vmat->addr = p->vma_top;
  vmat->len = len;
  vmat->prot = prot;
  vmat->flags = flags;
  vmat->f = f;
  vmat->off = offset;

  filedup(f);
  return vmat->addr;
}
```


munmap系统调用的功能是释放[addr,addr+len]，这段内存空间。因此只需知道对应的vma，然后检查是否有效，然后将内容写入文件。调用`uvmunmap`删除对应的地址空间映射。最后检查是否删除对应vma的全部已分配空间，如果是就关闭文件，释放vma。


```c

uint64 
sys_munmap(void)
{
  uint64 addr;
  int len;
  argaddr(0, &addr), argint(1, &len);
  addr = PGROUNDDOWN(addr);

  struct proc *p = myproc();
  struct VMA *vmat = 0;
 
  for (int i = 0; i < VMA_MAX; i++) {
      if (p->vma[i].valid && p->vma[i].addr <= addr && p->vma[i].addr + p->vma[i].len >= addr) {
          vmat = &p->vma[i];
          break;
      }
  }

  if (vmat == 0) return 0;

  if (vmat->flags == MAP_SHARED && (vmat->prot & PROT_WRITE) != 0) {
      filewrite(vmat->f, addr, len);
  }

  uvmunmap(p->pagetable, addr, len / PGSIZE, 1);

  vmat->mapcnt -= len;
  if (vmat->mapcnt == 0) {
      fileclose(vmat->f);
      vmat->valid = 0;
  }

  return 0;
}
```


## lazy allocation

在mmap系统调用中，我们仅是保存了对应的参数，增加了文件的引用计数，但是并没有为其分配地址空间。因此在访问到对应的地址空间时会出现读写异常。

此时在trap.c中我们要处理这部分错误，分配空间并完成映射。处理这部分错误的地点和cow实验处理错误的地点一致。

首先检查是否读写错误。然后获得访问的addr。从p->vma中找到对应的vma，如果没找到，可以判断就是访问非法的地址，此时`kill（p）`。

然后分配一个新的页（注意调用memset初始化为0，我在这里卡了半天），然后调用mappages将地址映射进进程的地址空间。将文件对应地址的内容读进该页。

```c
syscall();
  }else if(r_scause()==13||r_scause()==15){
    // 读写异常，需要分配内存
    uint64 addr = r_stval();
    struct VMA *vma = 0;
    for (int i = 0; i < VMA_MAX;i++){
      if (p->vma[i].valid && p->vma[i].addr <= addr && p->vma[i].addr + p->vma[i].len >= addr) {
        vma = &p->vma[i];
        break;
      }
    }

    if (vma == 0){
      printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
      printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
      setkilled(p);
    }else{
      uint64 newpage = (uint64)kalloc();
      memset((void *)newpage, 0, PGSIZE);
      if (mappages(p->pagetable, addr, PGSIZE, newpage, PTE_U | PTE_V | (vma->prot << 1)) < 0) {
          printf("mmappage filed\n");
          setkilled(p);
      } else {
          vma->mapcnt += PGSIZE;
          ilock(vma->f->ip);
          readi(vma->f->ip, 0, newpage, addr - vma->addr, PGSIZE);
          iunlock(vma->f->ip);
      }
    }

  } else if((which_dev = devintr()) != 0){
```


## 处理进程相关

### allocproc

首先在创建进程时，要`p->vma.valid`初始化为0。`p->vma_top`初始化为MAXVA-2*PSGSIZE。

```c
 for (int i = 0; i < VMA_MAX; i++) p->vma[i].valid = 0;
  p->vma_top = MAXVA - 2 * PGSIZE;
```

### exit

在exit时，要将vma写入文件，取消映射。这里我是将[addr,addr+len]的地址每次检查是否当前进程的地址空间，如果是，在调用uvmunmap。~~考虑到可能是分配完一组相邻的页后，将中间的页先释放掉了，然后如果一次性unmap所有的页，先前释放掉的页可能会发生unmatch的情况。~~

```c
 for (int i = 0; i < VMA_MAX; i++) {
      if (p->vma[i].valid) {
          if (p->vma[i].flags == MAP_SHARED && (p->vma[i].prot & PROT_WRITE) != 0) {
              filewrite(p->vma[i].f, p->vma[i].addr, p->vma[i].len);
          }
          
          for (uint64 a = p->vma[i].addr; a < p->vma[i].addr + p->vma[i].len;a+=PGSIZE)
            if(walkaddr(p->pagetable,a))
                uvmunmap(p->pagetable, PGROUNDDOWN(a), 1, 1);
                
          fileclose(p->vma[i].f);
          p->vma[i].valid = 0;
      }
  }

  // Close all open files.
```

### fork

最后处理fork，在fork时，要将父进程的vma拷贝给子进程，同时增加文件的引用计数。


```c
 np->vma_top = p->vma_top;
  for (i = 0; i < VMA_MAX; i++) {
      if (p->vma[i].valid) {
        filedup(p->vma[i].f);
        memmove(&np->vma[i], &p->vma[i], sizeof(struct VMA));
      }
  }

  safestrcpy(np->name, p->name, sizeof(p->name));

```