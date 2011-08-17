// #ident "@(#) $Id: pgint8object.h,v 1.7 2002/10/02 11:32:07 ghaering Exp $"
#ifndef Pg_INT8OBJECT_H
#define Pg_INT8OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif
#if defined(HAVE_LONG_LONG_SUPPORT)

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

#if ! defined(LONG_LONG_BIT)
#define LONG_LONG_BIT (8 * SIZEOF_LONG_LONG)
#endif

typedef struct {
    PyObject_HEAD
    LONG_LONG ob_ival;
} PgInt8Object;

extern PyTypeObject PgInt8_Type;

#define PgInt8_Check(op) ((op)->ob_type == &PgInt8_Type)

extern PyObject *PgInt8_FromLongLong(LONG_LONG);
extern PyObject *PgInt8_FromString(char *, char **, int);
extern PyObject *PgInt8_FromUnicode(Py_UNICODE *s, int, int);
extern PyObject *PgInt8_FromLong(long);
extern LONG_LONG     PgInt8_AsLongLong(PgInt8Object *);
extern long	     PgInt8_AsLong(PgInt8Object *);
extern LONG_LONG     PgInt8_GetMax(void);

/* Macro, trading safety for speed */
#define PgInt8_AS_LONGLONG(op) (((PgInt8Object *)(op))->ob_ival)

/***********************************************************************\
| initpgint8 MUST be called in the initialzation code of the module	|
| using the PgInt8 type.						|
\***********************************************************************/

extern void initpgint8(void);

#endif /* HAVE_LONG_LONG */
#ifdef __cplusplus
}
#endif
#endif /* !Py_INTOBJECT_H */
