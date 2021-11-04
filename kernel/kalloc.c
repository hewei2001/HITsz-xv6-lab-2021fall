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

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

// 为每个 CPU 分配内存链表，减少争抢
struct kmem kmems[NCPU];

void
kinit()
{
  // 初始化每一个 CPU 的结构体
  for(int i = 0; i < NCPU; i++)
    initlock(&kmems[i].lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p); // 这里其实可以优化，因为首次调用 freerange 时的 CPU 会分走所有的 page，不能平均分
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

  r = (struct run*)pa; // 强转为 run 用于 freelist

  // 获取当前 CPU 的 id 需要关中断
  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  release(&kmems[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  // 获取当前 CPU 的 id 需要关中断
  push_off();
  int id = cpuid();
  pop_off();

  // acquire(&kmems[id].lock);
  // r = kmems[id].freelist;
  // if(r)
  //   kmems[id].freelist = r->next;
  // release(&kmems[id].lock);
  // 可以先遍历当前 CPU 再遍历其他 CPU，也可以从当前开始一次遍历全部

  // 遍历所有 CPU 的 freelist 
  for (int i = id; i < NCPU + id; i++) {
    int j = i % NCPU; // 取模，循环访问

    acquire(&kmems[j].lock);
    r = kmems[j].freelist;
    if (r) // 如果当前 freelist 有 page 就分配
      kmems[j].freelist = r->next;
    release(&kmems[j].lock);

    if (r) { // 清空该 page 并退出遍历
      memset((char*)r, 5, PGSIZE); // fill with junk
      break;
    }
  }

  return (void*)r;
}
