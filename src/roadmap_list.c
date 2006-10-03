/* roadmap_list.c - Manage a list.
 *
 * LICENSE:
 *
 *
 *   This file is part of RoadMap.
 *
 *   Current linked list implementation borrowed from gpsbabel's
 *   queue.c, which is:
 *       Copyright (C) 2002 Robert Lipe, robertlipe@usa.net
 *
 *   Previous implementation, and API spec, 
 *       Copyright 2002 Pascal F. Martin
 *
 *   Merge done by Paul Fox, 2006
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   See roadmap_list.h.
 */

#include "roadmap_list.h"

/* places "new" after "existing" */
void roadmap_list_enqueue(RoadMapListItem *new, RoadMapListItem *existing)
{
        new->next = existing->next;
        new->prev = (RoadMapListItem *)existing;
        existing->next->prev = new;
        existing->next = new;
}

RoadMapListItem * roadmap_list_remove(RoadMapListItem *element)
{
        queue *prev = element->prev;
        queue *next = element->next;

        next->prev = prev;
        prev->next = next;

        element->prev = element->next = element;

        return element;
}

int roadmap_list_count(const RoadMapList *qh)
{
        RoadMapListItem *e;
        int i = 0;

        for (e = QUEUE_FIRST(qh); e != (queue *)qh; e = QUEUE_NEXT(e))
                i++;

        return i;
}


/*
 * The following code was derived from linked-list mergesort
 * sample code by Simon Tatham, code obtained from:
 *  http://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html
 * Modified for use with RoadMap's lists by Paul Fox, October 2006.
 *
 * Original description and copyright messages follow...
 */

/*
 * Demonstration code for sorting a linked list.
 * 
 * The algorithm used is Mergesort, because that works really well
 * on linked lists, without requiring the O(N) extra space it needs
 * when you do it on arrays.
 * 
 * This code can handle singly and doubly linked lists, and
 * circular and linear lists too. For any serious application,
 * you'll probably want to remove the conditionals on `is_circular'
 * and `is_double' to adapt the code to your own purpose. 
 */

/*
 * This file is copyright 2001 Simon Tatham.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL SIMON TATHAM BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#define NULL (void *)0

typedef struct element element;
struct element {
    element *next, *prev;
    int i;
};

/*
 * This is the actual sort function. Notice that it returns the new
 * head of the list. (It has to, because the head will not
 * generally be the same element after the sort.) So unlike sorting
 * an array, where you can do
 * 
 *     sort(myarray);
 * 
 * you now have to do
 * 
 *     list = listsort(mylist);
 */
void roadmap_list_sort
    (RoadMapList *lh, int (*cmp)(RoadMapListItem *, RoadMapListItem *)) {

    RoadMapListItem *p, *q, *e, *tail, *oldhead, *list;
    int insize, nmerges, psize, qsize, i;

    /*
     * Special case: if `list' is empty, we're done.
     */
    if (ROADMAP_LIST_EMPTY(lh))
        return;

    /*
     * The algorithm doesn't really want the extra list head
     * element.  So remove the list head for now.  Put it back later.
     */

    list = ROADMAP_LIST_FIRST(lh);
    roadmap_list_remove((RoadMapListItem *)lh);

    insize = 1;

    while (1) {
        p = list;
        oldhead = list;  /* only used for circular linkage */
        list = NULL;
        tail = NULL;

        nmerges = 0;  /* count number of merges we do in this pass */

        while (p) {
            nmerges++;  /* there exists a merge to be done */
            /* step `insize' places along from p */
            q = p;
            psize = 0;
            for (i = 0; i < insize; i++) {
                psize++;
                q = (q->next == oldhead ? NULL : q->next);
                if (!q) break;
            }

            /* if q hasn't fallen off end, we have two lists to merge */
            qsize = insize;

            /* now we have two lists; merge them */
            while (psize > 0 || (qsize > 0 && q)) {

                /* decide whether next element of merge comes from p or q */
                if (psize == 0) {
                    /* p is empty; e must come from q. */
                    e = q; q = q->next; qsize--;
                    if (q == oldhead) q = NULL;
                } else if (qsize == 0 || !q) {
                    /* q is empty; e must come from p. */
                    e = p; p = p->next; psize--;
                    if (p == oldhead) p = NULL;
                } else if (cmp(p,q) <= 0) {
                    /* First element of p is lower (or same);
                     * e must come from p. */
                    e = p; p = p->next; psize--;
                    if (p == oldhead) p = NULL;
                } else {
                    /* First element of q is lower; e must come from q. */
                    e = q; q = q->next; qsize--;
                    if (q == oldhead) q = NULL;
                }

                /* add the next element to the merged list */
                if (tail) {
                    tail->next = e;
                } else {
                    list = e;
                }

                /* Maintain reverse pointers in a doubly linked list. */
                e->prev = tail;

                tail = e;
            }

            /* now p has stepped `insize' places along, and q has too */
            p = q;
        }
        tail->next = list;
        list->prev = tail;

        /* If we have done only one merge, we're finished. */
        if (nmerges <= 1) {  /* allow for nmerges==0, the empty list case */

            /* Put the list head back at the start of the list */
            roadmap_list_put_before(list, (RoadMapListItem *)lh);
            return;

        }

        /* Otherwise repeat, merging lists twice the size */
        insize *= 2;
    }
}
