#ifndef __LIB_KERNEL_LIST_H              /* 이 헤더가 여러 번 포함되는 것을 막는다. */
#define __LIB_KERNEL_LIST_H              /* include guard를 시작한다. */

/* 이중 연결 리스트.
 *
 * 이 구현은 동적 메모리 할당을 요구하지 않는다.
 * 대신 리스트 원소가 될 수 있는 각 구조체는 반드시
 * `struct list_elem` 멤버를 내부에 포함해야 한다.
 * 리스트 함수들은 모두 이 `struct list_elem`을 기준으로 동작한다.
 * `list_entry` 매크로는 `struct list_elem` 포인터를,
 * 그것을 포함하는 바깥 구조체 포인터로 되돌리는 데 사용된다.
 *
 * 예를 들어 `struct foo`의 리스트가 필요하다고 하자.
 * `struct foo`는 다음처럼 `struct list_elem` 멤버를 가져야 한다:
 *
 * struct foo {
 *   struct list_elem elem;
 *   int bar;
 *   ...other members...
 * };
 *
 * 그러면 `struct foo`의 리스트는 다음처럼 선언하고 초기화할 수 있다:
 *
 * struct list foo_list;
 *
 * list_init (&foo_list);
 *
 * 순회(iteration)는 `struct list_elem`에서 그것을 감싸는 구조체로
 * 다시 변환해야 하는 대표적인 상황이다. foo_list를 이용한 예시는 다음과 같다:
 *
 * struct list_elem *e;
 *
 * for (e = list_begin (&foo_list); e != list_end (&foo_list);
 * e = list_next (e)) {
 *   struct foo *f = list_entry (e, struct foo, elem);
 *   ...do something with f...
 * }
 *
 * 실제 리스트 사용 예시는 소스 곳곳에서 찾을 수 있다.
 * 예를 들면 threads 디렉터리의 malloc.c, palloc.c, thread.c 모두
 * 리스트를 사용한다.
 *
 * 이 리스트 인터페이스는 C++ STL의 list<> 템플릿에서 영감을 받았다.
 * list<>에 익숙하다면 쉽게 사용할 수 있을 것이다.
 * 다만 이 리스트는 타입 검사도 하지 않고, 그 외의 올바름 검사도 많이 하지 못한다.
 * 잘못 쓰면 그대로 문제가 생긴다.
 *
 * 리스트 용어 정리:
 *
 * - "front": 리스트의 첫 번째 원소.
 *   빈 리스트에서는 정의되지 않는다. list_front()가 반환한다.
 *
 * - "back": 리스트의 마지막 원소.
 *   빈 리스트에서는 정의되지 않는다. list_back()가 반환한다.
 *
 * - "tail": 리스트의 마지막 원소 바로 뒤에 있다고 생각하는 원소.
 *   빈 리스트에서도 항상 정의된다.
 *   list_end()가 반환하며, 앞에서 뒤로 순회할 때 종료 표시(sentinel)로 쓴다.
 *
 * - "beginning": 비어 있지 않은 리스트에서는 front,
 *   빈 리스트에서는 tail이다. list_begin()이 반환하며,
 *   앞에서 뒤로 순회할 때 시작 지점으로 쓴다.
 *
 * - "head": 리스트의 첫 번째 원소 바로 앞에 있다고 생각하는 원소.
 *   빈 리스트에서도 항상 정의된다.
 *   list_rend()가 반환하며, 뒤에서 앞으로 순회할 때 종료 표시로 쓴다.
 *
 * - "reverse beginning": 비어 있지 않은 리스트에서는 back,
 *   빈 리스트에서는 head이다. list_rbegin()이 반환하며,
 *   뒤에서 앞으로 순회할 때 시작 지점으로 쓴다.
 *
 * - "interior element": head나 tail이 아닌 원소, 즉 실제 리스트 원소다.
 *   빈 리스트에는 내부 원소가 하나도 없다. */

#include <stdbool.h>                     /* bool 타입을 사용하기 위해 포함한다. */
#include <stddef.h>                      /* offsetof, size_t를 사용하기 위해 포함한다. */
#include <stdint.h>                      /* uint8_t 같은 고정 폭 정수를 사용하기 위해 포함한다. */

/* 리스트에 실제로 연결되는 노드 하나를 나타낸다. */
struct list_elem {
	struct list_elem *prev;     /* 바로 이전 리스트 원소를 가리킨다. */
	struct list_elem *next;     /* 바로 다음 리스트 원소를 가리킨다. */
};

/* 리스트 전체를 나타낸다. */
struct list {
	struct list_elem head;      /* 맨 앞 경계 역할을 하는 head sentinel이다. */
	struct list_elem tail;      /* 맨 뒤 경계 역할을 하는 tail sentinel이다. */
};

