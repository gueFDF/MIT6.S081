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

#define BUCKET_NUM 13
#define BUF_NUM NBUF

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct
{
  struct spinlock lock_global;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;

  // 修改为哈希桶(开链法)
  struct buf buckets[BUCKET_NUM];
  struct spinlock lock[BUCKET_NUM];
} bcache;

// void
// binit(void)
// {
//   struct buf *b;

//   initlock(&bcache.lock, "bcache");

//   // Create linked list of buffers
//   bcache.head.prev = &bcache.head;
//   bcache.head.next = &bcache.head;
//   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
//     b->next = bcache.head.next;
//     b->prev = &bcache.head;
//     initsleeplock(&b->lock, "buffer");
//     bcache.head.next->prev = b;
//     bcache.head.next = b;
//   }
// }

int hash(int value)
{
  return value % 13;
}
//使用哈希桶的方法实现

// void binit(void)
// {
//   //为每一个桶维护一个锁
//   for (int i = 0; i < BUCKET_NUM; i++)
//   {
//     initlock(&bcache.lock[i], "bcache");
//   }
//   //初始化一把大锁
//   initlock(&bcache.lock_global, "bcache_global");
//   //初始化每一个桶的头结点

//   for (int i = 0; i < BUCKET_NUM; i++)
//   {
//     static struct buf head;
//     bcache.buckets[i] = &head;
//     bcache.buckets[i]->next = bcache.buckets[i];
//   }
//   //初始化buffer
//   for (int i = 0; i < BUF_NUM; i++)
//   {
//     int id = hash(i);
//     struct buf *now = &bcache.buf[i];
//     now->refcnt = 0;
//     now->blockno = id;     //不清楚是干嘛
//     now->timestamp = ticks; //初始化时间戳
//     now->next = bcache.buckets[id]->next;
//     bcache.buckets[id]->next = now;
//   }
// }
void binit(void)
{
  // 初始化锁
  for (int i = 0; i < BUCKET_NUM; ++i)
  {
    initlock(&bcache.lock[i], "bcache");
  }
  initlock(&bcache.lock_global, "bcache_global");

  // 初始化初始引用(初始化虚拟头结点)
  for (int i = 0; i < BUCKET_NUM; ++i)
  {
    bcache.buckets[i].next = &bcache.buckets[i];
    
  }

  // 初始化 buffer
  for (int i = 0; i < BUF_NUM; ++i)
  {
    int id = hash(i);
    struct buf *now = &bcache.buf[i];
    now->blockno = id;
    now->refcnt = 0;
    now->timestamp=ticks;
    // now->timestamp = ticks;
    initsleeplock(&now->lock, "buffer");
    // 插入
    now->next = bcache.buckets[id].next;
    bcache.buckets[id].next = now;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
// static struct buf*
// bget(uint dev, uint blockno)
// {
//   struct buf *b;

//   acquire(&bcache.lock);

//   // Is the block already cached?
//   for(b = bcache.head.next; b != &bcache.head; b = b->next){
//     if(b->dev == dev && b->blockno == blockno){
//       b->refcnt++;
//       release(&bcache.lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }

//   // Not cached.
//   // Recycle the least recently used (LRU) unused buffer.
//   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
//     if(b->refcnt == 0) {
//       b->dev = dev;
//       b->blockno = blockno;
//       b->valid = 0;
//       b->refcnt = 1;
//       release(&bcache.lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }
//   panic("bget: no buffers");
// }

//使用哈希桶的方法实现LRU
// static struct buf *
// bget(uint dev, uint blockno)
// {
//   struct buf *b;
//   int id = hash(blockno);
//   acquire(&bcache.bucketslock[id]);
//   // 1. 查找这个块是否被缓存, 如果被缓存直接返回即可
//   for (b = bcache.buckets[id]->next; b != bcache.buckets[id]; b = b->next)
//   {
//     if (b->dev == dev && b->blockno == blockno)
//     {
//       b->refcnt++;
//       release(&bcache.bucketslock[id]);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }
//   // 2.没有被缓存
//   uint timestamp_min = 0x8fffffff;
//   struct buf *res = 0;
//   for (b = bcache.buckets[id]->next; b != bcache.buckets[id]; b = b->next)
//   {
//     if (b->refcnt == 0 && b->timestap < timestamp_min)
//     {
//       timestamp_min = b->timestap;
//       res = b;
//     }
//   }

//   if (res) //在自己的桶里面找到了
//   {
//     res->blockno = blockno;
//     res->dev = dev;
//     res->refcnt = 1;
//     res->valid = 0;
//     res->timestap = ticks;
//     release(&bcache.bucketslock[id]);
//     acquiresleep(&res->lock);
//     return res;
//   }
//   //没有找到，在其他桶里面找
//   //因为要在整个哈希里面找，所以要上大锁
//   acquire(&bcache.lock_global);
//   release(&bcache.bucketslock[id]);

// flag:
//   timestamp_min = 0x8fffffff;
//   for (int i = 0; i < BUF_NUM; i++)
//   {
//     //因为要避免其他进程拿到这个桶的bloca，所以要加锁
//     int key = hash(bcache.buf[i].blockno);
//     acquire(&bcache.bucketslock[key]);
//     if (bcache.buf[i].refcnt == 0 && bcache.buf[i].timestap < timestamp_min)
//     {
//       timestamp_min = bcache.buf[i].timestap;
//       res = &bcache.buf[i];
//     }
//    release(&bcache.bucketslock[key]);
//   }
//   if (!res) //在其他桶也没有找到
//   {
//     release(&bcache.lock_global);
//     panic("bget: no buffers");
//   }
//   //找到了，需要从原桶删除，加入新桶
//   int key = hash(res->blockno);
//   acquire(&bcache.bucketslock[key]);
//   if (res->refcnt != 0)
//   {
//     printf("被其他线程抢占，出问题\n");
//     release(&bcache.bucketslock[key]);
//     goto flag;
//   }
//   for (b = bcache.buckets[key]; b->next != res; b = b->next)
//   {
//   }
//   //找到res的前一个节点
//   b->next = b->next->next; //删除

//   //加入新桶
//   acquire(&bcache.bucketslock[id]);
//   res->dev = dev;
//   res->blockno = blockno;
//   res->refcnt = 1;
//   res->valid = 0;
//   res->timestap = ticks;
//   res->next = bcache.buckets[id]->next;
//   bcache.buckets[id]->next = res;
//   release(&bcache.bucketslock[key]);
//   release(&bcache.bucketslock[id]);
//   release(&bcache.lock_global);
//   acquiresleep(&res->lock);
//   return res;
// }

static struct buf *bget(uint dev, uint blockno)
{
  struct buf *b;

  int id = hash(blockno);
  acquire(&bcache.lock[id]);

  // 1. 查找这个块是否被缓存, 如果被缓存直接返回即可
  for (b = bcache.buckets[id].next; b != &bcache.buckets[id]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 2 先在自己的桶里寻找
  uint timestamp_min = 0x8fffffff;
  struct buf *res = 0;
  for (b = bcache.buckets[id].next; b != &bcache.buckets[id]; b = b->next)
  {
    if (b->refcnt == 0 && b->timestamp < timestamp_min)
    {
      timestamp_min = b->timestamp;
      res = b;
    }
  }
  if (res)
  {
    res->dev = dev;
    res->blockno = blockno;
    res->valid = 0;
    res->refcnt = 1;
    release(&bcache.lock[id]);
    acquiresleep(&res->lock);
    return res;
  }

  // 3. 没有被缓存, 需要进行驱逐(其他桶中找)
  // 注意这里需要加全局锁
  // 不然可能出现两个进程同时找到同一个空块的情况
  acquire(&bcache.lock_global);
  release(&bcache.lock[id]);

  // (1) 找到最早的时间戳
  // 可以直接对 buf 循环
  timestamp_min = 0x8fffffff;
  res = 0;
  for (int i = 0; i < BUF_NUM; ++i)
  {
    b = &bcache.buf[i];
    int key=hash(b->blockno);
    acquire(&bcache.lock[key]);
    if (b->refcnt == 0 && b->timestamp < timestamp_min)
    {
      timestamp_min = b->timestamp;
      res = b;
    }
    release(&bcache.lock[key]);
  }

  // 如果没找到直接 panic
  if (!res)
  {
    release(&bcache.lock_global);
    panic("bget: no buffers");
  }

  int id_new = hash(res->blockno);

  // 需要移动块到目标哈希桶中
  // 从原桶中删除
  acquire(&bcache.lock[id_new]);
  for (b = &bcache.buckets[id_new]; b->next != &bcache.buckets[id_new]; b = b->next)
  {
    if (b->next == res)
    {
      b->next = res->next;
      break;
    }
  }
  acquire(&bcache.lock[id]);
  // 插入新桶
  res->next = bcache.buckets[id].next;
  bcache.buckets[id].next = res;
  res->dev = dev;
  res->blockno = blockno;
  res->valid = 0;
  res->refcnt = 1;
  release(&bcache.lock[id_new]); // 具体释放位置应该在哪里?
  release(&bcache.lock[id]);
  release(&bcache.lock_global);
  acquiresleep(&res->lock);
  return res;
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;

    //我们只需要改变时间戳
    b->timestamp = ticks;
  }

  release(&bcache.lock[id]);
}

void bpin(struct buf *b)
{
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void bunpin(struct buf *b)
{
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}
