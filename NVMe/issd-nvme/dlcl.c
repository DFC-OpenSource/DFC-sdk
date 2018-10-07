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
#ifndef __linux__
#include <LPC18xx.h>
//#include "lpc_types.h"
#endif

#include "dlcl.h"

#if (!(DEBUG_INLINES && NO_INLINES))

INLINE_ONLY
	int
dlcl_ListIsValid (
		dlcl_list_t *l
		)
{
	return ( (l != NULL) &&
			(l->prev != NULL) &&
			(l->next != NULL) &&
			(l->prev->next == l->next->prev) );
}

/* Test if the list is empty. */
INLINE_ONLY
	int
dlcl_ListIsEmpty (
		dlcl_list_t *l
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l));

	return (l->prev == l && l->next == l);
}

/* Initialize a new list element. */
INLINE_ONLY
	void
dlcl_ListInit (
		dlcl_list_t *l
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(l != NULL);

	l->prev = l->next = l;
}

/* Get previous list element. */
INLINE_ONLY
	dlcl_list_t *
dlcl_ListPrev (
		dlcl_list_t *l
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l));

	return (l->prev);
}

/* Get next list element. */
INLINE_ONLY
	dlcl_list_t *
dlcl_ListNext (
		dlcl_list_t *l
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l));

	return (l->next);
}

/*
 * Add the second list L2 after the last element in list L1.
 */
INLINE_ONLY
	void
dlcl_ListAdd (
		dlcl_list_t *l1,
		dlcl_list_t *l2
		)
{
	dlcl_list_t *l1_last;
	dlcl_list_t *l2_last;

	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));
	DLCL_ASSERT(dlcl_ListIsValid(l2));

	l1_last = l1->prev;
	l2_last = l2->prev;

	l1_last->next = l2;
	l2->prev = l1_last;
	l2_last->next = l1;
	l1->prev = l2_last;

	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));
	DLCL_ASSERT(dlcl_ListIsValid(l2));
	DLCL_ASSERT(dlcl_ListIsValid(l1_last));
	DLCL_ASSERT(dlcl_ListIsValid(l2_last));
}

/*
 * Add the SINGLETON (single DLCL list element) after the last element
 * in list L1.
 *
 * This is the same as dlcl_ListAdd() except the second parameter SINGLETON
 *   MUST be a singleton.
 */
INLINE_ONLY
	void
dlcl_ListAddSingleton (
		dlcl_list_t *l1,
		dlcl_list_t *single
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));
	DLCL_ASSERT(dlcl_ListIsEmpty(single));

	/* First link the singleton, this leaves l1 unaffected. */
	single->next = l1;
	single->prev = l1->prev;

	/* Now do l1. */
	l1->prev->next = single;
	l1->prev = single;

	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));
	DLCL_ASSERT(dlcl_ListIsValid(single));
}

/*
 * Add the NEW (newly uninitialized single DLCL list element) after the last
 *   element in list L1.
 *
 * This is the same as dlcl_ListAddSingleton() except the second parameter NEW
 *   could be an uninitialized DLCL list element (no check will be done). If
 *   the NEW is still belong to some other linked list, then the old list
 *   might be BROKEN and it will be inserted to the end of list L1.
 */
INLINE_ONLY
	void
dlcl_ListAddNew (
		dlcl_list_t *l1,
		dlcl_list_t *new
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));

	new->next = l1;
	new->prev = l1->prev;

	l1->prev->next = new;
	l1->prev = new;

	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));
	DLCL_ASSERT(dlcl_ListIsValid(new));
}

/*
 * Insert the NEW (newly uninitialized single DLCL list element) after the
 *   first element in list L1.
 *
 * NEW should be a single DLCL list element NOT belong to any list. If it still
 *   belong to some other linked list, then the old list will be BROKEN and it
 *   will get inserted into the already linked list L1.
 */
INLINE_ONLY
	void
dlcl_ListAddNewAfter (
		dlcl_list_t *l1,
		dlcl_list_t *new
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));

	new->next = l1->next;
	new->prev = l1;

	l1->next->prev = new;
	l1->next = new;

	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));
	DLCL_ASSERT(dlcl_ListIsValid(new));
}

/* Remove the first element in the list L from the list. */
INLINE_ONLY
	void
dlcl_ListRemove (
		dlcl_list_t *l
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l));

	l->next->prev = l->prev;
	l->prev->next = l->next;

	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l->prev));
	DLCL_ASSERT(dlcl_ListIsValid(l->next));

	l->prev = l->next = l;

	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l));
}

/*
 * Insert list L2 before the first element in list L1.
 *
 * Exactly the same as dlcl_ListAdd().
 *
 * e.g.
 *   L1:     a <-> b <-> c <-> a
 *   L2:     d <-> e <-> f <-> d
 *   L1, L2: a <-> b <-> c <-> d <-> e <-> f <-> a
 */
INLINE_ONLY
	void
dlcl_ListInsertBefore (
		dlcl_list_t *l1,
		dlcl_list_t *l2
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));
	DLCL_ASSERT(dlcl_ListIsValid(l2));

	dlcl_ListAdd(l1,l2);
}

