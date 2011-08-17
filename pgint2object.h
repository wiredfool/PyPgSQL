// #ident "@(#) $Id: pgint2object.h,v 1.4 2001/09/26 05:45:30 ballie01 Exp $"
#ifndef Pg_INT2OBJECT_H
#define Pg_INT2OBJECT_H
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

#define INT2_BIT (16)
#define INT2_MAX 0x7fff

typedef struct {
    PyObject_HEAD
    short ob_ival;
} PgInt2Object;

extern PyTypeObject PgInt2_Type;

#define PgInt2_Check(op) ((op)->ob_type == &PgInt2_Type)

extern PyObject *PgInt2_FromInt2(short);
extern PyObject *PgInt2_FromLong(long);
extern PyObject *PgInt2_FromString(char *, char **, int);
extern PyObject *PgInt2_FromUnicode(Py_UNICODE *s, int, int);
extern short	     PgInt2_AsLong(PgInt2Object *);
extern short	     PgInt2_AsInt2(PgInt2Object *);
extern short	     PgInt2_GetMax(void);

/* Macro, trading safety for speed */
#define PgInt2_AS_INT2(op) (((PgInt2Object *)(op))->ob_ival)

/***********************************************************************\
| initpgint2 MUST be called in the initialzation code of the module	|
| using the PgInt2 type.						|
\***********************************************************************/

extern void initpgint2(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTOBJECT_H */
