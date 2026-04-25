#include "list.h"
#include "../debug.h"

/* Our doubly linked lists have two header elements: the "head"
   just before the first element and the "tail" just after the
   last element.  The `prev' link of the front header is null, as
   is the `next' link of the back header.  Their other two links
   point toward each other via the interior elements of the list.

   An empty list looks like this:

   +------+     +------+
   <---| head |<--->| tail |--->
   +------+     +------+

   A list with two elements in it looks like this:

   +------+     +-------+     +-------+     +------+
   <---| head |<--->|   1   |<--->|   2   |<--->| tail |<--->
   +------+     +-------+     +-------+     +------+

   The symmetry of this arrangement eliminates lots of special
   cases in list processing.  For example, take a look at
   list_remove(): it takes only two pointer assignments and no
   conditionals.  That's a lot simpler than the code would be
   without header elements.

   (Because only one of the pointers in each header element is used,
   we could in fact combine them into a single header element
   without sacrificing this simplicity.  But using two separate
   elements allows us to do a little bit of checking on some
   operations, which can be valuable.) */
/* 한국어로 풀어보면:
   이 리스트 구현에는 실제 데이터 원소 말고도 `head`와 `tail`이라는
   두 개의 표시용 원소가 있다.

   - `head`는 첫 번째 실제 원소 바로 앞
   - `tail`은 마지막 실제 원소 바로 뒤

   에 놓인다고 생각하면 된다.

   빈 리스트에서는 `head`와 `tail`이 서로 바로 연결되고,
   원소가 있으면 그 사이에 실제 원소들이 들어간다.

   이렇게 해 두면 "맨 앞에 넣는 경우", "맨 뒤에서 빼는 경우", "빈 리스트인 경우"
   같은 특수 처리가 크게 줄어든다.
   예를 들어 `list_remove()`는 조건문 없이 포인터 몇 개만 바꾸면 된다.

   즉 이 구조의 핵심 목적은 리스트 연산을 단순하게 만드는 것이다. */

static bool is_sorted (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) UNUSED;

