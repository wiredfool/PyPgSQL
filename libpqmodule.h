#ident "@(#) $Id: libpqmodule.h,v 1.11 2005/09/26 08:08:17 ghaering Exp $"
#ifndef Pg_LIBPQMODULE_H
#define Pg_LIBPQMODULE_H
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

#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include "port/port.h"
#include "pg_types.h"
#include "pgboolean.h"
#include "pgint2object.h"
#include "pgint8object.h"
#include "pgconnection.h"
#include "pgresult.h"
#include "pglargeobject.h"
#include "pgnotify.h"
#include "pgversion.h"

#ifdef MS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock.h>
#include <windows.h>
#ifdef __MINGW__
#include "win32.h"
#endif
#endif

#define MODULE_NAME	"libpq"

#if !defined(MAX_BUFFER_SIZE)
#    define MAX_BUFFER_SIZE (8192)	/* Maximum DB tuple size */
#endif

#if !defined(TRUE)
#    define TRUE (1)
#endif

#if !defined(FALSE)
#    define FALSE (0)
#endif

/***********************************************************************\
| The following 'MAGIC' number defines that last reserved OID.		|
\***********************************************************************/

#define MAX_RESERVED_OID	16383	/* 2^14 - 1 */

/***********************************************************************\
| It appears that versions of PostgreSQL prior to version 7.1 do not	|
| define InvalidOid, at least not in an area accessable to external	|
| programs.								|
\***********************************************************************/

#if !defined(InvalidOid)
#    define InvalidOid	((Oid)0)
#endif

/***********************************************************************\
| The following macro definitions (pg_{BEGIN|END}_ALLOW_THREADS) are	|
| used to wrap libpq function calls that can block if the connection is	|
| in blocking mode.  They are based on the Py_{BEGIN|END}_ALLOW_THREADS	|
| macros, which are used to wrap libpq functions that always block.	|
\***********************************************************************/

#if defined(WITH_THREAD)
#   define Pg_BEGIN_ALLOW_THREADS(cnx) \
    { \
	PyThreadState *_save = (PyThreadState *)NULL; \
	if (!PQisnonblocking(cnx)) \
	    _save = PyEval_SaveThread();
#   define Pg_END_ALLOW_THREADS(cnx) \
	if (!PQisnonblocking(cnx)) \
	   PyEval_RestoreThread(_save); \
    }
#else
#   define Pg_BEGIN_ALLOW_THREADS(cnx)
#   define Pg_END_ALLOW_THREADS(cnx)
#endif

/***************************************\
| Exception Objects for this module.	|
\***************************************/
					  /* StandardError		*/
extern PyObject *PqErr_Warning;		  /* |--Warning			*/
extern PyObject *PqErr_Error;		  /* +--Error			*/
extern PyObject *PqErr_InterfaceError;	  /*    |--InterfaceError	*/
extern PyObject *PqErr_DatabaseError;	  /*	+--DatabaseError	*/
extern PyObject *PqErr_DataError;	  /*	   |--DataError		*/
extern PyObject *PqErr_OperationalError;  /*	   |--OperationaError	*/
extern PyObject *PqErr_IntegrityError;	  /*	   |--IntegrityError	*/
extern PyObject *PqErr_InternalError;	  /*	   |--InternalError	*/
extern PyObject *PqErr_ProgrammingError;  /*	   |--ProgrammingError	*/
extern PyObject *PqErr_NotSupportedError; /*	   +--NotSupportedError	*/

void *PyMem_Strdup(void *);

#ifdef __cplusplus
}
#endif
#endif /* !Pg_LIBPQMODULE_H */
