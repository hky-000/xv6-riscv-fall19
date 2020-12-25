// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASH 13

struct {
  struct spinlock lock[HASH];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head[HASH];
} bcache;

void
binit(void)
{
  struct buf *b;
  int i;

  for(i=0; i<HASH; i++){
    initlock(&bcache.lock[i], "bcache");
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
    
  int num;
  // 分配buf
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    acquire(&bcache.lock[num]);
    num = NBUF % HASH;
    b->next = bcache.head[num].next; //头插法
    b->prev = &bcache.head[num];
    initsleeplock(&b->lock, "buffer");
    bcache.head[num].next->prev = b;
    bcache.head[num].next = b;
    release(&bcache.lock[num]);
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint _bhash = blockno % HASH;
  uint bhash = _bhash;

  acquire(&bcache.lock[_bhash]);
  // Is the block already cached?
  // 在块对应的hash链表里面找是否命中
  // 命中则返回块的位置，释放自旋锁，并获得睡眠锁
  for(b = bcache.head[_bhash].next; b != &bcache.head[_bhash]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[_bhash]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  // 没有命中，找当前链表是否有空闲位置
  for(b = bcache.head[_bhash].prev; b != &bcache.head[_bhash]; b = b->prev){
    if(b->refcnt == 0){
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[_bhash]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // 没有空闲位置，找其它链表是否有空闲位置，窃取过来
  for(int i=0; i<(HASH-1); i++){
    bhash = (bhash + 1) % HASH;
    acquire(&bcache.lock[bhash]);
    for(b = bcache.head[bhash].prev; b != &bcache.head[bhash]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->prev->next = b->next; //拿到b
        b->next->prev = b->prev;
        
        b->next = bcache.head[_bhash].next; 
        b->prev = &bcache.head[_bhash];
        b->next->prev = b;
        b->prev->next = b;
        release(&bcache.lock[bhash]);
        release(&bcache.lock[_bhash]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[bhash]);
  }
  release(&bcache.lock[_bhash]);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint bhash = b->blockno % HASH;

  acquire(&bcache.lock[bhash]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev; //拿出b
    b->prev->next = b->next;
    b->next = &bcache.head[bhash]; //将b放到head的前面
    b->prev = bcache.head[bhash].prev;
    bcache.head[bhash].prev->next = b;
    bcache.head[bhash].prev = b;
  }
  release(&bcache.lock[bhash]);
}

void
bpin(struct buf *b) {
  uint bhash = b->blockno % HASH;

  acquire(&bcache.lock[bhash]);
  b->refcnt++;
  release(&bcache.lock[bhash]);
}

void
bunpin(struct buf *b) {
  uint bhash = b->blockno % HASH;

  acquire(&bcache.lock[bhash]);
  b->refcnt--;
  release(&bcache.lock[bhash]);
}


