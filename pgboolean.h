// #ident "@(#) $Id: pgboolean.h,v 1.4 2001/08/24 22:26:00 ballie01 Exp $"
#ifndef Pg_BOOLEAN_H
#define Pg_BOOLEAN_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************\
| Copyright 2000 by Billy G. Allie					|
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

/* PostgreSQL boolean object interface */

/***********************************************************************\
| PgBooleanObject represents a PostgreSQL boolean value.  This is an	|
| immutable object; a PostgreSQL boolean object cannot change its value	|
| after it's creation.							|
|									|
| There are functions to create new boolean objects, and to test an ob-	|
| ject's validity.							|
|									|
| The type PgBooleanObject is exposed here so we can declare		|
| _Pg_TrueStruct and _Pg_FalseStruct below; don't use this definition.	|
\***********************************************************************/

typedef struct {	/* DO NOT USE THIS DEFINITION */
	PyObject_HEAD	/* DO NOT USE THIS DEFINITION */
	long ob_ival;	/* DO NOT USE THIS DEFINITION */
} PgBooleanObject;	/* DO NOT USE THIS DEFINITION */

extern PyTypeObject PgBoolean_Type;

#define PgBoolean_Check(op) ((op)->ob_type == &PgBoolean_Type)

extern PyObject* PgBoolean_FromString(char*);
extern PyObject* PgBoolean_FromLong(long);

/***********************************************************************\
| Pg_False and Pg_True are special.  All values of type PGboolean must	|
| point to either of these.						|
|									|
| Don't forget to apply Py_INCREF() when returning True or False!!!	|
\***********************************************************************/

/* Don't use these directly */
extern PgBooleanObject _Pg_FalseStruct, _Pg_TrueStruct;

#define Pg_False ((PyObject *)&_Pg_FalseStruct)
#define Pg_True ((PyObject *)&_Pg_TrueStruct)

/***********************************************************************\
| initpgboolean MUST be called in the initialzation code of the module	|
| using the PgBoolean type.						|
\***********************************************************************/

extern void initpgboolean(void);

#ifdef __cplusplus
}
#endif
#endif /* !Pg_BOOLEAN_H */
