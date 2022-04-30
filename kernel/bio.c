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
#define NBUP 17
struct {
  struct spinlock lock[NBUP];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUP];
} bcache;

void
binit(void)
{
  struct buf *b;

  //initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  //bcache.head.prev = &bcache.head;
  //bcache.head.next = &bcache.head;
  for(int i=0;i<NBUP;++i){
    initlock(&bcache.lock[i], "bcache");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int p = (b-bcache.buf) % NBUP;
    b->next = bcache.head[p].next;
    b->prev = &bcache.head[p];
    initsleeplock(&b->lock, "buffer");
    bcache.head[p].next->prev = b;
    bcache.head[p].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int p = (dev ^ blockno) % NBUP;
  //if (p<0)panic("hash p<0");
  acquire(&bcache.lock[p]);

  // Is the block already cached?
  for(b = bcache.head[p].next; b != &bcache.head[p]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[p]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  
  for(b = bcache.head[p].prev; b != &bcache.head[p]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[p]);
      acquiresleep(&b->lock);
      return b;
    }
  }
    
  for(int i=(p+1)%NBUP;i!=p;i=(i+1)%NBUP){
    acquire(&bcache.lock[i]);
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.lock[i]);
        b->next = bcache.head[p].next;
        b->prev = &bcache.head[p];
        bcache.head[p].next->prev = b;
        bcache.head[p].next = b;
        release(&bcache.lock[p]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
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
  if(!holdingsleep(&b->lock))
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

  releasesleep(&b->lock);
  int p = (b->blockno ^ b->dev) % NBUP;
  acquire(&bcache.lock[p]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[p].next;
    b->prev = &bcache.head[p];
    bcache.head[p].next->prev = b;
    bcache.head[p].next = b;
  }
  
  release(&bcache.lock[p]);
}

void
bpin(struct buf *b) {
  int p=(b->dev^b->blockno)%NBUP;
  acquire(&bcache.lock[p]);
  b->refcnt++;
  release(&bcache.lock[p]);
}

void
bunpin(struct buf *b) {
  int p=(b->dev^b->blockno)%NBUP;
  acquire(&bcache.lock[p]);
  b->refcnt--;
  release(&bcache.lock[p]);
}


