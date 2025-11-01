/* fifo.h — interrupt-safe ring buffer for embedded systems
 *
 * Usage:
 *   #define FIFO_USE_BASEPRI  // (optional) on Cortex-M; otherwise PRIMASK is
 * used #include "fifo.h"
 *
 *   #define CAP 128
 *   static uint8_t rx_storage[CAP];
 *   static fifo_t rx;
 *   void init(void) {
 *     fifo_init(&rx, rx_storage, CAP);   // CAP may be any >= 2
 *   }
 *
 *   // ISR (producer)
 *   void USARTx_IRQHandler(void) {
 *     uint8_t b = USARTx->DR;
 *     (void)fifo_push(&rx, b);           // drop if full (returns false)
 *   }
 *
 *   // Foreground (consumer)
 *   int read_byte(void) {
 *     uint8_t b;
 *     return fifo_pop(&rx, &b) ? b : -1; // -1 if empty
 *   }
 */

#ifndef FIFO_H_
#define FIFO_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* --------- Memory barrier (visibility between IRQ contexts) ---------- */
#if defined(__GNUC__) || defined(__clang__)
#define FIFO_DMB() __asm__ __volatile__("dmb ish" ::: "memory")
#else
#define FIFO_DMB()                                                             \
  do { /* platform-specific barrier */                                         \
  } while (0)
#endif

/* --------- Critical section primitives (configurable) -----------------
 * Choose one:
 *   - FIFO_USE_BASEPRI: mask only <= BASEPRI interrupts (Cortex-M3+)
 *   - default: PRIMASK (disable all maskable interrupts)
 * Or define your own FIFO_CS_ENTER/EXIT before including this header.
 */
#if !defined(FIFO_CS_ENTER) || !defined(FIFO_CS_EXIT)

#if defined(FIFO_USE_BASEPRI)
/* BASEPRI critical section (keeps higher-priority IRQs running).
 * Define FIFO_BASEPRI_LEVEL to your system's desired mask level (0-255, shifted
 * by 3 in HW). Example: 0x40 masks priorities numerically >= 0x40 (lower
 * urgency).
 */
#ifndef FIFO_BASEPRI_LEVEL
#define FIFO_BASEPRI_LEVEL 0x40
#endif

static inline uint32_t fifo_cs_enter_(void) {
  uint32_t old;
  __asm__ volatile("mrs %0, basepri\n"
                   "msr basepri, %1\n"
                   : "=r"(old)
                   : "r"(FIFO_BASEPRI_LEVEL)
                   : "memory");
  FIFO_DMB();
  return old;
}
static inline void fifo_cs_exit_(uint32_t old) {
  FIFO_DMB();
  __asm__ volatile("msr basepri, %0" ::"r"(old) : "memory");
}
#define FIFO_CS_STATE uint32_t __fifo_cs_token
#define FIFO_CS_ENTER() (__fifo_cs_token = fifo_cs_enter_())
#define FIFO_CS_EXIT() (fifo_cs_exit_(__fifo_cs_token))

#else
/* PRIMASK critical section (globally disable/enable maskable IRQs). */
static inline uint32_t fifo_cs_enter_(void) {
  uint32_t primask;
  __asm__ volatile("mrs %0, primask\n"
                   "cpsid i\n"
                   : "=r"(primask)::"memory");
  FIFO_DMB();
  return primask;
}
static inline void fifo_cs_exit_(uint32_t primask) {
  FIFO_DMB();
  __asm__ volatile("msr primask, %0" ::"r"(primask) : "memory");
}
#define FIFO_CS_STATE uint32_t __fifo_cs_token
#define FIFO_CS_ENTER() (__fifo_cs_token = fifo_cs_enter_())
#define FIFO_CS_EXIT() (fifo_cs_exit_(__fifo_cs_token))
#endif

#endif /* !defined(FIFO_CS_ENTER) || !defined(FIFO_CS_EXIT) */

/* ----------------------- FIFO structure ------------------------------ */
typedef struct {
  uint8_t *buf;         /* external storage */
  size_t cap;           /* capacity (number of bytes) */
  volatile size_t head; /* next write index */
  volatile size_t tail; /* next read index  */
  /* Optional: set mask = cap-1 for power-of-two fast wrap; else 0 */
  size_t mask;
  void (*error_callback)();
} fifo_t;

/* ----------------------- Helpers ------------------------------------- */
static inline bool is_pow2_(size_t x) { return (x & (x - 1u)) == 0u; }
static inline size_t wrap_(size_t idx, size_t cap, size_t mask) {
  return mask ? (idx & mask) : (idx >= cap ? idx - cap : idx);
}

/* ----------------------- API ----------------------------------------- */
static inline void fifo_init(fifo_t *f, uint8_t *storage, size_t capacity,
                             void (*error_callback)()) {
  f->buf = storage;
  f->cap = capacity;
  f->head = 0u;
  f->tail = 0u;
  f->mask = is_pow2_(capacity) ? (capacity - 1u) : 0u;
  f->error_callback = error_callback;
}

