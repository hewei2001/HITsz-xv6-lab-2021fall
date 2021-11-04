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

// 总共 NBUF 有 30 个，这里设置素数 13，使每个 hashbucket 不会过长 
#define NBUCKET 13

//uint extern ticks; // 外部函数，求时间戳

struct {
  struct spinlock lock[NBUCKET]; // 每个队列一个 lock
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head; // 用双向链表实现 LRU 机制
  struct buf hashbucket[NBUCKET]; //每个哈希队列是一个单向链表，同样有 LRU
} bcache;

// 哈希运算，不考虑冲突（桶无上限）
int
bhash(int blockno){
  int res = blockno % NBUCKET;
  return res;
}

void
binit(void)
{
  struct buf *b;

  // initlock(&bcache.lock, "bcache");

  // 初始化锁
  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.lock[i], "bcache.bucket");
    // 初始桶中的元素作为 head 指向空（无元素）
    bcache.hashbucket[i].next = 0;
  }

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // initsleeplock(&b->lock, "buffer");
    // bcache.head.next->prev = b;
    // bcache.head.next = b;

    b->next = bcache.hashbucket[0].next;
    initsleeplock(&b->lock, "buffer"); // 为 buf[NBUF] 里每个申请睡眠锁
    bcache.hashbucket[0].next = b;
  }
}

// 下面的 bget 涉及替换操作，这里封装一下
void
replaceBuffer(struct buf *LRUbuf, uint dev, uint blockno, uint timestamp){
  LRUbuf->dev = dev;
  LRUbuf->blockno = blockno;
  LRUbuf->valid = 0;
  LRUbuf->refcnt = 1;
  LRUbuf->timestamp = timestamp;
}

int
findBuffer(struct buf **LRUbuf, int next_h)
{
  int min_time = 0x3f3f3f3f;
  int flag = 0;
  for(struct buf *b = bcache.hashbucket[next_h].next; b != 0; b = b->next){
    if(b->refcnt == 0 && b->timestamp < min_time) {
      flag = 1; // 标记有没有找到
      *LRUbuf = b;
      min_time = b->timestamp; 
    }
  }
  return flag;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int h = bhash(blockno); // 在对应的桶号中找
  acquire(&bcache.lock[h]);

  // Is the block already cached?
  // 结束条件变成 b 为 NULL
  for(b = bcache.hashbucket[h].next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){ // buf 中已有 block
      b->refcnt++;
      release(&bcache.lock[h]);
      acquiresleep(&b->lock); // 获取睡眠锁，参见 brelse
      return b;
    }
  }
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) { // 该 block 没有正在被引用
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  // 如果没找到，则一定是没缓存（因为前面 hashbucket 是无冲突的）
  // 要到其他桶中找 LRU块 替换，并移到对应的 hashbucket
  int next_h = (h + 1) % NBUCKET;
  struct buf *LRUbuf = 0;
  // 从当前桶的下一个开始遍历，直到遍历一圈
  while(next_h != h){
    acquire(&bcache.lock[next_h]);// 获取当前bucket的锁
    // 如果在该桶中找到了 LRU 对象，直接换
    if(findBuffer(&LRUbuf, next_h)){
      replaceBuffer(LRUbuf, dev, blockno, ticks);
      struct buf *pre = &bcache.hashbucket[next_h];

      // 在单向链表中找到要断开的点
      while (pre->next != LRUbuf) pre = pre->next;
      pre->next = LRUbuf->next;
      release(&bcache.lock[next_h]);

      // 将抢夺的 block 放入自己的桶
      LRUbuf->next = bcache.hashbucket[h].next;
      bcache.hashbucket[h].next = LRUbuf;
      release(&bcache.lock[h]);
      acquiresleep(&LRUbuf->lock);
      return LRUbuf;
    }
    release(&bcache.lock[next_h]);
    next_h = (next_h + 1) % NBUCKET;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock)) // 写需要持有睡眠锁，但读不需要
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock); // 释放睡眠锁，参见 bget

  // 改用时间戳，则释放 buf 时不需要再把 buf 插到 head
  // 因此就不需要遍历链表，就不需要获取 bcache.lock 啦
  // acquire(&bcache.lock);
  // b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  // release(&bcache.lock);

  int h = bhash(b->blockno);

  acquire(&bcache.lock[h]);
  b->refcnt--;
  if(b->refcnt == 0)
    b->timestamp = ticks;     // 如果不再引用，就标记时间戳为最新
  release(&bcache.lock[h]);
}

void
bpin(struct buf *b) {
  int h = bhash(b->blockno); // 在对应的桶号中找
  acquire(&bcache.lock[h]);
  b->refcnt++;
  release(&bcache.lock[h]);
}

void
bunpin(struct buf *b) {
  int h = bhash(b->blockno); // 在对应的桶号中找
  acquire(&bcache.lock[h]);
  b->refcnt--;
  release(&bcache.lock[h]);
}


