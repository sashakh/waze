/*
    Generic queueing utilities.

    Copyright (C) 2002-2005 Robert Lipe, robertlipe@usa.net

    Modified for use with RoadMap, Paul Fox, 2005

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

 */

typedef struct queue {
        struct queue *next;
        struct queue *prev;
} queue;

void enqueue(queue *new_el, queue *old);
queue * dequeue(queue *element);
int queue_count(queue *q);

#define QUEUE_INIT(head) (head)->next = ((head)->prev = head)
#define QUEUE_FIRST(head) ((head)->next)
#define QUEUE_NEXT(element) ((element)->next)
#define QUEUE_LAST(head) ((head)->prev)
#define QUEUE_EMPTY(head) ((head)->next == (head))

/* anything attached to oldhead will be re-anchord at newhead.
 * newhead will be cleared, and anything at newhead will be lost.  */
#define QUEUE_MOVE(newhead,oldhead) \
        if ( QUEUE_EMPTY(oldhead) ) {\
                QUEUE_INIT(newhead); \
        } \
        else { \
                (newhead)->next = (oldhead)->next; \
                (newhead)->prev = (oldhead)->prev; \
                (newhead)->next->prev = (newhead); \
                (newhead)->prev->next = (newhead); \
        } \
        QUEUE_INIT(oldhead)

/* anything attached to fromhead will be appended at the end of tohead.
 * fromhead is emptied.  */
#define QUEUE_SPLICE(tohead,fromhead) \
        if ( !QUEUE_EMPTY(fromhead) ) {\
                (tohead)->prev->next = (fromhead)->next; \
                (fromhead)->next->prev = (tohead)->prev; \
                (fromhead)->prev->next = (tohead); \
                (tohead)->prev = (fromhead)->prev; \
        } \
        QUEUE_INIT(fromhead)

#define ENQUEUE_TAIL(listhead, element) \
                enqueue(element, (listhead)->prev)
#define ENQUEUE_HEAD(listhead, element) \
                enqueue(element, listhead)
#define ENQUEUE_AFTER(element, newelement) \
                enqueue(newelement, element)
#define ENQUEUE_BEFORE(element, newelement) \
                enqueue(newelement, (element)->prev)

#define QUEUE_FOR_EACH(listhead, element, tmp) \
        for ((element) = QUEUE_FIRST(listhead); \
                (tmp) = QUEUE_NEXT(element), \
                (element) != (listhead); \
                (element) = (tmp))

#define QUEUE_FOR_EACH_FROM_TO(from, to, element, tmp) \
        for ((element) = (from); \
                (tmp) = QUEUE_NEXT(element), \
                (element) != (to); \
                (element) = (tmp))
