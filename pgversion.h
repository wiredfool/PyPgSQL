// #ident "@(#) $Id: pgversion.h,v 1.1 2001/08/03 23:09:26 ballie01 Exp $"
#ifndef Pg_VERSION_H
#define Pg_VERSION_H
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

/***********************************************************************\
| PgVersionObject contains the version information of the PostgreSQL	|
| backend that the pyPgSQL connection object is connected to.		|
\***********************************************************************/

extern PyTypeObject PgVersion_Type;

#define PgVersion_Check(op) ((op)->ob_type == &PgVersion_Type)

extern PyObject *PgVersion_New(char*);

/***********************************************************************\
| initpgversion MUST be called in the initialzation code of the module	|
| using the PgVersion type.						|
\***********************************************************************/

extern void initpgversion(void);

#ifdef __cplusplus
}
#endif
#endif /* !Pg_VERSION_H */
