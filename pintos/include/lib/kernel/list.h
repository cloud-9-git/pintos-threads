#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H

/* Doubly linked list.
 *
 * This implementation of a doubly linked list does not require
 * use of dynamically allocated memory.  Instead, each structure
 * that is a potential list element must embed a struct list_elem
 * member.  All of the list functions operate on these `struct
 * list_elem's.  The list_entry macro allows conversion from a
 * struct list_elem back to a structure object that contains it.

 * For example, suppose there is a needed for a list of `struct
 * foo'.  `struct foo' should contain a `struct list_elem'
 * member, like so:

 * struct foo {
 *   struct list_elem elem;
 *   int bar;
 *   ...other members...
 * };

 * Then a list of `struct foo' can be be declared and initialized
 * like so:

 * struct list foo_list;

 * list_init (&foo_list);

 * Iteration is a typical situation where it is necessary to
 * convert from a struct list_elem back to its enclosing
 * structure.  Here's an example using foo_list:

 * struct list_elem *e;

 * for (e = list_begin (&foo_list); e != list_end (&foo_list);
 * e = list_next (e)) {
 *   struct foo *f = list_entry (e, struct foo, elem);
 *   ...do something with f...
 * }

 * You can find real examples of list usage throughout the
 * source; for example, malloc.c, palloc.c, and thread.c in the
 * threads directory all use lists.

 * The interface for this list is inspired by the list<> template
 * in the C++ STL.  If you're familiar with list<>, you should
 * find this easy to use.  However, it should be emphasized that
 * these lists do *no* type checking and can't do much other
 * correctness checking.  If you screw up, it will bite you.

 * Glossary of list terms:

 * - "front": The first element in a list.  Undefined in an
 * empty list.  Returned by list_front().

 * - "back": The last element in a list.  Undefined in an empty
 * list.  Returned by list_back().

 * - "tail": The element figuratively just after the last
 * element of a list.  Well defined even in an empty list.
 * Returned by list_end().  Used as the end sentinel for an
 * iteration from front to back.

 * - "beginning": In a non-empty list, the front.  In an empty
 * list, the tail.  Returned by list_begin().  Used as the
 * starting point for an iteration from front to back.

 * - "head": The element figuratively just before the first
 * element of a list.  Well defined even in an empty list.
 * Returned by list_rend().  Used as the end sentinel for an
 * iteration from back to front.

 * - "reverse beginning": In a non-empty list, the back.  In an
 * empty list, the head.  Returned by list_rbegin().  Used as
 * the starting point for an iteration from back to front.
 *
 * - "interior element": An element that is not the head or
 * tail, that is, a real list element.  An empty list does
 * not have any interior elements.*/
/* 한국어로 풀어보면:
 * 이 파일의 리스트는 "이중 연결 리스트"다.
 * 즉 각 원소가 앞 원소와 뒤 원소를 둘 다 가리키는 방식이다.
 *
 * 이 구현의 중요한 특징은, 리스트 노드를 따로 동적 할당하지 않는다는 점이다.
 * 대신 리스트에 들어갈 가능성이 있는 구조체가 자기 안에
 * `struct list_elem` 필드를 직접 포함해야 한다.
 *
 * 그래서 리스트는 구조체 전체를 직접 저장하는 것이 아니라,
 * 각 구조체 안에 들어 있는 `list_elem`들을 줄 세워 관리한다.
 * 나중에 `list_entry(...)`를 사용하면, 그 `list_elem`이 원래 어느 구조체의 것이었는지
 * 다시 찾아서 원래 구조체 포인터로 돌아갈 수 있다.
 *
 * 예를 들어 `struct thread` 안에 `elem`이 있으면,
 * 리스트에는 `thread` 전체가 아니라 `thread.elem`이 들어간다.
 * 그리고 리스트를 순회하다가 `elem`을 꺼낸 뒤,
 * `list_entry (e, struct thread, elem)`으로 다시 `struct thread *`를 얻는다.
 *
 * 용어도 같이 보면 좋다.
 * - front: 맨 앞 실제 원소
 * - back: 맨 뒤 실제 원소
 * - tail: 마지막 원소 바로 뒤에 있는 꼬리용 표시 원소
 * - beginning: 순방향 순회의 시작점
 * - head: 첫 원소 바로 앞에 있는 머리용 표시 원소
 * - reverse beginning: 역방향 순회의 시작점
 * - interior element: head/tail이 아닌 실제 데이터 원소
 *
 * 처음에는 복잡해 보여도 핵심은 이것뿐이다.
 * "구조체 안에 `list_elem`을 넣고, 리스트는 그 `list_elem`들을 연결한다." */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* List element. */
