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

#define NBUCKETS 13   // 13个哈希组

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  //struct buf head;
  struct buf hashbucket[NBUCKETS]; //每个哈希队列一个linked list及一个lock
} bcache;

// struct {
//   struct spinlock lock;
//   struct buf buf[NBUF];

//   // Linked list of all buffers, through prev/next.
//   // head.next is most recently used.
//   struct buf head;
// } bcache;

void
binit(void)
{
  struct buf *b;
  // Create linked list of buffers
  for (int i = 0; i < NBUCKETS; i++){  
    initlock(&bcache.lock[i], "bcache");
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
      b->next = bcache.hashbucket[0].next;
      b->prev = &bcache.hashbucket[0];
      initsleeplock(&b->lock, "buffer");
      bcache.hashbucket[0].next->prev = b;
      bcache.hashbucket[0].next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint bucket = blockno % NBUCKETS;    // 分配块号对应的哈希表
  acquire(&bcache.lock[bucket]);

  // Is the block already cached?
  for(b = bcache.hashbucket[bucket].next; b != &bcache.hashbucket[bucket]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[bucket]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  for(int i = 0; i < NBUF; i++){
    b = &bcache.buf[i];
    int flag = 0;
    uint pbucket = b->blockno % NBUCKETS;
    if (pbucket != bucket){              // 该空闲块的blockno与当前blockno属于不同的哈希块才需要再锁一次
      flag = 1;
      acquire(&bcache.lock[pbucket]);
    }
    if(b->refcnt == 0) {                  // 找到一个空闲缓存块
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      // 将该缓存块从一个哈希块移出
      b->prev->next = b->next;
      b->next->prev = b->prev;
      if (flag) release(&bcache.lock[pbucket]);     

      b->next = bcache.hashbucket[bucket].next;
      b->prev = &bcache.hashbucket[bucket];
      b->next->prev = b;
      b->prev->next = b;
      release(&bcache.lock[bucket]);      // 将该空闲缓存块移动到bucket哈希块
      acquiresleep(&b->lock);
      return b;
    }
    if (flag) release(&bcache.lock[pbucket]);
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
  uint bucket = b->blockno % NBUCKETS;    // 块号对应的哈希表
  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[bucket].next;
    b->prev = &bcache.hashbucket[bucket];
    bcache.hashbucket[bucket].next->prev = b;
    bcache.hashbucket[bucket].next = b;
  }
  
  release(&bcache.lock[bucket]);
}

void
bpin(struct buf *b) {
  uint bucket = b->blockno % NBUCKETS;    // 块号对应的哈希表
  acquire(&bcache.lock[bucket]);
  b->refcnt++;
  release(&bcache.lock[bucket]);
}

void
bunpin(struct buf *b) {
  uint bucket = b->blockno % NBUCKETS;    // 块号对应的哈希表
  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  release(&bcache.lock[bucket]);
}


