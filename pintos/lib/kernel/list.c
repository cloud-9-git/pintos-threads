#include "list.h"
#include "../debug.h"

/* 이 이중 연결 리스트는 두 개의 헤더 원소를 가진다. 첫 번째 원소 바로 앞의
   "head"와 마지막 원소 바로 뒤의 "tail"이다. 앞쪽 헤더의 `prev' 링크는
   null이고, 뒤쪽 헤더의 `next' 링크도 null이다. 나머지 두 링크는
   리스트 내부 원소들을 거쳐 서로를 향한다.

   빈 리스트는 다음과 같다:

   +------+     +------+
   <---| head |<--->| tail |--->
   +------+     +------+

   원소가 두 개 있는 리스트는 다음과 같다:

   +------+     +-------+     +-------+     +------+
   <---| head |<--->|   1   |<--->|   2   |<--->| tail |<--->
   +------+     +-------+     +-------+     +------+

   이런 배치의 대칭성 덕분에 리스트 처리에서 많은 예외 경우가 사라진다.
   예를 들어 list_remove()를 보면 포인터 대입 두 번만 필요하고 조건문은 없다.
   헤더 원소가 없을 때보다 훨씬 단순하다.

   (각 헤더 원소에서는 포인터 하나만 실제로 쓰이므로, 사실 이 단순함을
   유지한 채 하나의 헤더 원소로 합칠 수도 있다. 하지만 두 원소를 따로 두면
   몇몇 연산에서 약간의 검사를 수행할 수 있어 유용하다.) */

static bool is_sorted (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) UNUSED;

