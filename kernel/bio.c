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

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hashbucket[NBUCKETS];
} bcache;

static int
bucket_of(uint blockno){
  return  blockno % NBUCKETS;
}

static char lockname[NBUCKETS][16];

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i<NBUCKETS;i++){
    snprintf(lockname[i], sizeof(lockname[i]), "bcache.bucket%d", i);
    initlock(&bcache.lock[i], lockname[i]);

    // Create linked list of buffers
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

  int bid = bucket_of(blockno);

  acquire(&bcache.lock[bid]);

  // Is the block already cached?
  for(b = bcache.hashbucket[bid].next; b != &bcache.hashbucket[bid]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[bid]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  struct buf *victim=0;

  for(b = bcache.hashbucket[bid].next; b != &bcache.hashbucket[bid]; b = b->next){
    if(b->refcnt == 0) {
      victim = b;
      b->prev->next=b->next;
      b->next->prev=b->prev;
      break;
    }
  }
  if(victim == 0){
    for(int i = 0; i < NBUCKETS; i++){
      if(i == bid){
        continue;
      }else{
        release(&bcache.lock[bid]);
        acquire(&bcache.lock[i]);
        for(b = bcache.hashbucket[i].next; b != &bcache.hashbucket[i]; b = b->next){
          if(b->refcnt == 0) {
            victim = b;
            b->prev->next=b->next;
            b->next->prev=b->prev;
            break;
          }
        }
        release(&bcache.lock[i]);
        acquire(&bcache.lock[bid]);

        for(b = bcache.hashbucket[bid].next; b != &bcache.hashbucket[bid]; b = b->next){
          if(b->dev == dev && b->blockno == blockno){
            if(victim){
              victim->next = bcache.hashbucket[bid].next;   // 把借来的 victim 挂回本桶当空闲
              victim->prev = &bcache.hashbucket[bid];
              bcache.hashbucket[bid].next->prev = victim;
              bcache.hashbucket[bid].next = victim;
              victim = 0;
            }
            b->refcnt++;
            release(&bcache.lock[bid]);
            acquiresleep(&b->lock);
            return b;
          }
        }

        if(victim)break;
      }
    }
  }
  if(victim == 0)panic("bget: no buffers");

  victim->next = bcache.hashbucket[bid].next;
  victim->prev = &bcache.hashbucket[bid];
  bcache.hashbucket[bid].next->prev = victim;
  bcache.hashbucket[bid].next = victim;

  victim->dev = dev;
  victim->blockno = blockno;
  victim->valid = 0;
  victim->refcnt = 1;
  release(&bcache.lock[bid]);
  acquiresleep(&victim->lock);
  return victim;
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

  int bid =bucket_of(b->blockno);

  acquire(&bcache.lock[bid]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[bid].next;
    b->prev = &bcache.hashbucket[bid];
    bcache.hashbucket[bid].next->prev = b;
    bcache.hashbucket[bid].next = b;
  }

  release(&bcache.lock[bid]);
}

void
bpin(struct buf *b) {
  int bid = bucket_of(b->blockno);
  acquire(&bcache.lock[bid]);
  b->refcnt++;
  release(&bcache.lock[bid]);
}

void
bunpin(struct buf *b) {
  int bid = bucket_of(b->blockno);
  acquire(&bcache.lock[bid]);
  b->refcnt--;
  release(&bcache.lock[bid]);
}