/*
 * Insert list L2 after the first element in list L1.
 *
 * e.g.
 *   L1:     a <-> b <-> c <-> a
 *   L2:     d <-> e <-> f <-> d
 *   L1, L2: a <-> d <-> e <-> f <-> b <-> c <-> a
 */
INLINE_ONLY
	void
dlcl_ListInsertAfter (
		dlcl_list_t *l1,
		dlcl_list_t *l2
		)
{
	/* Sanity check. */
	DLCL_ASSERT(dlcl_ListIsValid(l1));
	DLCL_ASSERT(dlcl_ListIsValid(l2));

	dlcl_ListAdd(l1->next, l2);
}

/*
 * THIS API EXPECTS THE FIXED LIST HEAD DLCL USAGE!
 * 
 * Insert a list element into the list in order. The order is determined
 *   by comparing the list element through the provided function.
 *
 *   LIST      - An existing list, MUST NOT be NULL.
 *   ELEMENT   - The element to be inserted, MUST NOT be NULL.
 *   COMPARE   - A comparison function that returns negative number
 *                 if e1 < e2, 0 if e1 = e2 and positive number if e1 > e2.
 */
INLINE_ONLY
	void
dlcl_InsertInOrder (
		dlcl_list_t *list,
		dlcl_list_t *element,
		int (*compare) (dlcl_list_t *e1, dlcl_list_t *e2)
		)
{
	dlcl_list_t *next;

	if ( dlcl_ListIsEmpty(list) )
		dlcl_ListAdd(list,element);
	else {
		next = dlcl_ListNext(list);

		while ( next != list ) { 
			if ( (*compare) (element, next) < 0 ) 
				break;

			next = dlcl_ListNext(next);
		}

		if ( next == list )
			/* Went too far. */
			next = dlcl_ListPrev(next);

		if ( (*compare) (element,next) < 0 ) {
			dlcl_ListInsertBefore(next, element);
			DLCL_ASSERT(dlcl_ListPrev(next) == element);
		}
		else {
			dlcl_ListInsertAfter(next, element);
			DLCL_ASSERT(dlcl_ListNext(next) == element);
		}
	}

	DLCL_ASSERT(!dlcl_ListIsEmpty (list));
}

/*
 * THIS API EXPECTS THE NON-FIXED LIST HEAD DLCL USAGE!
 * 
 * Insert a singleton into the list in order. The order is determined
 *   by comparing the list element through the provided function.
 *
 *   LIST      - An existing list, COULD be NULL.
 *   SINGLETON - The singleton to be inserted, MUST NOT be NULL.
 *   COMPARE   - A comparison function that returns negative number
 *                 if e1 < e2, 0 if e1 = e2 and positive number if e1 > e2.
 *
 * Returns the new head of the list, which points to the smallest element in
 *   the list, after singleton is inserted.
 */
INLINE_ONLY
	dlcl_list_t *
dlcl_InsertSingletonInOrder (
		dlcl_list_t *list,
		dlcl_list_t *singleton,
		int (*compare) (void *e1, void *e2)
		)
{
	dlcl_list_t *temp = list;

	/* SANITY CHECK. */
	DLCL_ASSERT(singleton != NULL);
	DLCL_ASSERT(dlcl_ListIsEmpty(singleton));
	DLCL_ASSERT(compare != NULL);

	if ( list == NULL )
		return singleton;

	if ( ((*compare) ((void *) singleton, (void *) temp)) <= 0 ) {
		dlcl_ListAddSingleton(temp, singleton);
		return singleton;
	}
	else {
		temp = dlcl_ListNext(temp);

		while ( temp != list ) {
			if ( ((*compare) ((void *) singleton, (void *) temp)) <= 0 ) {
				dlcl_ListAddSingleton(temp, singleton);
				return list;
			}

			temp = dlcl_ListNext(temp);
		}

		dlcl_ListAddSingleton(temp, singleton);
	}

	return list;
}

/*
 * THIS API EXPECTS THE NON-FIXED LIST HEAD DLCL USAGE!
 *
 * Remove element from the list, and return the new head of the list.
 *
 *   HEAD    - Old head of the linked list.
 *   ELEMENT - The node to be removed from the list.
 *
 * Returns the new head of the linked list. NULL if the list will be empty
 *   (element is the only node in list).
 */
INLINE_ONLY
	dlcl_list_t *
dlcl_ListRemoveAndUpdateHead (
		dlcl_list_t *head,
		dlcl_list_t *element
		)
{
	/* SANITY CHECK. */
	DLCL_ASSERT(dlcl_ListIsValid(head));
	DLCL_ASSERT(dlcl_ListIsValid(element));

	if ( element == head ) {
		head = dlcl_ListNext(head);

		if ( head == element )
			head = NULL;
	}

	dlcl_ListRemove(element);
	return head;
}

#ifdef DEBUG_ON
/* Debug function that dumps the DLCL list contents. */
INLINE_ONLY
	void
dlcl_ListDump (
		dlcl_list_t *l
		)
{
	REST_DEBUG("DLCL element %lx, next %lx, prev %lx.\n",
			(long) l, (long) l->next, (long) l->prev);
}
#else
#define dlcl_ListDump(l)
#endif /* DEBUG_ON */

#endif  /* (!(DEBUG_INLINES && NO_INLINES)) */


