#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* 한국어로 풀어보면:
 * 각 스레드는 자기 전용 4KB 페이지 하나를 가진다.
 * 그 페이지 맨 아래에는 `struct thread`가 놓이고,
 * 맨 위쪽에서는 커널 스택이 아래 방향으로 자란다.
 *
 * 여기서 스택은 "함수가 일할 때 잠깐 쓰는 작업 공간"이라고 생각하면 된다.
 * 함수가 끝나면 어디로 돌아갈지, 지역 변수, 잠깐 저장한 값들이 여기에 놓인다.
 * 커널 스택은 그중에서도 "커널 코드가 실행될 때 쓰는 스택"이다.
 *
 * 왜 스레드마다 커널 스택이 따로 필요하냐면,
 * 각 스레드는 자기만의 함수 호출 흐름과 자기만의 지역 변수 상태를 가져야 하기 때문이다.
 * 스레드 A가 함수 중간까지 실행하다 멈추고, 나중에 다시 이어서 실행되려면
 * A가 쓰던 스택 내용이 그대로 남아 있어야 한다.
 * 그래서 스레드마다 자기 커널 스택을 따로 가진다.
 *
 * 즉 `struct thread`와 커널 스택이 같은 4KB 공간을 나눠 쓰는 구조다.
 * 그래서 둘 중 하나가 너무 커지면 다른 쪽 공간을 침범하게 된다.
 *
 * 여기서 중요한 점은 두 가지다.
 *
 * 1. `struct thread`를 너무 크게 만들면 안 된다.
 *    구조체가 커질수록 커널 스택이 사용할 공간이 줄어든다.
 *    그래서 새 필드를 추가할 때도 "정말 필요한가?"를 생각해야 한다.
 *
 * 2. 커널 함수 안에서 큰 지역 배열이나 큰 구조체를 스택에 올리면 안 된다.
 *    커널 스택은 넓지 않기 때문에, 지역 변수를 크게 잡으면 스택 오버플로우가 나기 쉽다.
 *    큰 데이터가 필요하면 `malloc()`이나 `palloc_get_page()` 같은 동적 할당을 쓰라는 뜻이다.
 *
 * 아주 단순하게 보면, 스레드 하나당 4KB짜리 방 하나가 있고
 * 방 바닥 쪽에는 `struct thread`가,
 * 천장 쪽에는 커널 스택이 있는 셈이다.
 * 바닥 짐을 너무 많이 늘리거나 천장에서 내려오는 스택을 너무 많이 쓰면
 * 둘이 부딪혀서 망가진다.
 *
 * 스택 오버플로우가 나면 보통 아래쪽의 `struct thread` 영역을 덮어쓰게 되고,
 * 특히 `magic` 값이 망가져서 `thread_current()`의 assertion failure로 먼저 드러날 가능성이 크다.
 * 그래서 `magic`은 "이 스레드 구조체가 아직 정상인가?"를 검사하는 안전장치 역할을 한다. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
/* 한국어로 풀어보면:
 * `elem`은 스레드를 리스트에 연결할 때 쓰는 연결 부품이다.
 *
 * 이 `elem` 하나를 두 군데에서 재사용할 수 있는데,
 * 그 이유는 READY 상태의 스레드와 BLOCKED 상태의 스레드가
 * 동시에 같은 `elem`을 서로 다른 리스트에 넣는 일이 없기 때문이다.
 *
 * - READY 상태면 run queue에 들어간다.
 * - BLOCKED 상태면 semaphore wait list에 들어간다.
 *
 * 즉 한 순간에 스레드는 둘 중 하나의 줄에만 서 있으므로,
 * `elem` 하나만 있어도 리스트 연결용으로 충분하다는 뜻이다. */
struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	int64_t	wakeup_tick;
	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. */

	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */

#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_sleep (int64_t wakeup_tick);
void thread_awake (int64_t current_tick);
void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

#endif /* threads/thread.h */
