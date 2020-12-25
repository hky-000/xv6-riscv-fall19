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

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU]; // 多核
// 为每个CPU维护一个空闲链表，每个链表有专用的锁
// 自旋锁：bcache.lock用于表示当前访问的bcache缓存块数据结构的是否被锁住
// 睡眠锁：b.lock用于表示bcache缓存块数据结构中的当前缓存数据块buf是否被锁住

void
kinit()
{
  for(int i=0; i<NCPU; i++){
    initlock(&kmems[i].lock, "kmem");
  }
  freerange(end, (void*)PHYSTOP); // 为所有运行freerange的CPU分配空闲的内存
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
  int id = cpuid();
  pop_off();

  acquire(&kmems[id].lock);
  // 将释放的空间插入当前CPU的空闲链表
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

  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmems[id].lock);
  r = kmems[id].freelist;
  if(r) //若有空页，则取出
    kmems[id].freelist = r->next;
  release(&kmems[id].lock);

  // 窃取其它链表的空闲区
  int i = 0;
  if(!r){
    for(i=0; i<NCPU; i++){
      if(i==id){
        continue;
      }
      acquire(&kmems[i].lock);
      r = kmems[i].freelist;
      if(r){ //若有空页，则取出
        kmems[i].freelist = r->next;
        release(&kmems[i].lock);
        break;
      } 
      release(&kmems[i].lock);
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