/* ELEM이 head이면 true, 아니면 false를 반환한다. */
static inline bool
is_head (struct list_elem *elem) {
	return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

/* ELEM이 내부 원소면 true,
   아니면 false를 반환한다. */
static inline bool
is_interior (struct list_elem *elem) {
	return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

/* ELEM이 tail이면 true, 아니면 false를 반환한다. */
static inline bool
is_tail (struct list_elem *elem) {
	return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

/* LIST를 빈 리스트로 초기화한다. */
void
list_init (struct list *list) {
	ASSERT (list != NULL);
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

/* LIST의 시작 원소를 반환한다. */
struct list_elem *
list_begin (struct list *list) {
	ASSERT (list != NULL);
	return list->head.next;
}

/* ELEM이 속한 리스트에서 ELEM 다음 원소를 반환한다.
   ELEM이 그 리스트의 마지막 원소라면 리스트 tail을 반환한다.
   ELEM 자체가 리스트 tail이면 결과는 정의되지 않는다. */
struct list_elem *
list_next (struct list_elem *elem) {
	ASSERT (is_head (elem) || is_interior (elem));
	return elem->next;
}

/* LIST의 tail을 반환한다.

   list_end()는 리스트를 앞에서 뒤로 순회할 때 자주 사용된다.
   예시는 list.h 상단의 긴 주석을 참고하라. */
struct list_elem *
list_end (struct list *list) {
	ASSERT (list != NULL);
	return &list->tail;
}

/* LIST를 뒤에서 앞으로 역순 순회할 때 쓰는
   LIST의 역방향 시작 지점을 반환한다. */
struct list_elem *
list_rbegin (struct list *list) {
	ASSERT (list != NULL);
	return list->tail.prev;
}

/* ELEM이 속한 리스트에서 ELEM 이전 원소를 반환한다.
   ELEM이 그 리스트의 첫 번째 원소라면 리스트 head를 반환한다.
   ELEM 자체가 리스트 head이면 결과는 정의되지 않는다. */
struct list_elem *
list_prev (struct list_elem *elem) {
	ASSERT (is_interior (elem) || is_tail (elem));
	return elem->prev;
}

/* LIST의 head를 반환한다.

   list_rend()는 리스트를 뒤에서 앞으로 역순 순회할 때 자주 사용된다.
   전형적인 사용 예는 다음과 같다. list.h 상단 예제를 따른 것이다:

   for (e = list_rbegin (&foo_list); e != list_rend (&foo_list);
   e = list_prev (e))
   {
   struct foo *f = list_entry (e, struct foo, elem);
   ...do something with f...
   }
   */
struct list_elem *
list_rend (struct list *list) {
	ASSERT (list != NULL);
	return &list->head;
}

/* LIST의 head를 반환한다.

   list_head()는 리스트를 순회하는 다른 스타일에 사용할 수 있다.
   예를 들면:

   e = list_head (&list);
   while ((e = list_next (e)) != list_end (&list))
   {
   ...
   }
   */
struct list_elem *
list_head (struct list *list) {
	ASSERT (list != NULL);
	return &list->head;
}

/* LIST의 tail을 반환한다. */
struct list_elem *
list_tail (struct list *list) {
	ASSERT (list != NULL);
	return &list->tail;
}

/* ELEM을 BEFORE 바로 앞에 삽입한다. BEFORE는 내부 원소일 수도 있고
   tail일 수도 있다. 후자의 경우는 list_push_back()과 같다. */
void
list_insert (struct list_elem *before, struct list_elem *elem) {
	ASSERT (is_interior (before) || is_tail (before));
	ASSERT (elem != NULL);

	elem->prev = before->prev;
	elem->next = before;
	before->prev->next = elem;
	before->prev = elem;
}

/* FIRST부터 LAST 직전까지의 원소들을 현재 리스트에서 제거한 뒤,
   BEFORE 바로 앞에 삽입한다. BEFORE는 내부 원소일 수도 있고
   tail일 수도 있다. */
void
list_splice (struct list_elem *before,
		struct list_elem *first, struct list_elem *last) {
	ASSERT (is_interior (before) || is_tail (before));
	if (first == last)
		return;
	last = list_prev (last);

	ASSERT (is_interior (first));
	ASSERT (is_interior (last));

	/* FIRST...LAST를 현재 리스트에서 깔끔하게 제거한다. */
	first->prev->next = last->next;
	last->next->prev = first->prev;

	/* FIRST...LAST를 새 리스트에 이어 붙인다. */
	first->prev = before->prev;
	last->next = before;
	before->prev->next = first;
	before->prev = last;
}

/* ELEM을 LIST의 맨 앞에 삽입해
   LIST의 front가 되게 한다. */
void
list_push_front (struct list *list, struct list_elem *elem) {
	list_insert (list_begin (list), elem);
}

/* ELEM을 LIST의 맨 뒤에 삽입해
   LIST의 back이 되게 한다. */
void
list_push_back (struct list *list, struct list_elem *elem) {
	list_insert (list_end (list), elem);
}

/* ELEM을 리스트에서 제거하고, 그 뒤에 있던 원소를 반환한다.
   ELEM이 어떤 리스트에도 속하지 않으면 동작은 정의되지 않는다.

   제거한 뒤에는 ELEM을 더 이상 리스트의 원소처럼 다루면 안전하지 않다.
   특히 제거 후 ELEM에 대해 list_next()나 list_prev()를 사용하면
   동작은 정의되지 않는다. 즉, 리스트 원소를 제거하는 순진한 루프는 실패한다:

 ** 이렇게 하지 말 것 **
 for (e = list_begin (&list); e != list_end (&list); e = list_next (e))
 {
 ...do something with e...
 list_remove (e);
 }
 ** 이렇게 하지 말 것 **

 다음은 리스트를 순회하면서 원소를 제거하는 올바른 방법 중 하나다:

for (e = list_begin (&list); e != list_end (&list); e = list_remove (e))
{
...do something with e...
}

리스트 원소를 free()해야 한다면 더 조심해야 한다.
이 경우에도 동작하는 다른 전략은 다음과 같다:

while (!list_empty (&list))
{
struct list_elem *e = list_pop_front (&list);
...do something with e...
}
*/
struct list_elem *
list_remove (struct list_elem *elem) {
	ASSERT (is_interior (elem));
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	return elem->next;
}

/* LIST의 front 원소를 제거하고 반환한다.
   제거 전에 LIST가 비어 있으면 동작은 정의되지 않는다. */
struct list_elem *
list_pop_front (struct list *list) {
	struct list_elem *front = list_front (list);
	list_remove (front);
	return front;
}

/* LIST의 back 원소를 제거하고 반환한다.
   제거 전에 LIST가 비어 있으면 동작은 정의되지 않는다. */
struct list_elem *
list_pop_back (struct list *list) {
	struct list_elem *back = list_back (list);
	list_remove (back);
	return back;
}

/* LIST의 front 원소를 반환한다.
   LIST가 비어 있으면 동작은 정의되지 않는다. */
struct list_elem *
list_front (struct list *list) {
	ASSERT (!list_empty (list));
	return list->head.next;
}

/* LIST의 back 원소를 반환한다.
   LIST가 비어 있으면 동작은 정의되지 않는다. */
struct list_elem *
list_back (struct list *list) {
	ASSERT (!list_empty (list));
	return list->tail.prev;
}

/* LIST 안의 원소 개수를 반환한다.
   시간 복잡도는 원소 수에 대해 O(n)이다. */
size_t
list_size (struct list *list) {
	struct list_elem *e;
	size_t cnt = 0;

	for (e = list_begin (list); e != list_end (list); e = list_next (e))
		cnt++;
	return cnt;
}

/* LIST가 비어 있으면 true, 아니면 false를 반환한다. */
bool
list_empty (struct list *list) {
	return list_begin (list) == list_end (list);
}

/* A와 B가 가리키는 `struct list_elem *'를 서로 바꾼다. */
static void
swap (struct list_elem **a, struct list_elem **b) {
	struct list_elem *t = *a;
	*a = *b;
	*b = t;
}

/* LIST의 순서를 뒤집는다. */
void
list_reverse (struct list *list) {
	if (!list_empty (list)) {
		struct list_elem *e;

		for (e = list_begin (list); e != list_end (list); e = e->prev)
			swap (&e->prev, &e->next);
		swap (&list->head.next, &list->tail.prev);
		swap (&list->head.next->prev, &list->tail.prev->next);
	}
}

/* 보조 데이터 AUX와 LESS 기준에 따라
   A부터 B 직전까지의 리스트 원소들이 정렬되어 있을 때만 true를 반환한다. */
static bool
is_sorted (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) {
	if (a != b)
		while ((a = list_next (a)) != b)
			if (less (a, list_prev (a), aux))
				return false;
	return true;
}

/* A에서 시작해서 B를 넘지 않는 범위 안에서,
   보조 데이터 AUX와 LESS 기준에 따라 비내림차순인 리스트 원소 구간을 찾는다.
   그 구간의 끝(배타적)을 반환한다.
   A부터 B 직전까지는 비어 있지 않은 구간이어야 한다. */
static struct list_elem *
find_end_of_run (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) {
	ASSERT (a != NULL);
	ASSERT (b != NULL);
	ASSERT (less != NULL);
	ASSERT (a != b);

	do {
		a = list_next (a);
	} while (a != b && !less (a, list_prev (a), aux));
	return a;
}

/* A0부터 A1B0 직전까지와 A1B0부터 B1 직전까지를 병합해,
   역시 B1 직전에서 끝나는 하나의 결합된 구간을 만든다.
   두 입력 구간은 모두 비어 있지 않아야 하며, 보조 데이터 AUX와 LESS 기준에 따라
   비내림차순으로 정렬되어 있어야 한다. 출력 구간도 같은 기준으로 정렬된다. */
static void
inplace_merge (struct list_elem *a0, struct list_elem *a1b0,
		struct list_elem *b1,
		list_less_func *less, void *aux) {
	ASSERT (a0 != NULL);
	ASSERT (a1b0 != NULL);
	ASSERT (b1 != NULL);
	ASSERT (less != NULL);
	ASSERT (is_sorted (a0, a1b0, less, aux));
	ASSERT (is_sorted (a1b0, b1, less, aux));

	while (a0 != a1b0 && a1b0 != b1)
		if (!less (a1b0, a0, aux))
			a0 = list_next (a0);
		else {
			a1b0 = list_next (a1b0);
			list_splice (a0, list_prev (a1b0), a1b0);
		}
}

/* 보조 데이터 AUX와 LESS 기준에 따라 LIST를 정렬한다.
   LIST 원소 수에 대해 시간 O(n lg n), 공간 O(1)인
   자연 반복 병합 정렬을 사용한다. */
void
list_sort (struct list *list, list_less_func *less, void *aux) {
	size_t output_run_cnt;        /* 현재 패스에서 출력된 run 개수. */

	ASSERT (list != NULL);
	ASSERT (less != NULL);

	/* 비내림차순인 인접 run들을 병합하면서 리스트를 반복해서 순회해,
	   run이 하나만 남을 때까지 계속한다. */
	do {
		struct list_elem *a0;     /* 첫 번째 run의 시작. */
		struct list_elem *a1b0;   /* 첫 번째 run의 끝이자 두 번째 run의 시작. */
		struct list_elem *b1;     /* 두 번째 run의 끝. */

		output_run_cnt = 0;
		for (a0 = list_begin (list); a0 != list_end (list); a0 = b1) {
			/* 각 반복은 출력 run 하나를 만든다. */
			output_run_cnt++;

			/* 비내림차순인 인접 run A0...A1B0와
			   A1B0...B1 두 개를 찾는다. */
			a1b0 = find_end_of_run (a0, list_end (list), less, aux);
			if (a1b0 == list_end (list))
				break;
			b1 = find_end_of_run (a1b0, list_end (list), less, aux);

			/* run들을 병합한다. */
			inplace_merge (a0, a1b0, b1, less, aux);
		}
	}
	while (output_run_cnt > 1);

	ASSERT (is_sorted (list_begin (list), list_end (list), less, aux));
}

/* 보조 데이터 AUX와 LESS 기준으로 정렬되어 있어야 하는 LIST 안의
   알맞은 위치에 ELEM을 삽입한다.
   평균 시간 복잡도는 LIST 원소 수에 대해 O(n)이다. */
void
list_insert_ordered (struct list *list, struct list_elem *elem,
		list_less_func *less, void *aux) {
	struct list_elem *e;

	ASSERT (list != NULL);
	ASSERT (elem != NULL);
	ASSERT (less != NULL);

	for (e = list_begin (list); e != list_end (list); e = list_next (e))
		if (less (elem, e, aux))
			break;
	return list_insert (e, elem);
}

/* LIST를 순회하면서, 보조 데이터 AUX와 LESS 기준으로 같은 값을 가지는
   인접 원소 집합마다 첫 번째를 제외한 나머지를 모두 제거한다.
   DUPLICATES가 null이 아니면, LIST에서 제거된 원소를 DUPLICATES 뒤에 붙인다. */
void
list_unique (struct list *list, struct list *duplicates,
		list_less_func *less, void *aux) {
	struct list_elem *elem, *next;

	ASSERT (list != NULL);
	ASSERT (less != NULL);
	if (list_empty (list))
		return;

	elem = list_begin (list);
	while ((next = list_next (elem)) != list_end (list))
		if (!less (elem, next, aux) && !less (next, elem, aux)) {
			list_remove (next);
			if (duplicates != NULL)
				list_push_back (duplicates, next);
		} else
			elem = next;
}

/* 보조 데이터 AUX와 LESS 기준으로 LIST에서 가장 큰 값을 가진 원소를 반환한다.
   최댓값이 둘 이상이면 리스트에서 더 앞에 나타나는 원소를 반환한다.
   리스트가 비어 있으면 tail을 반환한다. */
struct list_elem *
list_max (struct list *list, list_less_func *less, void *aux) {
	struct list_elem *max = list_begin (list);
	if (max != list_end (list)) {
		struct list_elem *e;

		for (e = list_next (max); e != list_end (list); e = list_next (e))
			if (less (max, e, aux))
				max = e;
	}
	return max;
}

/* 보조 데이터 AUX와 LESS 기준으로 LIST에서 가장 작은 값을 가진 원소를 반환한다.
   최솟값이 둘 이상이면 리스트에서 더 앞에 나타나는 원소를 반환한다.
   리스트가 비어 있으면 tail을 반환한다. */
struct list_elem *
list_min (struct list *list, list_less_func *less, void *aux) {
	struct list_elem *min = list_begin (list);
	if (min != list_end (list)) {
		struct list_elem *e;

		for (e = list_next (min); e != list_end (list); e = list_next (e))
			if (less (e, min, aux))
				min = e;
	}
	return min;
}