/* Returns true if ELEM is a head, false otherwise. */
/* 한국어: ELEM이 head 표시 원소이면 true를 반환한다. */
static inline bool
is_head (struct list_elem *elem) {
	return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

/* Returns true if ELEM is an interior element,
   false otherwise. */
/* 한국어: ELEM이 head/tail이 아닌 실제 데이터 원소이면 true를 반환한다. */
static inline bool
is_interior (struct list_elem *elem) {
	return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

/* Returns true if ELEM is a tail, false otherwise. */
/* 한국어: ELEM이 tail 표시 원소이면 true를 반환한다. */
static inline bool
is_tail (struct list_elem *elem) {
	return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

/* Initializes LIST as an empty list. */
/* 한국어: LIST를 빈 리스트 상태로 초기화한다. */
void
list_init (struct list *list) {
	ASSERT (list != NULL);
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

/* Returns the beginning of LIST.  */
/* 한국어: LIST의 순방향 시작점을 반환한다. 보통 첫 실제 원소를 가리킨다. */
struct list_elem *
list_begin (struct list *list) {
	ASSERT (list != NULL);
	return list->head.next;
}

/* Returns the element after ELEM in its list.  If ELEM is the
   last element in its list, returns the list tail.  Results are
   undefined if ELEM is itself a list tail. */
/* 한국어:
   ELEM 다음 원소를 반환한다.
   ELEM이 마지막 실제 원소라면 tail을 반환한다.
   ELEM 자체가 tail이면 동작이 정의되지 않는다. */
struct list_elem *
list_next (struct list_elem *elem) {
	ASSERT (is_head (elem) || is_interior (elem));
	return elem->next;
}

/* Returns LIST's tail.

   list_end() is often used in iterating through a list from
   front to back.  See the big comment at the top of list.h for
   an example. */
/* 한국어:
   LIST의 tail 표시 원소를 반환한다.
   보통 순방향 순회에서 "끝"을 나타내는 기준으로 사용한다. */
struct list_elem *
list_end (struct list *list) {
	ASSERT (list != NULL);
	return &list->tail;
}

/* Returns the LIST's reverse beginning, for iterating through
   LIST in reverse order, from back to front. */
/* 한국어: LIST를 뒤에서 앞으로 순회할 때 시작점이 되는 원소를 반환한다. */
struct list_elem *
list_rbegin (struct list *list) {
	ASSERT (list != NULL);
	return list->tail.prev;
}

/* Returns the element before ELEM in its list.  If ELEM is the
   first element in its list, returns the list head.  Results are
   undefined if ELEM is itself a list head. */
/* 한국어:
   ELEM 바로 앞 원소를 반환한다.
   ELEM이 첫 실제 원소라면 head를 반환한다.
   ELEM 자체가 head이면 동작이 정의되지 않는다. */
struct list_elem *
list_prev (struct list_elem *elem) {
	ASSERT (is_interior (elem) || is_tail (elem));
	return elem->prev;
}

/* Returns LIST's head.

   list_rend() is often used in iterating through a list in
   reverse order, from back to front.  Here's typical usage,
   following the example from the top of list.h:

   for (e = list_rbegin (&foo_list); e != list_rend (&foo_list);
   e = list_prev (e))
   {
   struct foo *f = list_entry (e, struct foo, elem);
   ...do something with f...
   }
   */
/* 한국어:
   LIST의 head 표시 원소를 반환한다.
   역방향 순회에서 "끝"을 나타내는 기준점으로 자주 사용한다. */
struct list_elem *
list_rend (struct list *list) {
	ASSERT (list != NULL);
	return &list->head;
}

/* Return's LIST's head.

   list_head() can be used for an alternate style of iterating
   through a list, e.g.:

   e = list_head (&list);
   while ((e = list_next (e)) != list_end (&list))
   {
   ...
   }
   */
/* 한국어:
   LIST의 head를 반환한다.
   이 함수는 `list_head()` -> `list_next()` 식의 순회 패턴에서 사용할 수 있다. */
struct list_elem *
list_head (struct list *list) {
	ASSERT (list != NULL);
	return &list->head;
}

/* Return's LIST's tail. */
/* 한국어: LIST의 tail을 반환한다. */
struct list_elem *
list_tail (struct list *list) {
	ASSERT (list != NULL);
	return &list->tail;
}

/* Inserts ELEM just before BEFORE, which may be either an
   interior element or a tail.  The latter case is equivalent to
   list_push_back(). */
/* 한국어:
   ELEM을 BEFORE 바로 앞에 삽입한다.
   BEFORE는 실제 데이터 원소일 수도 있고 tail일 수도 있다.
   BEFORE가 tail이면 결국 맨 뒤에 넣는 것과 같다. */
void
list_insert (struct list_elem *before, struct list_elem *elem) {
	ASSERT (is_interior (before) || is_tail (before));
	ASSERT (elem != NULL);

	elem->prev = before->prev;
	elem->next = before;
	before->prev->next = elem;
	before->prev = elem;
}

/* Removes elements FIRST though LAST (exclusive) from their
   current list, then inserts them just before BEFORE, which may
   be either an interior element or a tail. */
/* 한국어:
   FIRST부터 LAST 직전까지의 구간을 현재 리스트에서 떼어낸 뒤,
   BEFORE 바로 앞에 한 덩어리로 끼워 넣는다. */
void
list_splice (struct list_elem *before,
			struct list_elem *first, struct list_elem *last) {
	ASSERT (is_interior (before) || is_tail (before));
	if (first == last)
		return;
	last = list_prev (last);

	ASSERT (is_interior (first));
	ASSERT (is_interior (last));

	/* Cleanly remove FIRST...LAST from its current list. */
	/* 한국어: FIRST...LAST 구간을 원래 리스트에서 깔끔하게 분리한다. */
	first->prev->next = last->next;
	last->next->prev = first->prev;

	/* Splice FIRST...LAST into new list. */
	/* 한국어: 분리한 구간을 새 위치(before 앞)에 이어 붙인다. */
	first->prev = before->prev;
	last->next = before;
	before->prev->next = first;
	before->prev = last;
}

/* Inserts ELEM at the beginning of LIST, so that it becomes the
   front in LIST. */
/* 한국어: ELEM을 LIST 맨 앞에 넣는다. */
void
list_push_front (struct list *list, struct list_elem *elem) {
	list_insert (list_begin (list), elem);
}

/* Inserts ELEM at the end of LIST, so that it becomes the
   back in LIST. */
/* 한국어: ELEM을 LIST 맨 뒤에 넣는다. */
void
list_push_back (struct list *list, struct list_elem *elem) {
	list_insert (list_end (list), elem);
}

/* Removes ELEM from its list and returns the element that
   followed it.  Undefined behavior if ELEM is not in a list.

   It's not safe to treat ELEM as an element in a list after
   removing it.  In particular, using list_next() or list_prev()
   on ELEM after removal yields undefined behavior.  This means
   that a naive loop to remove the elements in a list will fail:

 ** DON'T DO THIS **
 for (e = list_begin (&list); e != list_end (&list); e = list_next (e))
 {
 ...do something with e...
 list_remove (e);
 }
 ** DON'T DO THIS **

 Here is one correct way to iterate and remove elements from a
list:

for (e = list_begin (&list); e != list_end (&list); e = list_remove (e))
{
...do something with e...
}

If you need to free() elements of the list then you need to be
more conservative.  Here's an alternate strategy that works
even in that case:

while (!list_empty (&list))
{
struct list_elem *e = list_pop_front (&list);
	...do something with e...
	}
	*/
/* 한국어:
   ELEM을 리스트에서 제거하고, 원래 그 뒤에 있던 원소를 반환한다.
   ELEM이 리스트 안에 없으면 동작이 정의되지 않는다.

   여기서 "원래 그 뒤에 있던 원소"는
   제거하기 전 기준으로 `ELEM->next`였던 원소를 뜻한다.
   예를 들어:

   head <-> A <-> ELEM <-> B <-> tail

   에서 `list_remove (ELEM)`을 하면,
   ELEM은 빠지고 함수는 `B`를 반환한다.
   만약 ELEM이 마지막 실제 원소였다면, 그 뒤 원소는 `tail`이다.

   중요한 점은, 제거한 뒤의 ELEM을 여전히 리스트 원소처럼 다루면 안 된다는 것이다.
   즉 `list_remove(e)`를 한 뒤에 `list_next(e)` 같은 걸 호출하면 안 된다.

   그래서 원소를 순회하며 제거할 때는
   `e = list_remove(e)` 패턴을 쓰거나,
   더 안전하게는 앞에서 하나씩 `list_pop_front()`로 꺼내는 방식을 쓴다. */
struct list_elem *
list_remove (struct list_elem *elem) {
	ASSERT (is_interior (elem));
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	return elem->next;
}

/* Removes the front element from LIST and returns it.
   Undefined behavior if LIST is empty before removal. */
/* 한국어: LIST 맨 앞 원소를 제거해서 반환한다. 빈 리스트면 동작이 정의되지 않는다. */
struct list_elem *
list_pop_front (struct list *list) {
	struct list_elem *front = list_front (list);
	list_remove (front);
	return front;
}

/* Removes the back element from LIST and returns it.
   Undefined behavior if LIST is empty before removal. */
/* 한국어: LIST 맨 뒤 원소를 제거해서 반환한다. 빈 리스트면 동작이 정의되지 않는다. */
struct list_elem *
list_pop_back (struct list *list) {
	struct list_elem *back = list_back (list);
	list_remove (back);
	return back;
}

/* Returns the front element in LIST.
   Undefined behavior if LIST is empty. */
/* 한국어: LIST의 맨 앞 실제 원소를 반환한다. 빈 리스트면 동작이 정의되지 않는다. */
struct list_elem *
list_front (struct list *list) {
	ASSERT (!list_empty (list));
	return list->head.next;
}

/* Returns the back element in LIST.
   Undefined behavior if LIST is empty. */
/* 한국어: LIST의 맨 뒤 실제 원소를 반환한다. 빈 리스트면 동작이 정의되지 않는다. */
struct list_elem *
list_back (struct list *list) {
	ASSERT (!list_empty (list));
	return list->tail.prev;
}

/* Returns the number of elements in LIST.
   Runs in O(n) in the number of elements. */
/* 한국어: LIST 안 실제 원소 개수를 세어 반환한다. 시간 복잡도는 O(n)이다. */
size_t
list_size (struct list *list) {
	struct list_elem *e;
	size_t cnt = 0;

	for (e = list_begin (list); e != list_end (list); e = list_next (e))
		cnt++;
	return cnt;
}

/* Returns true if LIST is empty, false otherwise. */
/* 한국어: LIST가 비어 있으면 true, 아니면 false를 반환한다. */
bool
list_empty (struct list *list) {
	return list_begin (list) == list_end (list);
}

/* Swaps the `struct list_elem *'s that A and B point to. */
/* 한국어: A와 B가 가리키는 `struct list_elem *` 값을 서로 바꾼다. */
static void
swap (struct list_elem **a, struct list_elem **b) {
	struct list_elem *t = *a;
	*a = *b;
	*b = t;
}

/* Reverses the order of LIST. */
/* 한국어: LIST의 원소 순서를 뒤집는다. */
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

/* Returns true only if the list elements A through B (exclusive)
   are in order according to LESS given auxiliary data AUX. */
/* 한국어:
   A부터 B 직전까지의 구간이 LESS 기준으로 정렬되어 있을 때만 true를 반환한다. */
static bool
is_sorted (struct list_elem *a, struct list_elem *b,
		list_less_func *less, void *aux) {
	if (a != b)
		while ((a = list_next (a)) != b)
			if (less (a, list_prev (a), aux))
				return false;
	return true;
}

/* Finds a run, starting at A and ending not after B, of list
   elements that are in nondecreasing order according to LESS
   given auxiliary data AUX.  Returns the (exclusive) end of the
   run.
   A through B (exclusive) must form a non-empty range. */
/* 한국어:
   A에서 시작해서 B를 넘지 않는 범위 안에서,
   LESS 기준으로 오름차순이 유지되는 연속 구간(run)을 찾는다.
   반환값은 그 구간의 끝 다음 위치(exclusive end)다. */
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

/* Merges A0 through A1B0 (exclusive) with A1B0 through B1
   (exclusive) to form a combined range also ending at B1
   (exclusive).  Both input ranges must be nonempty and sorted in
   nondecreasing order according to LESS given auxiliary data
   AUX.  The output range will be sorted the same way. */
/* 한국어:
   정렬되어 있는 두 구간 A0...A1B0, A1B0...B1을 제자리에서 합쳐
   하나의 더 큰 정렬 구간으로 만든다. */
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

/* Sorts LIST according to LESS given auxiliary data AUX, using a
   natural iterative merge sort that runs in O(n lg n) time and
   O(1) space in the number of elements in LIST. */
/* 한국어:
   LIST를 LESS 기준으로 정렬한다.
   자연 병합 정렬(natural iterative merge sort)을 사용하며,
   시간 복잡도는 O(n log n), 추가 공간은 O(1)이다. */
void
list_sort (struct list *list, list_less_func *less, void *aux) {
	size_t output_run_cnt;        /* Number of runs output in current pass. */

	ASSERT (list != NULL);
	ASSERT (less != NULL);

		/* Pass over the list repeatedly, merging adjacent runs of
		   nondecreasing elements, until only one run is left. */
		/* 한국어: 정렬된 구간(run)들을 반복해서 합쳐 하나만 남을 때까지 진행한다. */
	do {
		struct list_elem *a0;     /* Start of first run. */
		struct list_elem *a1b0;   /* End of first run, start of second. */
		struct list_elem *b1;     /* End of second run. */

		output_run_cnt = 0;
		for (a0 = list_begin (list); a0 != list_end (list); a0 = b1) {
			/* Each iteration produces one output run. */
			/* 한국어: 한 번 돌 때마다 출력 구간 하나가 만들어진다. */
			output_run_cnt++;

			/* Locate two adjacent runs of nondecreasing elements
			   A0...A1B0 and A1B0...B1. */
			/* 한국어: 서로 인접한 두 개의 정렬 구간을 찾는다. */
			a1b0 = find_end_of_run (a0, list_end (list), less, aux);
			if (a1b0 == list_end (list))
				break;
			b1 = find_end_of_run (a1b0, list_end (list), less, aux);

			/* Merge the runs. */
			/* 한국어: 찾은 두 정렬 구간을 병합한다. */
			inplace_merge (a0, a1b0, b1, less, aux);
		}
	}
	while (output_run_cnt > 1);

	ASSERT (is_sorted (list_begin (list), list_end (list), less, aux));
}

/* Inserts ELEM in the proper position in LIST, which must be
   sorted according to LESS given auxiliary data AUX.
   Runs in O(n) average case in the number of elements in LIST. */
/* 한국어:
   이미 정렬되어 있는 LIST 안에서 ELEM이 들어가야 할 올바른 위치를 찾아 삽입한다.
   평균 시간 복잡도는 O(n)이다. */
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

/* Iterates through LIST and removes all but the first in each
   set of adjacent elements that are equal according to LESS
   given auxiliary data AUX.  If DUPLICATES is non-null, then the
   elements from LIST are appended to DUPLICATES. */
/* 한국어:
   LIST를 순회하면서 서로 인접한 중복 원소들 중 첫 번째만 남기고 나머지를 제거한다.
   DUPLICATES가 NULL이 아니면, 제거한 중복 원소들은 DUPLICATES 뒤에 붙인다. */
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

/* Returns the element in LIST with the largest value according
   to LESS given auxiliary data AUX.  If there is more than one
   maximum, returns the one that appears earlier in the list.  If
   the list is empty, returns its tail. */
/* 한국어:
   LESS 기준으로 가장 큰 원소를 반환한다.
   최댓값이 여러 개면 리스트에서 더 앞에 있는 것을 반환한다.
   빈 리스트면 tail을 반환한다. */
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

/* Returns the element in LIST with the smallest value according
   to LESS given auxiliary data AUX.  If there is more than one
   minimum, returns the one that appears earlier in the list.  If
   the list is empty, returns its tail. */
/* 한국어:
   LESS 기준으로 가장 작은 원소를 반환한다.
   최솟값이 여러 개면 리스트에서 더 앞에 있는 것을 반환한다.
   빈 리스트면 tail을 반환한다. */
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
