/*
 * This file provides a generic doubly linked, circular list
 *   library.
 */

/*
 * README
 *
 * There are two ways this doubly linked list library is being used now.
 *
 * One usage will have a FIXED list head, even when the list is empty.
 *   When new elements get added/removed, the list head remains the same.
 *
 * The other usage will NOT have a FIXED list head, and a NULL head means
 *   the list is empty. When the new elements get added/removed, the list
 *   head needs to be updated.
 *
 * This library has API for both usages. Please understand what each API does
 *   before use them. If certain API expects particular list usage, it would
 *   explicitly indicate what type of usage it expects. If no specific usage
 *   is indicated, then it should be okay to use in either case.
 */
#ifndef _DLCL_H
#define _DLCL_H 1

#include <stdio.h>
#include <stdlib.h>

/* CNVME -FIXME*/
#if defined(__linux__) && !defined(__KERNEL__)
#define __KERNEL__
#endif

#undef DEBUG_ON
/* CNVME
 * define this when we need to have assert (don't trust our application)
 * if not defined, application should be intelligent! */
// #define DEBUG_ON

#ifdef __KERNEL__
// #undef DEBUG_ON
#ifdef DEBUG_ON
#define DLCL_ASSERT(x)  assert(x)
#else
#define DLCL_ASSERT(x)
#endif /* DEBUG_ON */

#undef INLINE_ONLY
// #define INLINE_ONLY     static inline
#define INLINE_ONLY     inline

#undef DEBUG_INLINES
#define DEBUG_INLINES   0
#undef NO_INLINES
#define NO_INLINES      0

#else /* !__KERNEL__ */
#define INLINE_ONLY
#ifdef DLCL_DEBUG_ON
#define DLCL_ASSERT(x)  ASSERT(x)
#else
#define DLCL_ASSERT(x) do {\
	if ((x)) \
	break; \
	printf ("\r\n\r\n%s:%d DLCL_ASSERT!!!\r\n", __func__, __LINE__); \
} while (0)
#endif /* DLCL_DEBUG_ON */
#endif /* _KERNEL__ */

typedef struct dlcl_list_s {
	struct dlcl_list_s *next;
	struct dlcl_list_s *prev;
}__attribute__((packed))dlcl_list_t;



#define DLCL_STATIC_INITIALIZER(list) {.prev=&list,.next=&list}

/* THESE WILL BE USED WITH NON-FIXED LIST HEAD DLCL! */
#define dlcl_inList(l)          (!dlcl_ListIsEmpty(l))
#define dlcl_ListFirst(l)       dlcl_ListNext(l)
#define dlcl_ListLast(l)        dlcl_ListPrev(l)


/* Generic list validation routine. */
INLINE_ONLY int dlcl_ListIsEmpty (dlcl_list_t *l);

INLINE_ONLY int dlcl_ListIsValid ( dlcl_list_t *l );

INLINE_ONLY void dlcl_ListInit ( dlcl_list_t *l);

INLINE_ONLY dlcl_list_t *dlcl_ListNext (dlcl_list_t *l);

INLINE_ONLY void dlcl_ListAdd (dlcl_list_t *l1,dlcl_list_t *l2);

INLINE_ONLY void dlcl_ListAddSingleton (dlcl_list_t *l1,dlcl_list_t *single);

INLINE_ONLY void dlcl_ListAddNew (dlcl_list_t *l1,dlcl_list_t *new);

INLINE_ONLY void dlcl_ListAddNewAfter (dlcl_list_t *l1,dlcl_list_t *new);

INLINE_ONLY void dlcl_ListRemove (dlcl_list_t *l);

INLINE_ONLY void dlcl_ListInsertBefore (dlcl_list_t *l1,dlcl_list_t *l2);

INLINE_ONLY void dlcl_ListInsertAfter (dlcl_list_t *l1,dlcl_list_t *l2);

INLINE_ONLY void dlcl_InsertInOrder (dlcl_list_t *list,dlcl_list_t *element,int (*compare) (dlcl_list_t *e1, dlcl_list_t *e2));

INLINE_ONLY dlcl_list_t *dlcl_InsertSingletonInOrder (dlcl_list_t *list,dlcl_list_t *singleton,int (*compare) (void *e1, void *e2));

INLINE_ONLY dlcl_list_t *dlcl_ListRemoveAndUpdateHead (dlcl_list_t *head,dlcl_list_t *element);

/* Q wrappers */
#define QTAILQ_INSERT_TAIL(head, node) \
	dlcl_ListAddSingleton ((dlcl_list_t *)head, (dlcl_list_t *)node);

#define QTAILQ_INIT(head) dlcl_ListInit ((dlcl_list_t *)head)

#define QTAILQ_EMPTY(head) dlcl_ListIsEmpty ((dlcl_list_t *)head)

#define QTAILQ_FIRST(head) dlcl_ListNext ((dlcl_list_t *)head)

#define QTAILQ_REMOVE(head, node) dlcl_ListRemove ((dlcl_list_t *)node)

#define QTAILQ_FOREACH(var, head) \
	for ((var) = (typeof(var))dlcl_ListNext ((dlcl_list_t *)head); \
			((var) != ((typeof(var))head)); (var) =  (typeof(var))dlcl_ListNext ((dlcl_list_t *)head))

#endif

