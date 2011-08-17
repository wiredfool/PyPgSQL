// #ident "@(#) $Id: pgresult.h,v 1.3 2001/09/24 07:10:56 ballie01 Exp $"
#ifndef Pg_RESULT_H
#define Pg_RESULT_H
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

/* pyPgSQL result object interface */

/***********************************************************************\
| PgResult defines the PostgreSQL PgResult Object.			|
\***********************************************************************/

/*******************************\
| PgResult object definition	|
\*******************************/

typedef enum {
    RESULT_ERROR = -1,
    RESULT_EMPTY,
    RESULT_DQL,
    RESULT_DDL,
    RESULT_DML
} resulttypes;

typedef struct {
    PyObject_HEAD
    PGresult *res;	/* The libpq result object.			*/
    PgConnection *conn;	/* The python connection object for this result */
    PyObject *type;	/* The type of the result.			*/
    PyObject *status;	/* The result status.				*/
    PyObject *ntuples;	/* The number of tuples returns.		*/
    PyObject *nfields;	/* The number of fields in a tuple.		*/
    PyObject *btuples;	/* Flag: are these binary tuples.		*/
    PyObject *cstatus;	/* The command status.				*/
    PyObject *ctuples;	/* The number of command tuples.		*/
    PyObject *oidval;	/* The OID of the last inserted row.		*/
} PgResult;

extern PyTypeObject PgResult_Type;

extern PyObject *PgResult_New(PGresult *, PgConnection *, int);

extern int PgResult_check(PyObject *);

#define PgResult_Check(op) ((op)->ob_type == &PgResult_Type)

#define PgResult_Get(v) ((v)->res)

/***********************************************************************\
| initpgresult MUST be called in the initialzation code of the module	|
| using the PgResult Object.						|
\***********************************************************************/

extern void initpgresult(void);

#ifdef __cplusplus
}
#endif
#endif /* !Pg_RESULT_H */
