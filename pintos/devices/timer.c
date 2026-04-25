#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "threads/interrupt.h"
#include "threads/io.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* 8254 타이머 칩의 하드웨어 세부 사항은 [8254]를 참고하라. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* OS가 부팅된 이후 경과한 타이머 틱 수. */
static int64_t ticks;
// 지금 자고 있는 스레드들을 담는 리스트
static struct list sleep_list;

/* 타이머 틱당 루프 반복 횟수.
   timer_calibrate()에서 초기화된다. */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static bool wake_tick_less(const struct list_elem *a, const struct list_elem *b, void *aux);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);

/* 8254 프로그래머블 인터벌 타이머(PIT)가
   초당 PIT_FREQ번 인터럽트를 발생시키도록 설정하고,
   해당 인터럽트를 등록한다. */
void
timer_init (void) {
	list_init(&sleep_list);	// sleep_list 초기화
	/* 8254 입력 주파수를 TIMER_FREQ로 나눈 값을
	   반올림하여 사용한다.
	   타이머가 몇 번마다 인터럽트를 발생시킬지 계산 */
	uint16_t count = (1193180 + TIMER_FREQ / 2) / TIMER_FREQ;	// 기존 PIT 타이머 설정 

	outb (0x43, 0x34);    /* CW: 카운터 0, LSB 다음 MSB, 모드 2, 바이너리. */
	outb (0x40, count & 0xff);
	outb (0x40, count >> 8);

	/* 타이머 인터럽트가 오면 timer_interrupt()를 실행하라”라고 등록 */
	intr_register_ext (0x20, timer_interrupt, "8254 Timer");	
}

