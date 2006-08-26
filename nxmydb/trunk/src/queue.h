/*-
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: /repoman/r/ncvs/src/sys/sys/queue.h,v 1.66 2006/05/26 18:17:53 emaste Exp $
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

/*
 * A tail queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or
 * after an existing element, at the head of the list, or at the end of
 * the list. A tail queue may be traversed in either direction.
 *
 * For details on the use of these macros, see the queue(3) manual page.
 */

#ifdef DEBUG
/* Store the last 2 places the queue element or head was altered */
struct qm_trace {
    char *lastfile;
    int lastline;
    char *prevfile;
    int prevline;
};

#define TRACE_BUF struct qm_trace trace;

#define TRASH_IT(x) do {                                                        \
    (x) = (void *)-1;                                                           \
} while (0)

#define QMD_TRACE_HEAD(head) do {                                               \
    (head)->trace.prevline = (head)->trace.lastline;                            \
    (head)->trace.prevfile = (head)->trace.lastfile;                            \
    (head)->trace.lastline = __LINE__;                                          \
    (head)->trace.lastfile = __FILE__;                                          \
} while (0)

#define QMD_TRACE_ELEM(elem) do {                                               \
    (elem)->trace.prevline = (elem)->trace.lastline;                            \
    (elem)->trace.prevfile = (elem)->trace.lastfile;                            \
    (elem)->trace.lastline = __LINE__;                                          \
    (elem)->trace.lastfile = __FILE__;                                          \
} while (0)

#define QMD_TAILQ_CHECK_HEAD(head, field) do {                                  \
    if (!TAILQ_EMPTY(head) &&                                                   \
            TAILQ_FIRST((head))->field.tqe_prev != &TAILQ_FIRST((head))) {      \
        Assert(!"Bad tailq head: first->prev != head");                         \
    }                                                                           \
} while (0)

#define QMD_TAILQ_CHECK_TAIL(head, field) do {                                  \
    if (*(head)->tqh_last != NULL) {                                            \
        Assert(!"Bad tailq: NEXT(head->tqh_last) != NULL");                     \
    }                                                                           \
} while (0)

#define QMD_TAILQ_CHECK_NEXT(elm, field) do {                                   \
    if (TAILQ_NEXT((elm), field) != NULL &&                                     \
            TAILQ_NEXT((elm), field)->field.tqe_prev !=                         \
            &((elm)->field.tqe_next)) {                                         \
        Assert(!"Bad link elm: next->prev != elm");                             \
    }                                                                           \
} while (0)

#define QMD_TAILQ_CHECK_PREV(elm, field) do {                                   \
    if (*(elm)->field.tqe_prev != (elm)) {                                      \
        Assert(!"Bad link elm: prev->next != elm");                             \
    }                                                                           \
} while (0)

#else
#define QMD_TRACE_ELEM(elem)
#define QMD_TRACE_HEAD(head)
#define QMD_TAILQ_CHECK_HEAD(head, field)
#define QMD_TAILQ_CHECK_TAIL(head, headname)
#define QMD_TAILQ_CHECK_NEXT(elm, field)
#define QMD_TAILQ_CHECK_PREV(elm, field)
#define TRACE_BUF
#define TRASH_IT(x)
#endif  /* DEBUG */

/*
 * Tail queue declarations.
 */
#define TAILQ_HEAD(name, type)                                                  \
struct name {                                                                   \
    struct type *tqh_first; /* first element */                                 \
    struct type **tqh_last; /* addr of last next element */                     \
    TRACE_BUF                                                                   \
}

#define TAILQ_HEAD_INITIALIZER(head)                                            \
    { NULL, &(head).tqh_first }

#define TAILQ_ENTRY(type)                                                       \
struct {                                                                        \
    struct type *tqe_next;  /* next element */                                  \
    struct type **tqe_prev; /* address of previous next element */              \
    TRACE_BUF                                                                   \
}

/*
 * Tail queue functions.
 */
#define TAILQ_CONCAT(head1, head2, field) do {                                  \
    if (!TAILQ_EMPTY(head2)) {                                                  \
        *(head1)->tqh_last = (head2)->tqh_first;                                \
        (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;                 \
        (head1)->tqh_last = (head2)->tqh_last;                                  \
        TAILQ_INIT((head2));                                                    \
        QMD_TRACE_HEAD(head1);                                                  \
        QMD_TRACE_HEAD(head2);                                                  \
    }                                                                           \
} while (0)

#define TAILQ_EMPTY(head)   ((head)->tqh_first == NULL)

#define TAILQ_FIRST(head)   ((head)->tqh_first)

#define TAILQ_FOREACH(var, head, field)                                         \
    for ((var) = TAILQ_FIRST((head));                                           \
        (var);                                                                  \
        (var) = TAILQ_NEXT((var), field))