/* 한국어:
 * 리스트 안에서 원소 하나를 앞뒤와 연결할 때 쓰는 부품이다.
 * 보통 다른 구조체 안에 필드로 들어간다. */
struct list_elem {
	struct list_elem *prev;     /* Previous list element. */
	struct list_elem *next;     /* Next list element. */
};

/* List. */
/* 한국어:
 * 리스트 전체를 나타내는 구조체다.
 * `head`, `tail`은 실제 데이터가 아니라 경계 표시용 원소다. */
struct list {
	struct list_elem head;      /* List head. */
	struct list_elem tail;      /* List tail. */
};

/* Converts pointer to list element LIST_ELEM into a pointer to
   the structure that LIST_ELEM is embedded inside.  Supply the
   name of the outer structure STRUCT and the member name MEMBER
   of the list element.  See the big comment at the top of the
   file for an example. */
/* 한국어:
 * `LIST_ELEM` 포인터를 다시 "이 elem을 멤버로 갖고 있던 원래 구조체"의 포인터로 바꾼다.
 * 예를 들어 `struct thread` 안에 `elem`이 있다면,
 * `list_entry (e, struct thread, elem)`으로 `e`에서 다시 `struct thread *`를 얻는다. */
#define list_entry(LIST_ELEM, STRUCT, MEMBER)           \
	((STRUCT *) ((uint8_t *) &(LIST_ELEM)->next     \
			- offsetof (STRUCT, MEMBER.next)))

void list_init (struct list *);

/* List traversal. */
/* 한국어: 리스트를 순회할 때 쓰는 함수들. */
struct list_elem *list_begin (struct list *);
struct list_elem *list_next (struct list_elem *);
struct list_elem *list_end (struct list *);

struct list_elem *list_rbegin (struct list *);
struct list_elem *list_prev (struct list_elem *);
struct list_elem *list_rend (struct list *);

struct list_elem *list_head (struct list *);
struct list_elem *list_tail (struct list *);

/* List insertion. */
/* 한국어: 리스트에 원소를 넣을 때 쓰는 함수들. */
void list_insert (struct list_elem *, struct list_elem *);
void list_splice (struct list_elem *before,
			struct list_elem *first, struct list_elem *last);
void list_push_front (struct list *, struct list_elem *);
void list_push_back (struct list *, struct list_elem *);

/* List removal. */
/* 한국어: 리스트에서 원소를 뺄 때 쓰는 함수들. */
struct list_elem *list_remove (struct list_elem *);
struct list_elem *list_pop_front (struct list *);
struct list_elem *list_pop_back (struct list *);

/* List elements. */
/* 한국어: 리스트의 맨 앞/뒤 원소를 얻는 함수들. */
struct list_elem *list_front (struct list *);
struct list_elem *list_back (struct list *);

/* List properties. */
/* 한국어: 리스트 길이, 비었는지 여부 같은 성질을 보는 함수들. */
size_t list_size (struct list *);
bool list_empty (struct list *);

/* Miscellaneous. */
/* 한국어: 뒤집기 같은 기타 유틸 함수들. */
void list_reverse (struct list *);

/* Compares the value of two list elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
/* 한국어:
 * 리스트 원소 둘의 순서를 비교하는 비교 함수 타입이다.
 * 정렬이나 ordered insert를 할 때 사용한다. */
typedef bool list_less_func (const struct list_elem *a,
	                             const struct list_elem *b,
	                             void *aux);

/* Operations on lists with ordered elements. */
/* 한국어: 정렬된 리스트를 유지하거나 정렬할 때 쓰는 함수들. */
void list_sort (struct list *,
	                list_less_func *, void *aux);
void list_insert_ordered (struct list *, struct list_elem *,
                          list_less_func *, void *aux);
void list_unique (struct list *, struct list *duplicates,
                  list_less_func *, void *aux);

/* Max and min. */
/* 한국어: 리스트에서 가장 큰 원소와 가장 작은 원소를 찾는 함수들. */
struct list_elem *list_max (struct list *, list_less_func *, void *aux);
struct list_elem *list_min (struct list *, list_less_func *, void *aux);

#endif /* lib/kernel/list.h */
