// #ident "@(#) $Id: pgconnection.h,v 1.3 2004/05/10 03:34:03 ballie01 Exp $"
#ifndef Pg_CONNECTION_H
#define Pg_CONNECTION_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************\
| Copyright 2001 by Billy G. Allie										|
| All rights Reserved.													|
|																		|
| Permission to use, copy, modify, and distribute this software and its |
| documentation for any purpose and without fee is hereby granted, pro- |
| vided that the above copyright notice appear in all copies and that	|
| both that copyright notice and this permission notice appear in sup-	|
| porting documentation, and that the copyright owner's name not be		|
| used in advertising or publicity pertaining to distribution of the	|
| software without specific, written prior permission.					|
|																		|
| THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,	|
| INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.	IN	|
| NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR	|
| CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS	|
| OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE |
| OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE	|
| USE OR PERFORMANCE OF THIS SOFTWARE.									|
\***********************************************************************/

/* pyPgSQL connection object interface */

/***********************************************************************\
| PgConnection defines the PostgreSQL PgConnection Object.				|
\***********************************************************************/

/***************************************\
| PgConnection object definition		|
\***************************************/

typedef struct {
	PyObject_HEAD
	PGconn *conn;
	PyObject *host;
	PyObject *port;
	PyObject *db;
	PyObject *options;
	PyObject *tty;
	PyObject *user;
	PyObject *pass;
	PyObject *bePID;
	PyObject *socket;
	PyObject *version;
	PyObject *notices;
	PyObject *cinfo;
	PyObject *debug;

} PgConnection;

extern PyTypeObject PgConnection_Type;

extern PyObject *PgConnection_New(PGconn *);

extern int PgConnection_check(PyObject *);

#define PgConnection_Check(op) ((op)->ob_type == &PgConnection_Type)

#define PgConnection_Get(v) ((v)->conn)

/***********************************************************************\
| initpgconnection MUST be called in the initialzation code of the		|
| module using the PgConnection Object.									|
\***********************************************************************/

extern void initpgconnection(void);

#ifdef __cplusplus
}
#endif
#endif /* !Pg_CONNECTION_H */