#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                              \
    for ((var) = TAILQ_FIRST((head));                                           \
        (var) && ((tvar) = TAILQ_NEXT((var), field), 1);                        \
        (var) = (tvar))

#define TAILQ_FOREACH_REVERSE(var, head, headname, field)                       \
    for ((var) = TAILQ_LAST((head), headname);                                  \
        (var);                                                                  \
        (var) = TAILQ_PREV((var), headname, field))

#define TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)            \
    for ((var) = TAILQ_LAST((head), headname);                                  \
        (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);              \
        (var) = (tvar))

#define TAILQ_INIT(head) do {                                                   \
    TAILQ_FIRST((head)) = NULL;                                                 \
    (head)->tqh_last = &TAILQ_FIRST((head));                                    \
    QMD_TRACE_HEAD(head);                                                       \
} while (0)

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) do {                      \
    QMD_TAILQ_CHECK_NEXT(listelm, field);                                       \
    if ((TAILQ_NEXT((elm), field) = TAILQ_NEXT((listelm), field)) != NULL) {    \
        TAILQ_NEXT((elm), field)->field.tqe_prev = &TAILQ_NEXT((elm), field);   \
    } else {                                                                    \
        (head)->tqh_last = &TAILQ_NEXT((elm), field);                           \
        QMD_TRACE_HEAD(head);                                                   \
    }                                                                           \
    TAILQ_NEXT((listelm), field) = (elm);                                       \
    (elm)->field.tqe_prev = &TAILQ_NEXT((listelm), field);                      \
    QMD_TRACE_ELEM(&(elm)->field);                                              \
    QMD_TRACE_ELEM(&listelm->field);                                            \
} while (0)

#define TAILQ_INSERT_BEFORE(listelm, elm, field) do {                           \
    QMD_TAILQ_CHECK_PREV(listelm, field);                                       \
    (elm)->field.tqe_prev = (listelm)->field.tqe_prev;                          \
    TAILQ_NEXT((elm), field) = (listelm);                                       \
    *(listelm)->field.tqe_prev = (elm);                                         \
    (listelm)->field.tqe_prev = &TAILQ_NEXT((elm), field);                      \
    QMD_TRACE_ELEM(&(elm)->field);                                              \
    QMD_TRACE_ELEM(&listelm->field);                                            \
} while (0)

#define TAILQ_INSERT_HEAD(head, elm, field) do {                                \
    QMD_TAILQ_CHECK_HEAD(head, field);                                          \
    if ((TAILQ_NEXT((elm), field) = TAILQ_FIRST((head))) != NULL) {             \
        TAILQ_FIRST((head))->field.tqe_prev = &TAILQ_NEXT((elm), field);        \
    } else {                                                                    \
        (head)->tqh_last = &TAILQ_NEXT((elm), field);                           \
    }                                                                           \
    TAILQ_FIRST((head)) = (elm);                                                \
    (elm)->field.tqe_prev = &TAILQ_FIRST((head));                               \
    QMD_TRACE_HEAD(head);                                                       \
    QMD_TRACE_ELEM(&(elm)->field);                                              \
} while (0)

#define TAILQ_INSERT_TAIL(head, elm, field) do {                                \
    QMD_TAILQ_CHECK_TAIL(head, field);                                          \
    TAILQ_NEXT((elm), field) = NULL;                                            \
    (elm)->field.tqe_prev = (head)->tqh_last;                                   \
    *(head)->tqh_last = (elm);                                                  \
    (head)->tqh_last = &TAILQ_NEXT((elm), field);                               \
    QMD_TRACE_HEAD(head);                                                       \
    QMD_TRACE_ELEM(&(elm)->field);                                              \
} while (0)

#define TAILQ_LAST(head, headname)                                              \
    (*(((struct headname *)((head)->tqh_last))->tqh_last))

#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define TAILQ_PREV(elm, headname, field)                                        \
    (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#define TAILQ_REMOVE(head, elm, field) do {                                     \
    QMD_TAILQ_CHECK_NEXT(elm, field);                                           \
    QMD_TAILQ_CHECK_PREV(elm, field);                                           \
    if ((TAILQ_NEXT((elm), field)) != NULL) {                                   \
        TAILQ_NEXT((elm), field)->field.tqe_prev = (elm)->field.tqe_prev;       \
    } else {                                                                    \
        (head)->tqh_last = (elm)->field.tqe_prev;                               \
        QMD_TRACE_HEAD(head);                                                   \
    }                                                                           \
    *(elm)->field.tqe_prev = TAILQ_NEXT((elm), field);                          \
    TRASH_IT((elm)->field.tqe_next);                                            \
    TRASH_IT((elm)->field.tqe_prev);                                            \
    QMD_TRACE_ELEM(&(elm)->field);                                              \
} while (0)

#endif /* _QUEUE_H_ */
