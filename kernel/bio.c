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

#define NBUCKETS 13

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  struct buf bucket[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;
  int i;
  for (i=0; i<NBUCKETS; ++i)
  {
    initlock(&bcache.lock[i], "bcache");
    // Create linked list of buffers
    bcache.bucket[i].prev = &bcache.bucket[i];
    bcache.bucket[i].next = &bcache.bucket[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.bucket[0].next;
    b->prev = &bcache.bucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.bucket[0].next->prev = b;
    bcache.bucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucketno;
  int i;

  bucketno = blockno % NBUCKETS;

  acquire(&bcache.lock[bucketno]);

  // Is the block already cached?
  for(b = bcache.bucket[bucketno].next; b != &bcache.bucket[bucketno]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[bucketno]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  // If found space in current bucket
  for(b = bcache.bucket[bucketno].prev; b != &bcache.bucket[bucketno]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[bucketno]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[bucketno]); // Release current lock in time to reduce contention
  // If not found space in current bucket, steal a block from other buckets
  if (b == &bcache.bucket[bucketno])
  {
    for (i=0; i<NBUCKETS; ++i)
    {
      if (i != bucketno)
      {
        acquire(&bcache.lock[i]);
        for(b = bcache.bucket[i].prev; b != &bcache.bucket[i]; b = b->prev){
          if(b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            // Steal a block from bucket[i]
            b->prev->next = b->next;
            b->next->prev = b->prev;
            // Insert this buffer to current bucket's linked list
            acquire(&bcache.lock[bucketno]);
            b->next = bcache.bucket[bucketno].next;
            b->prev = &bcache.bucket[bucketno];
            bcache.bucket[bucketno].next->prev = b;
            bcache.bucket[bucketno].next = b;
            release(&bcache.lock[bucketno]);
            release(&bcache.lock[i]);
            acquiresleep(&b->lock);
            return b;
          }
        }
        // Buffer not found in bucket i
        if (b == &bcache.bucket[i])
          release(&bcache.lock[i]);
      }
    }
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
  int bucketno;

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  bucketno = b->blockno % NBUCKETS;

  acquire(&bcache.lock[bucketno]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.bucket[bucketno].next;
    b->prev = &bcache.bucket[bucketno];
    bcache.bucket[bucketno].next->prev = b;
    bcache.bucket[bucketno].next = b;
  }
  
  release(&bcache.lock[bucketno]);
}

void
bpin(struct buf *b) {
  int bucketno;
  bucketno = b->blockno % NBUCKETS;
  acquire(&bcache.lock[bucketno]);
  b->refcnt++;
  release(&bcache.lock[bucketno]);
}

void
bunpin(struct buf *b) {
  int bucketno;
  bucketno = b->blockno % NBUCKETS;
  acquire(&bcache.lock[bucketno]);
  b->refcnt--;
  release(&bcache.lock[bucketno]);
}