static inline void fifo_reset(fifo_t *f) {
  FIFO_CS_STATE;
  FIFO_CS_ENTER();
  f->head = f->tail = 0u;
  FIFO_CS_EXIT();
}

static inline size_t fifo_capacity(const fifo_t *f) { return f->cap; }

/* Size must be computed atomically w.r.t. concurrent head/tail updates. */
static inline size_t fifo_size(const fifo_t *f) {
  size_t h, t;
  FIFO_CS_STATE;
  FIFO_CS_ENTER();
  h = f->head;
  t = f->tail;
  FIFO_CS_EXIT();
  return (h >= t) ? (h - t) : (f->cap - (t - h));
}

static inline bool fifo_is_empty(const fifo_t *f) {
  bool empty;
  FIFO_CS_STATE;
  FIFO_CS_ENTER();
  empty = (f->head == f->tail);
  FIFO_CS_EXIT();
  return empty;
}

static inline bool fifo_is_full(const fifo_t *f) {
  bool full;
  FIFO_CS_STATE;
  FIFO_CS_ENTER();
  size_t next = wrap_(f->head + 1u, f->cap, f->mask);
  full = (next == f->tail);
  FIFO_CS_EXIT();
  return full;
}

/* Push a single byte. Returns false if full (byte not written). */
static inline bool fifo_push(fifo_t *f, uint8_t byte) {
#if defined(FIFO_SPSC)
  size_t next = wrap_(f->head + 1u, f->cap, f->mask);
  if (next == f->tail)
    return false; /* full */
  f->buf[f->head] = byte;
  FIFO_DMB(); /* ensure data visible before head move */
  f->head = next;
  return true;
#else
  bool ok = false;
  FIFO_CS_STATE;
  FIFO_CS_ENTER();
  size_t next = wrap_(f->head + 1u, f->cap, f->mask);
  if (next != f->tail) {
    f->buf[f->head] = byte;
    FIFO_DMB();
    f->head = next;
    ok = true;
  } else {
    f->error_callback();
  }
  FIFO_CS_EXIT();
  return ok;
#endif
}

/* Pop a single byte. Returns false if empty. */
static inline bool fifo_pop(fifo_t *f, uint8_t *out) {
#if defined(FIFO_SPSC)
  if (f->head == f->tail)
    return false; /* empty */
  *out = f->buf[f->tail];
  FIFO_DMB(); /* ensure data read before tail move */
  f->tail = wrap_(f->tail + 1u, f->cap, f->mask);
  return true;
#else
  bool ok = false;
  FIFO_CS_STATE;
  FIFO_CS_ENTER();
  if (f->head != f->tail) {
    *out = f->buf[f->tail];
    FIFO_DMB();
    f->tail = wrap_(f->tail + 1u, f->cap, f->mask);
    ok = true;
  }
  FIFO_CS_EXIT();
  return ok;
#endif
}

/* Write up to len bytes; returns number actually written. */
static inline size_t fifo_write(fifo_t *f, const uint8_t *src, size_t len) {
  size_t n = 0;
  while (n < len) {
    /* Optionally batch inside one CS for better throughput. */
    FIFO_CS_STATE;
    FIFO_CS_ENTER();
    size_t head = f->head, tail = f->tail, cap = f->cap, mask = f->mask;
    size_t space =
        (tail > head) ? (tail - head - 1u) : (cap - (head - tail) - 1u);
    size_t chunk = (len - n < space) ? (len - n) : space;
    /* First contiguous segment to end of buffer */
    size_t to_end = mask ? ((cap - (head & mask)) & mask) + 1u : (cap - head);
    if (chunk > to_end)
      chunk = to_end;
    for (size_t i = 0; i < chunk; ++i)
      f->buf[wrap_(head + i, cap, mask)] = src[n + i];
    FIFO_DMB();
    f->head = wrap_(head + chunk, cap, mask);
    FIFO_CS_EXIT();

    if (chunk == 0) {
      f->error_callback();
      break; /* full */
    }
    n += chunk;
  }
  return n;
}

/* Read up to len bytes; returns number actually read. */
static inline size_t fifo_read(fifo_t *f, uint8_t *dst, size_t len) {
  size_t n = 0;
  while (n < len) {
    FIFO_CS_STATE;
    FIFO_CS_ENTER();
    size_t head = f->head, tail = f->tail, cap = f->cap, mask = f->mask;
    size_t avail = (head >= tail) ? (head - tail) : (cap - (tail - head));
    size_t chunk = (len - n < avail) ? (len - n) : avail;
    size_t to_end = mask ? ((cap - (tail & mask)) & mask) + 1u : (cap - tail);
    if (chunk > to_end)
      chunk = to_end;
    for (size_t i = 0; i < chunk; ++i)
      dst[n + i] = f->buf[wrap_(tail + i, cap, mask)];
    FIFO_DMB();
    f->tail = wrap_(tail + chunk, cap, mask);
    FIFO_CS_EXIT();

    if (chunk == 0)
      break; /* empty */
    n += chunk;
  }
  return n;
}

#endif /* FIFO_H_ */
