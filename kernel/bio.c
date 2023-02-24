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

struct {
  struct spinlock lock[HASHPRIME];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[HASHPRIME];
} bcache;

void
binit(void)
{
  struct buf *b;

  uint i = 0;
  for(i = 0; i < HASHPRIME; i++)
  {
    initlock(&bcache.lock[i], "bhash");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  i = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++, i++){
    if(i == HASHPRIME) i = 0;
    b->next = bcache.head[i].next;
    b->prev = &bcache.head[i];
    bcache.head[i].next->prev = b;
    bcache.head[i].next = b;
    // Create linked list of buffers
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // Is the block already cached?
  uint idx = blockno % HASHPRIME;
  acquire(&bcache.lock[idx]);
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Not cached in idx'th hash table.
  // Recycle the other least recently used (LRU) unused buffer in idx'th hash table.
  // in this case, we keep the hold of the lock of idx'th hash table
  // and try to replace a buffer
  for(b = bcache.head[idx].prev; b != &bcache.head[idx]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[idx]);
  // we fail to find a replaceable buffer in idx'th hash table
  // we try to replace a buffer in other hash table
  for(int i = 0; i < HASHPRIME; i++)
  {
    if(i == idx) continue;
    if(i < idx)
    {
      acquire(&bcache.lock[i]);
      acquire(&bcache.lock[idx]);
    }
    else
    {
      acquire(&bcache.lock[idx]);
      acquire(&bcache.lock[i]);
    }
    // we need to hold both the locks of the i'th table and the idx'th table
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        // we find a replaceable buffer, now we remove it from i'th table and insert it into idx'th table
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev = b->prev;
        b->prev->next = b->next;
        // we now insert the buffer into the idx'th table
        b->next = bcache.head[idx].next;
        b->prev = &bcache.head[idx];
        bcache.head[idx].next->prev= b;
        bcache.head[idx].next = b;
        // we then return with bcache.lock[idx] released
        release(&bcache.lock[i]);
        release(&bcache.lock[idx]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
    release(&bcache.lock[idx]);
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

  uint idx = b->blockno % HASHPRIME;
  acquire(&bcache.lock[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[idx].next;
    b->prev = &bcache.head[idx];
    bcache.head[idx].next->prev = b;
    bcache.head[idx].next = b;
  }
  release(&bcache.lock[idx]);
}

void
bpin(struct buf *b) {
  uint idx = b->blockno % HASHPRIME;
  acquire(&bcache.lock[idx]);
  b->refcnt++;
  release(&bcache.lock[idx]);
}

void
bunpin(struct buf *b) {
  uint idx = b->blockno % HASHPRIME;
  acquire(&bcache.lock[idx]);
  b->refcnt--;
  release(&bcache.lock[idx]);
}