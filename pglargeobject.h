#ident "@(#) $Id: pglargeobject.h,v 1.3 2001/10/13 20:40:35 ballie01 Exp $"
#ifndef Pg_LARGEOBJECT_H
#define Pg_LARGEOBJECT_H
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

/* pyPgSQL large object interface */

/***********************************************************************\
| PgLargeObject defines the PostgreSQL large object (lo) type.		|
\***********************************************************************/

/***************************************\
| PgLargeObject object definition	|
\***************************************/

typedef struct
{
    PyObject_HEAD

    /* These are the public members */
    PyObject	*lo_name;	/* The name for this LO.		*/
    PyObject    *lo_mname;      /* The mode as text ('r', 'w', etc.)    */
    PyObject    *lo_closed;     /* Flag: 0 => opened, 1 => closed.      */
    int		lo_softspace;	/* Softspace flag for this LO.		*/

    /* These are the private members. */   
    PgConnection *lo_conn;	/* Connection object for this large obj.*/
    Oid		lo_oid;		/* The OID of this large object (LO).	*/
    int		lo_fd; 		/* File descriptor for the LO.		*/
    int		lo_mode;	/* Read/write mode for the LO.		*/
    int		lo_dirty;	/* Flag: !0 := buffer written to.	*/
    int		lo_offset;	/* Location in LO for the buffer.	*/
    char	*lo_buf;	/* Buffer for this LO.			*/
    int		lo_size;	/* Bytes read/written into buffer.	*/
    int		lo_idx;		/* Next byte to read from buffer.	*/
    int		need_commit;	/* Flag: !0 := commit needed on close.	*/
} PgLargeObject; 

extern PyTypeObject PgLargeObject_Type;

#define PgLargeObject_Check(op) ((op)->ob_type == &PgLargeObject_Type)

extern PyObject *PgLargeObject_New(PyObject *, Oid, int);

/***********************************************************************\
| initpgversion MUST be called in the initialzation code of the module	|
| using the PgVersion type.						|
\***********************************************************************/

extern void initpglargeobject(void);

#ifdef __cplusplus
}
#endif
#endif /* !Pg_LARGEOBJECT_H */