/* LIST_ELEM이 포함된 바깥 구조체의 주소를 구한다.
   STRUCT에는 바깥 구조체 타입 이름을,
   MEMBER에는 그 구조체 안의 list_elem 멤버 이름을 넣는다.
   위의 긴 설명 블록에 사용 예가 있다. */
#define list_entry(LIST_ELEM, STRUCT, MEMBER)           \
	((STRUCT *) ((uint8_t *) &(LIST_ELEM)->next     \
		- offsetof (STRUCT, MEMBER.next)))       /* next 멤버 오프셋을 이용해 원래 구조체 주소를 계산한다. */

void list_init (struct list *);                         /* 리스트를 빈 상태로 초기화한다. */

/* 리스트 순회 관련 함수들. */
struct list_elem *list_begin (struct list *);          /* 앞에서 뒤로 순회할 때 시작 원소를 반환한다. */
struct list_elem *list_next (struct list_elem *);      /* 현재 원소의 다음 원소를 반환한다. */
struct list_elem *list_end (struct list *);            /* 앞방향 순회의 종료 지점인 tail을 반환한다. */

struct list_elem *list_rbegin (struct list *);         /* 뒤에서 앞으로 순회할 때 시작 원소를 반환한다. */
struct list_elem *list_prev (struct list_elem *);      /* 현재 원소의 이전 원소를 반환한다. */
struct list_elem *list_rend (struct list *);           /* 역방향 순회의 종료 지점인 head를 반환한다. */

struct list_elem *list_head (struct list *);           /* 리스트의 head sentinel 자체를 반환한다. */
struct list_elem *list_tail (struct list *);           /* 리스트의 tail sentinel 자체를 반환한다. */

/* 리스트 삽입 관련 함수들. */
void list_insert (struct list_elem *, struct list_elem *); /* 특정 원소 바로 앞에 새 원소를 삽입한다. */
void list_splice (struct list_elem *before,            /* FIRST부터 LAST 직전까지를 BEFORE 앞에 통째로 옮긴다. */
		struct list_elem *first, struct list_elem *last);
void list_push_front (struct list *, struct list_elem *); /* 리스트 맨 앞에 원소를 넣는다. */
void list_push_back (struct list *, struct list_elem *);  /* 리스트 맨 뒤에 원소를 넣는다. */

/* 리스트 제거 관련 함수들. */
struct list_elem *list_remove (struct list_elem *);    /* 특정 원소를 제거하고 그 다음 원소를 반환한다. */
struct list_elem *list_pop_front (struct list *);      /* 맨 앞 원소를 꺼내서 제거한다. */
struct list_elem *list_pop_back (struct list *);       /* 맨 뒤 원소를 꺼내서 제거한다. */

/* 리스트 원소 접근 함수들. */
struct list_elem *list_front (struct list *);          /* 맨 앞 실제 원소를 반환한다. */
struct list_elem *list_back (struct list *);           /* 맨 뒤 실제 원소를 반환한다. */

/* 리스트 상태를 확인하는 함수들. */
size_t list_size (struct list *);                      /* 리스트 안 원소 개수를 센다. */
bool list_empty (struct list *);                       /* 리스트가 비어 있는지 확인한다. */

/* 기타 보조 함수. */
void list_reverse (struct list *);                     /* 리스트 원소 순서를 반대로 뒤집는다. */

/* 두 리스트 원소 A와 B를 비교하는 함수 타입이다.
   AUX는 비교에 필요한 보조 데이터를 전달할 때 사용한다.
   A가 B보다 작으면 true, 그렇지 않으면 false를 반환한다. */
typedef bool list_less_func (const struct list_elem *a, /* 비교 대상 왼쪽 원소. */
                             const struct list_elem *b, /* 비교 대상 오른쪽 원소. */
                             void *aux);                /* 비교 시 함께 넘길 추가 데이터. */

/* 정렬된 리스트를 다루는 연산들. */
void list_sort (struct list *,                         /* 리스트 전체를 정렬한다. */
                list_less_func *, void *aux);
void list_insert_ordered (struct list *, struct list_elem *, /* 정렬 순서를 유지하며 원소를 삽입한다. */
                          list_less_func *, void *aux);
void list_unique (struct list *, struct list *duplicates,    /* 인접한 중복 원소를 제거하고 필요하면 duplicates에 모은다. */
                  list_less_func *, void *aux);

/* 최댓값/최솟값 탐색 함수들. */
struct list_elem *list_max (struct list *, list_less_func *, void *aux); /* 가장 큰 원소를 찾는다. */
struct list_elem *list_min (struct list *, list_less_func *, void *aux); /* 가장 작은 원소를 찾는다. */

#endif /* lib/kernel/list.h */          /* include guard를 끝낸다. */
