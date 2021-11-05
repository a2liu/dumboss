#include "sync.h"
#include "basics.h"
#include "logging.h"
#include "memory.h"
#include <stdatomic.h>
#include <stddef.h>

// TODO: Add more macros for clang builtin as needed. Builtins list is here:
// https://releases.llvm.org/10.0.0/tools/clang/docs/LanguageExtensions.html
//                                      - Albert Liu, Nov 03, 2021 Wed 00:50 EDT
#define a_init(ptr_val, initial) __c11_atomic_init(ptr_val, initial)
#define a_load(obj)              __c11_atomic_load(obj, __ATOMIC_SEQ_CST)
#define a_store(obj, value)      __c11_atomic_store(obj, value, __ATOMIC_SEQ_CST)
#define a_cxweak(obj, expected, desired)                                                           \
  __c11_atomic_compare_exchange_weak(obj, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define a_cxstrong(obj, expected, desired)                                                         \
  __c11_atomic_compare_exchange_strong(obj, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define READ_MUTEX  U32(1 << 0)
#define WRITE_MUTEX U32(1 << 1)

// TODO: overflow might happen in certain places idk
//                                      - Albert Liu, Nov 03, 2021 Wed 00:51 EDT

// should we be using u64 or s64 here? u64 has the semantic
// meaning that all values will be positive, which we want, but makes it harder
// to subtract numbers correctly, and has required wrapping behavior, which we
// might not want.
struct Queue {
  // eventually we should add a `next` buffer that points to the queue to be
  // used for writing. Also, probably should add a 32-bit `ref_count` field for
  // when multiple people have a reference to the same queue.
  _Atomic(s64) read_head;  // first element index that's safe to read from
  _Atomic(s64) write_head; // first element index that's safe to write to
  _Atomic(u32) flags;      // Flags
  const s32 elem_size;     // having a larger elem_size doesn't make sense.
  const s64 count;         // size in elements
  s64 _unused0;
  s64 _unused1;
  s64 _unused2;
  s64 _unused3;

  // This is the beginning of the second cache line
  u8 data[];
};

_Static_assert(sizeof(_Atomic(s64)) == 8, "atomics are zero-cost right?? :)");
_Static_assert(sizeof(Queue) == 64, "Queue should have 64 byte control structure");

Queue *Queue__create(const Buffer buffer, const s32 elem_size) {
  const u64 address = (u64)buffer.data;
  if (address != align_down(address, 64)) return NULL; // align to cache line

  s64 count = buffer.count - (s64)sizeof(Queue);
  count -= count % elem_size;
  if (count <= 0 || elem_size == 0) return NULL;

  Queue *queue = (Queue *)buffer.data;
  a_init(&queue->read_head, 0);
  a_init(&queue->write_head, 0);
  a_init(&queue->flags, 0);
  *((s32 *)&queue->elem_size) = elem_size;
  *((s64 *)&queue->count) = count;
  queue->_unused0 = 0x1eadbeef1eadbeef;
  queue->_unused1 = 0x1eadbeef1eadbeef;
  queue->_unused2 = 0x1eadbeef1eadbeef;
  queue->_unused3 = 0x1eadbeef1eadbeef;

  return queue;
}

s64 Queue__enqueue(Queue *queue, const void *buffer, s64 count, s32 elem_size) {
  assert(elem_size == queue->elem_size);
  if (count == 0) return 0;

  NAMED_BREAK(set_mutex) {
    u32 flags = a_load(&queue->flags) & ~WRITE_MUTEX;
    REPEAT(5) {
      if (a_cxweak(&queue->flags, &flags, flags | WRITE_MUTEX)) break(set_mutex);
      flags &= ~WRITE_MUTEX;
    }

    return -1;
  }

  const s64 queue_size = queue->count;
  s64 read_head = a_load(&queue->read_head), write_head = a_load(&queue->write_head);
  s64 new_head = min(write_head + count, read_head + queue_size);
  s64 write_count = new_head - write_head;

  memcpy(&queue->data[(write_head % queue_size) * elem_size], buffer, write_count * elem_size);
  a_store(&queue->write_head, new_head);

  u32 flags = a_load(&queue->flags);
  while (a_cxweak(&queue->flags, &flags, flags & ~WRITE_MUTEX))
    ;

  return write_count;
}

s64 Queue__dequeue(Queue *queue, void *buffer, s64 count, s32 elem_size) {
  assert(elem_size == queue->elem_size);
  if (count == 0) return 0;

  NAMED_BREAK(set_mutex) {
    u32 flags = a_load(&queue->flags) & (~READ_MUTEX);
    REPEAT(5) {
      if (a_cxweak(&queue->flags, &flags, flags | READ_MUTEX)) break(set_mutex);
      flags &= (~READ_MUTEX);
    }

    return -1;
  }

  const s64 queue_size = queue->count;
  s64 read_head = a_load(&queue->read_head), write_head = a_load(&queue->write_head);
  s64 new_head = min(read_head + count, write_head);
  s64 read_count = new_head - read_head;

  memcpy(buffer, &queue->data[(read_head % queue_size) * elem_size], read_count * elem_size);
  a_store(&queue->read_head, new_head);

  u32 flags = a_load(&queue->flags);
  while (a_cxweak(&queue->flags, &flags, flags & ~READ_MUTEX))
    ;

  return read_count;
}

s64 Queue__len(const Queue *queue, s32 elem_size) {
  assert(elem_size == queue->elem_size);

  s64 read_head = queue->read_head;
  s64 write_head = queue->write_head;

  return (write_head - read_head) / elem_size;
}

s64 Queue__capacity(const Queue *queue, s32 elem_size) {
  assert(elem_size == queue->elem_size);

  return queue->count / elem_size;
}
