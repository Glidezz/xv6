# thread

## Uthread: switching between threads

本实验要求我们实现一个用户级线程，看似很难，其实非常简单。

首先是要完成寄存器切换的程序，这部分代码可以直接参考[switch.S](../kernel/swtch.S)的代码，即可完成寄存器的切换工作。

然后我们要在thread中保存上下文（即寄存器）信息。因此我们需要在thread中保存context。

```c
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
  struct context context;
};
```

在创建线程时，我们要将context的中的ra指向func，并且将sp指向用户栈的栈顶。

这里要注意栈底是高地址，栈顶是低地址。

```c
void 
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->state = RUNNABLE;
  // YOUR CODE HERE
  t->context.ra = (uint64)func;
  t->context.sp = (uint64)(&t->stack[STACK_SIZE]);
}
```

然后要完成context的切换工作，
```c
 if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
    thread_switch((uint64)&t->context, (uint64)&next_thread->context);
  } else
      next_thread = 0;
```

## Using threads

在使用线程这部分实验，只需要在访问临界资源的位置及时加锁，然后逐步降低加锁的粒度，即可完成实验。

```c

static 
void put(int key, int value)
{
  int i = key % NBUCKET;
  
  // is the key already present?
  struct entry *e = 0;
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key)
      break;
  }
  pthread_mutex_lock(&lock);  // acquire lock
  if(e){
    // update the existing key.
    e->value = value;
  } else {
    // the new is new.
    insert(key, value, &table[i], table[i]);
  }
  pthread_mutex_unlock(&lock);  // release lock
}

static struct entry*
get(int key)
{
  int i = key % NBUCKET;
  pthread_mutex_lock(&lock);  // acquire lock

  struct entry *e = 0;
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key) break;
  }
  pthread_mutex_unlock(&lock);  // release lock

  return e;
}
```

## Barrier

这里要我们使用linux的信号量机制，如果调用barrier的线程个数小于nthread那么此时时的线程等待（sleep），直到所有的线程都调用barrier后，
唤醒所有的线程并开始下一轮的执行。


```c

static void 
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  pthread_mutex_lock(&bstate.barrier_mutex);  // acquire lock
  if(++bstate.nthread < nthread) {
      pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  } else {
      bstate.nthread = 0;
      bstate.round++;
      pthread_cond_broadcast(&bstate.barrier_cond);
  }
  pthread_mutex_unlock(&bstate.barrier_mutex);  // release lock
  
}
```