/* 짧은 지연을 구현하는 데 쓰이는 loops_per_tick을 보정한다. */
void
timer_calibrate (void) {
	unsigned high_bit, test_bit;

	ASSERT (intr_get_level () == INTR_ON);
	printf ("Calibrating timer...  ");

	/* loops_per_tick을 2의 거듭제곱 값 중
	   한 타이머 틱보다 여전히 작은 가장 큰 값으로 근사한다. */
	loops_per_tick = 1u << 10;
	while (!too_many_loops (loops_per_tick << 1)) {
		loops_per_tick <<= 1;
		ASSERT (loops_per_tick != 0);
	}

	/* loops_per_tick의 다음 8비트를 보정한다. */
	high_bit = loops_per_tick;
	for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
		if (!too_many_loops (high_bit | test_bit))
			loops_per_tick |= test_bit;

	printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* OS가 부팅된 이후 경과한 타이머 틱 수를 반환한다. */
int64_t
timer_ticks (void) {
	enum intr_level old_level = intr_disable ();
	int64_t t = ticks;
	intr_set_level (old_level);
	barrier ();
	return t;
}

/* THEN 이후 경과한 타이머 틱 수를 반환한다.
   THEN은 이전에 timer_ticks()가 반환한 값이어야 한다. */
int64_t
timer_elapsed (int64_t then) {
	return timer_ticks () - then;
}

/* 약 TICKS 타이머 틱 동안 현재 스레드의 실행을 멈춘다. */
void timer_sleep(int64_t ticks)
{
	// sleep할 시간이 0 이하이면 재울 필요가 없으므로 바로 반환한다.
	if (ticks <= 0)
		return;

	// sleep_list 조작 중 timer_interrupt가 끼어들지 못하게 인터럽트를 끄고,
	// 나중에 원래 상태로 복구하기 위해 이전 인터럽트 상태를 저장한다.
	enum intr_level old_level = intr_disable();

	// timer_sleep을 호출한 현재 실행 중인 스레드를 가져온다.
	struct thread *cur = thread_current();

	// 현재 시각에 sleep할 tick 수를 더해, 이 스레드가 깨어나야 할 절대 tick 값을 저장한다.
	cur->wake_tick = timer_ticks() + ticks;

	// sleep_list에 현재 스레드를 wake_tick이 빠른 순서대로 삽입한다.
	// 리스트에는 thread 전체가 아니라 cur->elem 연결 고리가 들어간다.
	list_insert_ordered(&sleep_list, &cur->elem, wake_tick_less, NULL);

	// 현재 스레드를 BLOCKED 상태로 바꿔 ready_list에서 실행 대상이 아니게 만든다.
	thread_block();

	// timer_sleep에 들어오기 전 인터럽트 상태로 복구한다.
	intr_set_level(old_level);
}

static bool wake_tick_less(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
	/*a
	= 리스트에서 받은 list_elem 주소

	struct thread
	= 이 elem이 들어있는 바깥 구조체 타입

	elem
	= struct thread 안에서 list_elem 필드 이름
	
	a 주소에서 elem 필드 위치만큼 거꾸로 빼서
    struct thread 전체 시작 주소를 찾는다*/
	struct thread *ta = list_entry(a, struct thread, elem); 
	struct thread *tb = list_entry(b, struct thread, elem);

	/*
	ta가 tb보다 빨리 깨어나야 하면 true
	→ list_insert_ordered가 ta를 tb보다 앞에 두게 함*/
	return ta->wake_tick < tb->wake_tick;	
}

/* 약 MS밀리초 동안 실행을 멈춘다. */
void
timer_msleep (int64_t ms) {
	real_time_sleep (ms, 1000);
}

/* 약 US마이크로초 동안 실행을 멈춘다. */
void
timer_usleep (int64_t us) {
	real_time_sleep (us, 1000 * 1000);
}

/* 약 NS나노초 동안 실행을 멈춘다. */
void
timer_nsleep (int64_t ns) {
	real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* 타이머 통계를 출력한다. */
void
timer_print_stats (void) {
	printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* 타이머 인터럽트 핸들러.
   타이머 인터럽트가 발생할 때마다 호출된다.
   여기서는 전체 tick을 증가시키고, sleep_list에서 깨어날 시간이 된 스레드를 깨운다. */
static void
timer_interrupt(struct intr_frame *args UNUSED)
{
	/* 시스템이 부팅된 뒤 지난 tick 수를 1 증가시킨다. */
	ticks++;

	/* 현재 실행 중인 스레드의 timer tick 처리.
	   time slice 감소, 스케줄링 관련 처리 등이 여기서 수행된다. */
	thread_tick();

	/* sleep_list에 자고 있는 스레드가 남아 있는 동안 반복한다. */
	while (!list_empty(&sleep_list))
	{

		/* sleep_list의 맨 앞 원소를 가져온다.
		   sleep_list는 wake_tick 기준 오름차순으로 정렬되어 있으므로,
		   맨 앞 원소가 가장 먼저 깨어날 스레드다. */
		struct list_elem *e = list_front(&sleep_list);

		/* list_elem만으로는 wake_tick을 알 수 없으므로,
		   e를 포함하고 있는 struct thread 전체를 찾아낸다. */
		struct thread *sleeping_thread = list_entry(e, struct thread, elem);

		/* 현재 tick이 sleeping_thread의 wake_tick보다 작으면
		   아직 깨어날 시간이 되지 않은 것이다.
		   sleep_list는 정렬되어 있으므로, 맨 앞 스레드가 아직 못 깨면
		   뒤의 스레드들도 아직 깰 시간이 아니다. */
		if (ticks < sleeping_thread->wake_tick)
		{
			break;
		}

		/* 깨어날 시간이 된 스레드를 sleep_list에서 제거한다. */
		list_pop_front(&sleep_list);

		/* 제거한 스레드를 BLOCKED 상태에서 READY 상태로 바꾸고
		   ready_list에 넣어 스케줄링 대상이 되게 한다. */
		thread_unblock(sleeping_thread);
	}
}

/* LOOPS번 반복하는 데 한 타이머 틱보다 오래 걸리면 true,
   그렇지 않으면 false를 반환한다. */
static bool
too_many_loops (unsigned loops) {
	/* 타이머 틱이 한 번 발생할 때까지 기다린다. */
	int64_t start = ticks;
	while (ticks == start)
		barrier ();

	/* LOOPS번 루프를 수행한다. */
	start = ticks;
	busy_wait (loops);

	/* 틱 수가 바뀌었다면 너무 오래 반복한 것이다. */
	barrier ();
	return start != ticks;
}

/* 짧은 지연을 구현하기 위해 단순 루프를 LOOPS번 반복한다.

   코드 정렬이 타이밍에 큰 영향을 줄 수 있으므로 NO_INLINE으로
   지정했다. 그렇지 않으면 이 함수가 위치마다 다르게 인라인되어
   결과를 예측하기 어려워질 수 있다. */
static void NO_INLINE
busy_wait (int64_t loops) {
	while (loops-- > 0)
		barrier ();
}

/* 약 NUM/DENOM초 동안 잠든다. */
static void
real_time_sleep (int64_t num, int32_t denom) {
	/* NUM/DENOM초를 타이머 틱으로 변환하되, 버림한다.

	   (NUM / DENOM) s
	   ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
	   1 s / TIMER_FREQ ticks
	   */
	int64_t ticks = num * TIMER_FREQ / denom;

	ASSERT (intr_get_level () == INTR_ON);
	if (ticks > 0) {
		/* 적어도 한 번의 온전한 타이머 틱을 기다리는 경우다.
		   timer_sleep()은 CPU를 다른 프로세스에 양보하므로 이를 사용한다. */
		timer_sleep (ticks);
	} else {
		/* 그렇지 않다면 틱보다 짧은 시간을 더 정확히 맞추기 위해
		   바쁜 대기 루프를 사용한다. 오버플로 가능성을 피하기 위해
		   분자와 분모를 1000으로 나눠 축소한다. */
		ASSERT (denom % 1000 == 0);
		busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
	}
}
