// #ident "@(#) $Id: pgnotify.h,v 1.1 2001/08/24 22:26:00 ballie01 Exp $"
#ifndef Pg_NOTIFY_H
#define Pg_NOTIFY_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************\
| Copyright 2001 by Billy G. Allie					|
| All rights Reserved.							|
|									|
| Permission to use, copy, modify, and distribute this software and its	|
| documentation for any purpose and without fee is hereby granted, pro-	|
| vided that the above copyright notice appear in all copies and that	|
| both that copyright notice and this permission notice appear in sup-	|
| porting documentation, and that the copyright owner's name not be	|
| used in advertising or publicity pertaining to distribution of the	|
| software without specific, written prior permission.			|
|									|
| THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,	|
| INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN	|
| NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR	|
| CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS	|
| OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE	|
| OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE	|
| USE OR PERFORMANCE OF THIS SOFTWARE.					|
\***********************************************************************/

/* pyPgSQL version object interface */

/*******************************\
| PgNotify object definition	|
\*******************************/

typedef struct {
    PyObject_HEAD
    PyObject *relname;	/* The name of the relation containing data     */
    PyObject *be_pid;	/* The process ID of the notifing backend	*/
} PgNotify;

/***********************************************************************\
| PgNotifyObject defines the PostgreSQL Notify object for Python.	|
\***********************************************************************/

extern PyTypeObject PgNotify_Type;

#define PgNotify_Check(op) ((op)->ob_type == &PgNotify_Type)

extern PyObject *PgNotify_New(PGnotify *);

/***********************************************************************\
| initpgnotify MUST be called in the initialzation code of the module	|
| using the PgNotify type.						|
\***********************************************************************/

extern void initpgnotify(void);

void *PyMem_Strdup(void *);

#ifdef __cplusplus
}
#endif
#endif /* !Pg_NOTIFY_H */
