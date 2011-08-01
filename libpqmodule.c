#ident "@(#) $Id: libpqmodule.c,v 1.33 2006/06/07 17:21:28 ghaering Exp $"
/* vi:set sw=4 ts=8 showmode ai: */
/**(H+)*****************************************************************\
| Name:		libpqmodule.c						|
|									|
| Description:	This file contains code to allow Python to have access	|
|		to PostgreSQL databases via the libpq functions.  This	|
|		module, is not DB-API 2.0 compliant.  It only exposes	|
|		(most of) the PostgreSQL libpq C API to Python.		|
|=======================================================================|
| Copyright 2000 by Billy G. Allie.					|
| All rights reserved.							|
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
|=======================================================================|
| Revision History:							|
|									|
| Date      Ini Description						|
| --------- --- ------------------------------------------------------- |
| 07JUN2006 gh  Patch for a security hole in PostgreSQL (CVE-2006-2314):|
|               escaping quotes with backslashes is insecure.  The      |
|               change is to escape single quotes with another single   |
|               quote: \' => ''.                                        |
| 08APR2005 bga Un-did one of the fixes put in on 01MAR2005.  It wasn't |
|		broke until I 'fixed' it.				|
| 01MAR2005 bga Fixed most outstanding bug reports and patches.		|
| 09NOV2003 bga Fixed a buffer overrun error in libPQquoteBytea based	|
|		on a fix by James Matthew Farrow. [Bug #838317].	|
| 09NOV2003 bga Fixed a buffer overrun error in libPQquoteBytea based	|
|		on a fix by James Matthew Farrow. [Bug #838317].	|
| 16JUN2003 gh  On win32, we usually statically link against libpq.	|
|		Because of fortunate circumstances, a problem didn't	|
|		show up until now: we need to call WSAStartup() to	|
|		initialize the socket stuff from Windows *in our	|
|		module* in order for the statically linked libpq to	|
|		work. I just took the relevant DllMain function from	|
|		the libpq sources and put it here.			|
| 15JUN2003 bga Modified some comments to reflect reality.		|
| 09JUN2003 gh  Applied patch from Laurent Pinchart: 			|
|		In libPQquoteString, bytea are quoted using as much as	|
|		5 bytes per input byte (0x00 is quoted '\\000'), so	|
|		allocating (slen * 4) + 3 is not enough for data that	|
|		contain lots of 0x00 bytes.				|
| 02JUN2003 bga Added PG_TIMETZ to the mix [Patch #708013].		|
| 28NOV2002 bga Fixed changed PG_TIMESTAMP oid, added PG_TIMESTAMPTZ	|
|		and PG_REFCURSOR oids. [Bug #845360]			|
| 19OCT2002 gh	Fixed the format string of ParseTuple in 		|
|		libPQbool_FromInt. This closes a bug that appeared on	|
|		Linux/Alpha (#625121).					|
| 01OCT2002 gh  HAVE_LONG_LONG => HAVE_LONG_LONG_SUPPORT		|
| 21APR2002 gh  Removed special escaping of control characters in	|
|		arrays.							|
| 03FEB2002 bga Added code to support the PG_ACLTIEM and PG_MACADDR oid	|
|		numbers [Bug #510244].					|
| 21JAN2002 bga Applied patch by Chris Bainbridge [Patch #505941].	|
| --------- bga Remove prior comments to reduce the size of the flower	|
|		box.  See revision 1.25 for earlier comments.		|
\*(H-)******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <Python.h>
#include <structmember.h>
#include <fileobject.h>
#include "libpqmodule.h"

/***********************************************************************\
| Define a value for the psuedo types, PG_ROWID and PG_BLOB.  It should	|
| not be a valid type OID.						|
\***********************************************************************/

#define PG_ROWID (1)
#define PG_BLOB	(2)

/***************************************\
| Exception Objects for this module.	|
\***************************************/
					  /* StandardError		*/
PyObject *PqErr_Warning;		  /* |--Warning			*/
PyObject *PqErr_Error;		  	  /* +--Error			*/
PyObject *PqErr_InterfaceError;		  /*    |--InterfaceError	*/
PyObject *PqErr_DatabaseError;		  /*	+--DatabaseError	*/
PyObject *PqErr_DataError;		  /*	   |--DataError		*/
PyObject *PqErr_OperationalError;	  /*	   |--OperationaError	*/
PyObject *PqErr_IntegrityError;		  /*	   |--IntegrityError	*/
PyObject *PqErr_InternalError;		  /*	   |--InternalError	*/
PyObject *PqErr_ProgrammingError;	  /*	   |--ProgrammingError	*/
PyObject *PqErr_NotSupportedError;	  /*	   +--NotSupportedError	*/

/***********************************************************************\
| Define the libq module functions.					|
\***********************************************************************/

#define DIG(VAL) ((VAL) + '0')
#define VAL(CH)  ((CH) - '0')

/*--------------------------------------------------------------------------*/

static char libPQquoteString_Doc[] =
    "PgQuoteString(string) -> string\n"
    "    This is a helper function that will quote a Python String in a "
    "manner\n    that is acceptable to PostgreSQL";

/***********************************************************************\
| The following routine will quote a string in a manner acceptable for	|
| use in PostgreSQL.  The quoting rules are:				|
| 									|
| 1.  Control characters are quoted as follows:				|
|									|
|	Value	Quoted Representation					|
|       -----	---------------------					|
|	 NUL	* NOT ALLOWED *						|
|	  BS	\b							|
|	 TAB	\t							|
|	  NL	\n							|
|	  CR	\r							|
|	other	\OOO where OOO is the octal representation of the con-	|
|		trol character's ordinal number.			|
|									|
| 2.  The backslash is quoted as \\.					|
|									|
| 3.  The single quote is quoted as ''.					|
|									|
| 4.  All other characters are unchanged.				|
|									|
| Note: If the optional forArray argument is 1, then the escaping is	|
|	changed as follows:						|
|       1.  Control characters are quoted as follows:			|
|									|
|	    Value	Quoted Representation				|
|           -----	---------------------				|
|	     NUL	* NOT ALLOWED *					|
|	      BS	\\b						|
|	     TAB	\\t						|
|	      NL	\\n						|
|	      CR	\\r						|
|	   other	\OOO where OOO is the octal representation of	|
|			     the control character's ordinal number.	|
|									|
|	2. The backslash is escpaed as \\\\.				|
|	3. The single quote is escaped as ''.				|
|	4. The double quote is escaped as \\".				|
|	5. All other characters are unchanged.				|
\***********************************************************************/

static PyObject *libPQquoteString(PyObject *self, PyObject *args)
{
    int i, j, slen, byte, forArray=0;
    unsigned char *sin;
    unsigned char *sout;
    PyObject *result;

    if (!PyArg_ParseTuple(args,"s|i:PgQuoteString", &sin, &forArray)) 
        return NULL;

    /* Calculate the size for the new memory string. */
    slen = strlen((char *)sin);
    i = (slen * (forArray ? 7 : 4)) + 3;

    sout = (unsigned char *)PyMem_Malloc(i); /* Assume every char is quoted */
    if (sout == (unsigned char *)NULL)
	return PyErr_NoMemory();
    
    sout[0] = (forArray ? '"' : '\'');

    for (i = 0, j = 1; i < slen; i++)
    {
	switch (sin[i])
	{
	    case '"':
		if (forArray)
		{
		    sout[j++] = '\\';
		    sout[j++] = '\\';
		}
		sout[j++] = sin[i];
		break;

	    case '\'':
		sout[j++] = '\'';
		sout[j++] = sin[i];
		break;

	    case '\\':
		sout[j++] = sin[i];
		sout[j++] = sin[i];
		if (forArray)
		{
		    sout[j++] = sin[i];
		    sout[j++] = sin[i];
		}
		break;

	    case '\b':
		sout[j++] = '\\';
		sout[j++] = 'b';
		break;

	    case '\f':
		sout[j++] = '\\';
		sout[j++] = 'f';
		break;

	    case '\n':
		sout[j++] = '\\';
		sout[j++] = 'n';
		break;

	    case '\r':
		sout[j++] = '\\';
		sout[j++] = 'r';
		break;

	    case '\t':
		sout[j++] = '\\';
		sout[j++] = 't';
		break;

	    default:
		if (sin[i] < 32)
		{
		    /* escape any control character not already escaped. */
		    byte = (unsigned char)sin[i];
		    sout[j++] = '\\';
		    sout[j++] = DIG((byte >> 6) & 3);
		    sout[j++] = DIG((byte >> 3) & 7);
		    sout[j++] = DIG(byte & 7);
		}
		else
		    sout[j++] = sin[i];
	}
    }

    sout[j++] = (forArray ? '"' : '\'');
    sout[j] = (char)0;

    result = Py_BuildValue("s#", sout, j);
    PyMem_Free(sout);

    return result;
}

/*--------------------------------------------------------------------------*/

static char libPQquoteBytea_Doc[] =
    "PgQuoteString(string, forArray) -> string\n"
    "    This is a helper function that will quote a Python String (that can "
    "contain embedded NUL bytes) in a manner\n    that is acceptable to "
    "PostgreSQL";

/***********************************************************************\
| The following routine will quote a bytea string in a manner accept-	|
| able for use in PostgreSQL.  The quoting rules are:			|
| 									|
| 1.  The NUL character is escaped as \\000.				|
|									|
| 2.  Non-printable characters are escaped as \OOO where OOO is the	|
|     octal representation of the characters ordinal number.		|
|									|
| 3.  The backslash is escaped as \\\\.					|
|									|
| 4.  The single quote is escaped as ''.				|
|									|
| 5.  All other characters are unchanged.				|
|									|
| Note: If the optional forArray argument is 1, then the escaping is	|
|	changed as follows:						|
|	1. The NUL character is escaped as \\\\000.			|
|	2. Non-printable characters are escapes as \\\\OOO.		|
|	3. The backslash is escpaed as \\\\\\\\.			|
|	4. The single quote is escaped as ''.				|
|	5. The double quote is escaped as \\".				|
|	6. All other characters are unchanged.				|
\***********************************************************************/

static PyObject *libPQquoteBytea(PyObject *self, PyObject *args)
{
    int i, j, slen, byte, forArray = 0;
    unsigned char *sin;
    unsigned char *sout;
    PyObject *result;

    if (!PyArg_ParseTuple(args,"s#|i:PgQuoteBytea", &sin, &slen, &forArray)) 
        return NULL;

    /* Calculate the size for the new memory string. */
    i = (slen * (forArray ? 8 : 5)) + 3;

    sout = (unsigned char *)PyMem_Malloc(i); /* Assume every char is quoted */
    if (sout == (unsigned char *)NULL)
	return PyErr_NoMemory();
    
    sout[0] = (forArray ? '"' : '\'');

    for (i = 0, j = 1; i < slen; i++)
    {
	switch (sin[i])
	{
	    case '"':
		if (forArray)
		{
		    sout[j++] = '\\';
		    sout[j++] = '\\';
		}
		sout[j++] = sin[i];
		break;

	    case '\'':
		sout[j++] = '\'';
		sout[j++] = sin[i];
		break;

	    case '\\':
		sout[j++] = sin[i];
		sout[j++] = sin[i];
		sout[j++] = sin[i];
		sout[j++] = sin[i];
		if (forArray)
		{
		    sout[j++] = sin[i];
		    sout[j++] = sin[i];
		    sout[j++] = sin[i];
		    sout[j++] = sin[i];
		}
		break;

	    case '\0':
		sout[j++] = '\\';
		sout[j++] = '\\';
		if (forArray)
		{
		    sout[j++] = '\\';
		    sout[j++] = '\\';
		}
		sout[j++] = '0';
		sout[j++] = '0';
		sout[j++] = '0';
		break;

	    default:
		if (!isprint(sin[i]))
		{
		    /* escape any control character not already escaped. */
		    byte = (unsigned char)sin[i];
		    sout[j++] = '\\';
		    if (forArray)
		    {
			sout[j++] = '\\';
			sout[j++] = '\\';
			sout[j++] = '\\';
		    }
		    sout[j++] = DIG((byte >> 6) & 3);
		    sout[j++] = DIG((byte >> 3) & 7);
		    sout[j++] = DIG(byte & 7);
		}
		else
		    sout[j++] = sin[i];
	}
    }

    sout[j++] = (forArray ? '"' : '\'');

    result = Py_BuildValue("s#", sout, j);
    PyMem_Free(sout);

    return result;
}

/*--------------------------------------------------------------------------*/

PyObject *unQuoteBytea(char *sin)
{
    int i, j, slen, byte;
    char *sout;
    PyObject *result;

    slen = strlen(sin);
    sout = (char *)PyMem_Malloc(slen);
    if (sout == (char *)NULL)
	return PyErr_NoMemory();
    
    for (i = j = 0; i < slen;)
    {
	switch (sin[i])
	{
	    case '\\':
		i++;
		if (sin[i] == '\\')
		    sout[j++] = sin[i++];
		else
		{
		    if ((!isdigit(sin[i]))   ||
			(!isdigit(sin[i+1])) ||
			(!isdigit(sin[i+2])))
			goto unquote_error;

		    byte = VAL(sin[i++]);
		    byte = (byte << 3) + VAL(sin[i++]);
		    sout[j++] = (byte << 3) + VAL(sin[i++]);
		}
		break;

	    default:
		sout[j++] = sin[i++];
	}
    }

    result = Py_BuildValue("s#", sout, j);
    PyMem_Free(sout);

    return result;

unquote_error:
    PyMem_Free(sout);
    PyErr_SetString(PyExc_ValueError, "Bad input string for type bytea");
    return (PyObject *)NULL;
}

static char libPQunQuoteBytea_Doc[] =
    "PgUnQuoteString(string) -> string\n"
    "    This is a helper function that will un-quote a string that is the "
    "returned\n    value from a bytea field.  The returned Python string may "
    "contain embedded\n    NUL characters.";

static PyObject *libPQunQuoteBytea(PyObject *self, PyObject *args)
{
    char *sin;

    if (!PyArg_ParseTuple(args,"s:PgUnQuoteString", &sin)) 
        return NULL;

    return unQuoteBytea(sin);
}

/*--------------------------------------------------------------------------*/

static char libPQconndefaults_Doc[] =
    "PQconndefaults() -> [[<option info>], ...]\n"
    "    Returns a list of default connection options.  Each item in the list\n"
    "    contains [keyword, envvar, compiled, val, label, dispchar, "
    "dispsize].";

static PyObject *libPQconndefaults(PyObject *self, PyObject *args)
{
    PQconninfoOption *opt;
    PyObject *list = (PyObject *)NULL, *item = (PyObject *)NULL;

    if (!PyArg_ParseTuple(args, "")) 
    {
	PyErr_SetString(PqErr_InterfaceError,
			"PQconndefaults() takes no parameters");
        return NULL;
    }

    opt = PQconndefaults();

    if ((list = PyList_New(0)) == (PyObject *)NULL) goto errorExit;

    while ((opt != NULL) && (opt->keyword != (char *)NULL))
    {
	item = Py_BuildValue("[ssssssi]", opt->keyword, opt->envvar,
					  opt->compiled, opt->val, opt->label,
					  opt->dispchar, opt->dispsize);
	if (!item) goto errorExit;

	if (PyList_Append(list, item)) goto errorExit;

	opt++;
    }

    return list;

errorExit:
    Py_XDECREF(item);
    Py_XDECREF(list);
    return (PyObject *)NULL;
}

/*--------------------------------------------------------------------------*/

static char libPQconnectdb_Doc[] =
    "PQconnectdb(conninfo) -> PgConnection\n"
    "    Connect to a PostgreSQL database.";

static PyObject *libPQconnectdb(PyObject *self, PyObject *args)
{
    PyObject    *conn;
    PGconn	*cnx;
    char	*conninfo;

    if (!PyArg_ParseTuple(args,"s:PQconnectdb",&conninfo)) 
        return (PyObject *)NULL;

    Py_BEGIN_ALLOW_THREADS
    cnx = PQconnectdb(conninfo);
    Py_END_ALLOW_THREADS

    if (cnx  == (PGconn *)NULL)
    {
	PyErr_SetString(PyExc_MemoryError,
		       "Can't allocate new PGconn structure in PQconnectdb.");
	return (PyObject *)NULL;
    }

    if (PQstatus(cnx) != CONNECTION_OK)
    {
	PyErr_SetString(PqErr_DatabaseError, PQerrorMessage(cnx));
	PQfinish(cnx);
	return (PyObject *)NULL;
    }

    conn = PgConnection_New(cnx);
    /* Save the connection info string (used for pickling). */
    if (conn != (PyObject *)NULL)
	((PgConnection *)conn)->cinfo = Py_BuildValue("s", conninfo);
    return conn;
}

/*--------------------------------------------------------------------------*/

#if defined(DO_NOT_DEFINE_THIS_MACRO)
static char libPQsetdbLogin_Doc[] =
    "PGsetdbLogin(pghost, pgport, pgopt, pgtty, dbname, login, passwd) ->\n"
    "		PgConnection\n"
    "    Connect to a PostgreSQL database.";

static PyObject *libPQsetdbLogin(PyObject *self, PyObject *args)
{
    PGconn	*cnx;
    char	*pghost = "";
    char	*pgport = "";
    char	*pgoptions = "";
    char	*pgtty = "";
    char	*dbname = "";
    char	*login = "";
    char	*passwd = "";

    if (!PyArg_ParseTuple(args,"sssssss:PQsetdbLogin", &pghost, &pgport,
			  &pgoptions, &pgtty, &dbname, &login, &passwd)) 
        return (PyObject *)NULL;

    Py_BEGIN_ALLOW_THREADS
    cnx = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbname, login,
			passwd);
    Py_END_ALLOW_THREADS

    if (cnx == (PGconn *)NULL)
    {
	PyErr_SetString(PyExc_MemoryError,
		       "Can't allocate new PGconn structure in PQsetdbLogin.");
	return (PyObject *)NULL;
    }

    if (PQstatus(cnx) != CONNECTION_OK)
    {
	PyErr_SetString(PqErr_DatabaseError, PQerrorMessage(cnx));
	PQfinish(cnx);
	return (PyObject *)NULL;
    }

    return PgConnection_New(cnx);
}

/*--------------------------------------------------------------------------*/

static char libPQconnectStart_Doc[] =
    "PQconnectStart(conninfo) -> PgConnection\n"
    "    Connect to a PostgreSQL database in a non-blocking manner.";

static PyObject *libPQconnectStart(PyObject *self, PyObject *args)
{
    PGconn	*cnx;
    char	*conninfo;

    if (!PyArg_ParseTuple(args,"s:PQconnectStart",&conninfo)) 
        return (PyObject *)NULL;

    if ((cnx = PQconnectStart(conninfo)) == (PGconn *)NULL)
    {
	PyErr_SetString(PyExc_MemoryError,
		   "Can't allocate new PGconn structure in PQconnectStart.");
	return (PyObject *)NULL;
    }

    if (PQstatus(cnx) == CONNECTION_BAD)
    {
	PyErr_SetString(PqErr_DatabaseError, PQerrorMessage(cnx));
	PQfinish(cnx);
	return (PyObject *)NULL;
    }

    conn = PgConnection_New(cnx);
    /* Save the connection info string (used for pickling). */
    if (conn)
	((PgConnection *)conn)->cinfo = Py_BuildValue("s", conninfo);
    return conn;
}
#endif

/*--------------------------------------------------------------------------*/

static char libPQresStatus_Doc[] =
    "PQresStatus(status)\n"
    "    Returns a string describing the result status code 'status'.";

static PyObject *libPQresStatus(PyObject *self, PyObject *args)
{
    int		resultStatus;

    if (!PyArg_ParseTuple(args,"i:PQresStatus", &resultStatus)) 
        return NULL;

    return Py_BuildValue("s", PQresStatus(resultStatus));
}

/*--------------------------------------------------------------------------*/

static char libPQresType_Doc[] =
    "PQresType(type)\n"
    "    Returns a string describing the result type code 'type'.";

static PyObject *libPQresType(PyObject *self, PyObject *args)
{
    char *desc;
    int	 resultType;

    if (!PyArg_ParseTuple(args,"i:PQresType", &resultType)) 
        return NULL;

    switch (resultType)
    {
	case RESULT_ERROR:
	    desc = "RESULT_ERROR";
	    break;

	case RESULT_EMPTY:
	    desc = "RESULT_EMPTY";
	    break;

	case RESULT_DQL:
	    desc = "RESULT_DQL";
	    break;

	case RESULT_DDL:
	    desc = "RESULT_DDL";
	    break;

	case RESULT_DML:
	    desc = "RESULT_DML";
	    break;

	default:
	    PyErr_SetString(PqErr_InterfaceError,
			    "Unknown result type.");
	    return NULL;
    }

    return Py_BuildValue("s", desc);
}

/*--------------------------------------------------------------------------*/

static char libPQftypeName_Doc[] =
    "PQftypeName(type)\n"
    "    Returns a string describing the field type code 'type'.";

static PyObject *libPQftypeName(PyObject *self, PyObject *args)
{
    char *desc;
    int	 fieldType;

    if (!PyArg_ParseTuple(args,"i:PQftypeName", &fieldType)) 
        return NULL;

    switch (fieldType)
    {
	case PG_ABSTIME:	desc = "abstime";	break;
	case PG_ACLITEM:	desc = "aclitem";	break;
	case PG_BLOB:   	desc = "blob";		break;
	case PG_BOOL:		desc = "bool";		break;
	case PG_BOX:		desc = "box";		break;
	case PG_BPCHAR:		desc = "char";		break;
	case PG_BYTEA:		desc = "bytea";		break;
	case PG_CASH:		desc = "money";		break;
	case PG_CHAR:		desc = "char";		break;
	case PG_CID:		desc = "cid";		break;
	case PG_CIDR:		desc = "cidr";		break;
	case PG_CIRCLE:		desc = "circle";	break;
	case PG_DATE:		desc = "date";		break;
	case PG_FLOAT4:		desc = "float4";	break;
	case PG_FLOAT8:		desc = "float";		break;
	case PG_INET:		desc = "inet";		break;
	case PG_INT2:		desc = "int2";		break;
	case PG_INT2VECTOR:	desc = "int2vector";	break;
	case PG_INT4:		desc = "integer";	break;
	case PG_INT8:		desc = "bigint";	break;
	case PG_INTERVAL:	desc = "interval";	break;
	case PG_LINE:		desc = "line"	;	break;
	case PG_LSEG:		desc = "lseg";		break;
	case PG_MACADDR:	desc = "macaddr";	break;
	case PG_NAME:		desc = "name";		break;
	case PG_NUMERIC:	desc = "numeric";	break;
	case PG_OID:		desc = "oid";		break;
	case PG_OIDVECTOR:	desc = "oidvector";	break;
	case PG_PATH:		desc = "path";		break;
	case PG_POINT:		desc = "point";		break;
	case PG_POLYGON:	desc = "polygon";	break;
	case PG_REFCURSOR:	desc = "refcursor";	break;
	case PG_REGPROC:	desc = "regproc";	break;
	case PG_RELTIME:	desc = "reltime";	break;
	case PG_ROWID:		desc = "rowid";		break;
	case PG_TEXT:		desc = "text";		break;
	case PG_TID:		desc = "tid";		break;
	case PG_TIME:		desc = "time";		break;
	case PG_TIMETZ:		desc = "timetz";	break;
	case PG_TIMESTAMP:	desc = "timestamp";	break;
	case PG_TIMESTAMPTZ:	desc = "timestamptz";	break;
	case PG_TINTERVAL:	desc = "tinterval";	break;
	case PG_UNKNOWN:	desc = "unknown";	break;
	case PG_VARBIT:		desc = "varbit";	break;
	case PG_VARCHAR:	desc = "varchar";	break;
	case PG_XID:		desc = "xid";		break;
	case PG_ZPBIT:		desc = "zpbit";		break;
	default:		desc = (char *)NULL;
    }

    return Py_BuildValue("s", desc);
}

/*--------------------------------------------------------------------------*/

#if defined(HAVE_LONG_LONG_SUPPORT)
static PyObject *libPQint8_FromObject(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyNumberMethods *nb;

    if (!PyArg_ParseTuple(args,"O:PgInt8", &obj)) 
        return (PyObject *)NULL;

    if (PgInt2_Check(obj))
    {
        return PgInt8_FromLong((long)PgInt2_AS_INT2(obj));
    }
    else if (PyInt_Check(obj))
    {
        return PgInt8_FromLong(PyInt_AS_LONG(obj));
    }
    else if (PyLong_Check(obj))
    {
        LONG_LONG a = PyLong_AsLongLong(obj);

        if (a == -1 && PyErr_Occurred())
            return (PyObject *)NULL;

        return PgInt8_FromLongLong(a);
    }
    else if (PyFloat_Check(obj))
    {
        PyObject *o;
	LONG_LONG a;

        /* There is no PyFloat_AsLongLong() function, so convert the float
         * into a Python Long then convert the Long into a long long
         */

        nb = obj->ob_type->tp_as_number;
        o = (PyObject *)(*nb->nb_long)(obj);
        a = PyLong_AsLongLong(o);

        if (a == -1 && PyErr_Occurred())
            return (PyObject *)NULL;

        return PgInt8_FromLongLong(a);
    }
    else if (PyString_Check(obj))
    {
        char *s = PyString_AsString(obj);
        
        if (s == (char *)NULL)
            return (PyObject *)PyErr_NoMemory();

        return PgInt8_FromString(s, (char **)NULL, 10);
    }

    PyErr_SetString(PyExc_TypeError, "a string or numeric is required");
    return (PyObject *)NULL;
}
#endif

/*--------------------------------------------------------------------------*/

static PyObject *libPQint2_FromObject(PyObject *self, PyObject *args)
{
    PyObject *obj;

    if (!PyArg_ParseTuple(args,"O:PgInt2", &obj)) 
        return (PyObject *)NULL;

    if (PyInt_Check(obj))
    {
        return PgInt2_FromLong(PyInt_AS_LONG(obj));
    }
#if defined(HAVE_LONG_LONG_SUPPORT)
    else if (PgInt8_Check(obj))
    {
        long a = PyLong_AsLong(obj);

        if (a == -1 && PyErr_Occurred())
            return (PyObject *)NULL;

        return PgInt2_FromLong(a);
    }
#endif
    else if (PyLong_Check(obj))
    {
        long a = PyLong_AsLong(obj);

        if (a == -1 && PyErr_Occurred())
            return (PyObject *)NULL;

        return PgInt2_FromLong(a);
    }
    else if (PyFloat_Check(obj))
    {
        long a;
        LONG_LONG f;

        a = (long)(f = (LONG_LONG)PyFloat_AsDouble(obj));

        if ((LONG_LONG)a != f)
        {
            PyErr_SetString(PyExc_OverflowError,
                            "number to large to convert to PgInt2");
            return (PyObject *)NULL;
        }

        return PgInt2_FromLong(a);
    }
    else if (PyString_Check(obj))
    {
        char *s = PyString_AsString(obj);
        
        if (s == (char *)NULL)
            return PyErr_NoMemory();

        return PgInt2_FromString(s, (char **)NULL, 10);
    }

    PyErr_SetString(PyExc_TypeError, "a string or numeric is required");
    return (PyObject *)NULL;
}

/*--------------------------------------------------------------------------*/

static PyObject *libPQbool_FromString(PyObject *self, PyObject *args)
{
    char *s;

    if (!PyArg_ParseTuple(args,"s:BooleanFromString", &s)) 
        return NULL;

    return PgBoolean_FromString(s);
}

/*--------------------------------------------------------------------------*/

static PyObject *libPQbool_FromInt(PyObject *self, PyObject *args)
{
    long l;

    if (!PyArg_ParseTuple(args, "l:BooleanFromInteger", &l)) 
        return NULL;

    return PgBoolean_FromLong(l);
}

/*--------------------------------------------------------------------------*/

static PyObject *libPQbool_FromObject(PyObject *self, PyObject *args)
{
    PyObject *obj;

    if (!PyArg_ParseTuple(args,"O:PgBoolean", &obj)) 
        return NULL;

    if (PyInt_Check(obj))
    {
        return PgBoolean_FromLong(PyInt_AS_LONG(obj));
    }
    else if (PyLong_Check(obj) || PyFloat_Check(obj))
    {
        long i = (*(obj->ob_type->tp_as_number)->nb_nonzero)(obj);

        return PgBoolean_FromLong(i);
    }
    else if (PyString_Check(obj))
    {
        return libPQbool_FromString(self, args);
    }

    PyErr_SetString(PyExc_TypeError, "a string or numeric is required");
    return (PyObject *)NULL;
}

/*--------------------------------------------------------------------------*/

static PyObject *libPQlargeObject_New(PyObject *self, PyObject *args)
{
    PyObject *conn;
    long int loOid;

    if (!PyArg_ParseTuple(args, "O!l:PgLargeObject",
				&PgConnection_Type, &conn, &loOid))
        return NULL;

    return PgLargeObject_New(conn, loOid, 1);
}

/*--------------------------------------------------------------------------*/

static PyObject *libPQversion_New(PyObject *self, PyObject *args)
{
    char* v;

    if (!PyArg_ParseTuple(args, "s:PgVersion", &v))
        return NULL;

    return PgVersion_New(v);
}

/***********************************************************************\
| Define the libpq methods and attributes.				|
\***********************************************************************/

static PyMethodDef libpqMethods[] = {
    { "PQresStatus",  (PyCFunction)libPQresStatus,  1, libPQresStatus_Doc },
    { "PQresType",    (PyCFunction)libPQresType,    1, libPQresType_Doc },
    { "PQftypeName",  (PyCFunction)libPQftypeName,  1, libPQftypeName_Doc },
    { "PQconnectdb",  (PyCFunction)libPQconnectdb,  1, libPQconnectdb_Doc },
#if defined(DO_NOT_DEFINE_THIS_MACRO)
    { "PQsetdbLogin", (PyCFunction)libPQsetdbLogin, 1, libPQsetdbLogin_Doc },
    { "PQconnectStart",  (PyCFunction)libPQconnectStart,  1,
						    libPQconnectStart_Doc },
#endif
    { "PQconndefaults",  (PyCFunction)libPQconndefaults,  1,
						    libPQconndefaults_Doc },
    { "PgQuoteString", (PyCFunction)libPQquoteString, 1, libPQquoteString_Doc },
    { "PgQuoteBytea", (PyCFunction)libPQquoteBytea, 1, libPQquoteBytea_Doc },
    { "PgUnQuoteBytea", (PyCFunction)libPQunQuoteBytea, 1,
						    libPQunQuoteBytea_Doc },
    { "PgBooleanFromString", (PyCFunction)libPQbool_FromString, 1 },
    { "PgBooleanFromInteger", (PyCFunction)libPQbool_FromInt, 1 },
    { "PgBoolean", (PyCFunction)libPQbool_FromObject, 1 },
#if defined(HAVE_LONG_LONG_SUPPORT)
    { "PgInt8", (PyCFunction)libPQint8_FromObject, 1 },
#endif
    { "PgInt2", (PyCFunction)libPQint2_FromObject, 1 },
    { "PgLargeObject", (PyCFunction)libPQlargeObject_New, 1 },
    { "PgVersion", (PyCFunction)libPQversion_New, 1 },
    { NULL, NULL }
};

#ifdef MS_WIN32
BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
        LPVOID lpReserved)
{
    WSADATA wsaData;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (WSAStartup(MAKEWORD(1, 1), &wsaData))
            {
                /*
                 * No really good way to do error handling here, since we
                 * don't know how we were loaded
                 */
                return FALSE;
            }
            break;
        case DLL_PROCESS_DETACH:
            WSACleanup();
            break;
    }

    return TRUE;
}
#endif

DL_EXPORT(void) initlibpq(void)
{
    PyObject *m, *d;

    m = Py_InitModule("libpq", libpqMethods);
    d = PyModule_GetDict(m);

    /*-----------------------------------------------------------------------*/

    PgConnection_Type.ob_type	= &PyType_Type;
    PgResult_Type.ob_type	= &PyType_Type;

    initpgconnection();
    initpgresult();
    initpglargeobject();
    initpgnotify();
    initpgboolean();
    initpgint2();
#if defined(HAVE_LONG_LONG_SUPPORT)
    initpgint8();
#endif
    initpgversion();

    /*-----------------------------------------------------------------------*/

    PyDict_SetItemString(d, "__version__", Py_BuildValue("s", "$Revision: 1.33 $"));

    /*-----------------------------------------------------------------------*/

    PyDict_SetItemString(d, "CONNECTION_OK",
			 Py_BuildValue("i", CONNECTION_OK));
    PyDict_SetItemString(d, "CONNECTION_BAD",
				Py_BuildValue("i", CONNECTION_BAD));

    /*-----------------------------------------------------------------------*/
    PyDict_SetItemString(d, "POLLING_FAILED",
				Py_BuildValue("i", PGRES_POLLING_FAILED));
    PyDict_SetItemString(d, "POLLING_READING",
				Py_BuildValue("i", PGRES_POLLING_READING));
    PyDict_SetItemString(d, "POLLING_WRITING",
				Py_BuildValue("i", PGRES_POLLING_WRITING));
    PyDict_SetItemString(d, "POLLING_OK",
			 Py_BuildValue("i", PGRES_POLLING_OK));
    PyDict_SetItemString(d, "POLLING_ACTIVE",
				Py_BuildValue("i", PGRES_POLLING_ACTIVE));
    PyDict_SetItemString(d, "EMPTY_QUERY",
				Py_BuildValue("i", PGRES_EMPTY_QUERY));
    PyDict_SetItemString(d, "COMMAND_OK",
			 Py_BuildValue("i", PGRES_COMMAND_OK));
    PyDict_SetItemString(d, "TUPLES_OK", Py_BuildValue("i", PGRES_TUPLES_OK));
    PyDict_SetItemString(d, "COPY_OUT", Py_BuildValue("i", PGRES_COPY_OUT));
    PyDict_SetItemString(d, "COPY_IN", Py_BuildValue("i", PGRES_COPY_IN));
    PyDict_SetItemString(d, "BAD_RESPONSE",
				Py_BuildValue("i", PGRES_BAD_RESPONSE));
    PyDict_SetItemString(d, "NONFATAL_ERROR",
				Py_BuildValue("i", PGRES_NONFATAL_ERROR));
    PyDict_SetItemString(d, "FATAL_ERROR", 
				Py_BuildValue("i", PGRES_FATAL_ERROR));

    /*-----------------------------------------------------------------------*/

    PyDict_SetItemString(d, "PG_ABSTIME", Py_BuildValue("i", PG_ABSTIME));
    PyDict_SetItemString(d, "PG_ACLITEM", Py_BuildValue("i", PG_ACLITEM));
    PyDict_SetItemString(d, "PG_BIGINT", Py_BuildValue("i", PG_INT8));
    PyDict_SetItemString(d, "PG_BLOB", Py_BuildValue("i", PG_BLOB));
    PyDict_SetItemString(d, "PG_BOOL", Py_BuildValue("i", PG_BOOL));
    PyDict_SetItemString(d, "PG_BOX", Py_BuildValue("i", PG_BOX));
    PyDict_SetItemString(d, "PG_BPCHAR", Py_BuildValue("i", PG_BPCHAR));
    PyDict_SetItemString(d, "PG_BYTEA", Py_BuildValue("i", PG_BYTEA));
    PyDict_SetItemString(d, "PG_CASH", Py_BuildValue("i", PG_CASH));
    PyDict_SetItemString(d, "PG_CHAR", Py_BuildValue("i", PG_CHAR));
    PyDict_SetItemString(d, "PG_CID", Py_BuildValue("i", PG_CID));
    PyDict_SetItemString(d, "PG_CIDR", Py_BuildValue("i", PG_CIDR));
    PyDict_SetItemString(d, "PG_CIRCLE", Py_BuildValue("i", PG_CIRCLE));
    PyDict_SetItemString(d, "PG_DATE", Py_BuildValue("i", PG_DATE));
    PyDict_SetItemString(d, "PG_FLOAT", Py_BuildValue("i", PG_FLOAT8));
    PyDict_SetItemString(d, "PG_FLOAT4", Py_BuildValue("i", PG_FLOAT4));
    PyDict_SetItemString(d, "PG_FLOAT8", Py_BuildValue("i", PG_FLOAT8));
    PyDict_SetItemString(d, "PG_INET", Py_BuildValue("i", PG_INET));
    PyDict_SetItemString(d, "PG_INT2", Py_BuildValue("i", PG_INT2));
    PyDict_SetItemString(d, "PG_INT2VECTOR", Py_BuildValue("i", PG_INT2VECTOR));
    PyDict_SetItemString(d, "PG_INT4", Py_BuildValue("i", PG_INT4));
    PyDict_SetItemString(d, "PG_INT8", Py_BuildValue("i", PG_INT8));
    PyDict_SetItemString(d, "PG_INTEGER", Py_BuildValue("i", PG_INT4));
    PyDict_SetItemString(d, "PG_INTERVAL", Py_BuildValue("i", PG_INTERVAL));
    PyDict_SetItemString(d, "PG_LINE", Py_BuildValue("i", PG_LINE));
    PyDict_SetItemString(d, "PG_LSEG", Py_BuildValue("i", PG_LSEG));
    PyDict_SetItemString(d, "PG_MACADDR", Py_BuildValue("i", PG_MACADDR));
    PyDict_SetItemString(d, "PG_MONEY", Py_BuildValue("i", PG_CASH));
    PyDict_SetItemString(d, "PG_NAME", Py_BuildValue("i", PG_NAME));
    PyDict_SetItemString(d, "PG_NUMERIC", Py_BuildValue("i", PG_NUMERIC));
    PyDict_SetItemString(d, "PG_OID", Py_BuildValue("i", PG_OID));
    PyDict_SetItemString(d, "PG_OIDVECTOR", Py_BuildValue("i", PG_OIDVECTOR));
    PyDict_SetItemString(d, "PG_PATH", Py_BuildValue("i", PG_PATH));
    PyDict_SetItemString(d, "PG_POINT", Py_BuildValue("i", PG_POINT));
    PyDict_SetItemString(d, "PG_POLYGON", Py_BuildValue("i", PG_POLYGON));
    PyDict_SetItemString(d, "PG_REFCURSOR", Py_BuildValue("i", PG_REFCURSOR));
    PyDict_SetItemString(d, "PG_REGPROC", Py_BuildValue("i", PG_REGPROC));
    PyDict_SetItemString(d, "PG_RELTIME", Py_BuildValue("i", PG_RELTIME));
    PyDict_SetItemString(d, "PG_ROWID", Py_BuildValue("i", PG_ROWID));
    PyDict_SetItemString(d, "PG_SMALLINT", Py_BuildValue("i", PG_INT2));
    PyDict_SetItemString(d, "PG_TEXT", Py_BuildValue("i", PG_TEXT));
    PyDict_SetItemString(d, "PG_TID", Py_BuildValue("i", PG_TID));
    PyDict_SetItemString(d, "PG_TIME", Py_BuildValue("i", PG_TIME));
    PyDict_SetItemString(d, "PG_TIMETZ", Py_BuildValue("i", PG_TIMETZ));
    PyDict_SetItemString(d, "PG_TIMESTAMP", Py_BuildValue("i", PG_TIMESTAMP));
    PyDict_SetItemString(d, "PG_TIMESTAMPTZ", Py_BuildValue("i", PG_TIMESTAMPTZ));
    PyDict_SetItemString(d, "PG_TINTERVAL", Py_BuildValue("i", PG_TINTERVAL));
    PyDict_SetItemString(d, "PG_UNKNOWN", Py_BuildValue("i", PG_UNKNOWN));
    PyDict_SetItemString(d, "PG_VARBIT", Py_BuildValue("i", PG_VARBIT));
    PyDict_SetItemString(d, "PG_VARCHAR", Py_BuildValue("i", PG_VARCHAR));
    PyDict_SetItemString(d, "PG_XID", Py_BuildValue("i", PG_XID));
    PyDict_SetItemString(d, "PG_ZPBIT", Py_BuildValue("i", PG_ZPBIT));

    /*-----------------------------------------------------------------------*/

    PyDict_SetItemString(d, "RESULT_DDL", Py_BuildValue("i", RESULT_DDL));
    PyDict_SetItemString(d, "RESULT_DQL", Py_BuildValue("i", RESULT_DQL));
    PyDict_SetItemString(d, "RESULT_DML", Py_BuildValue("i", RESULT_DML));
    PyDict_SetItemString(d, "RESULT_EMPTY", Py_BuildValue("i", RESULT_EMPTY));
    PyDict_SetItemString(d, "RESULT_ERROR", Py_BuildValue("i", RESULT_ERROR));

    /*-----------------------------------------------------------------------*/

    PyDict_SetItemString(d, "INV_SEEK_SET", Py_BuildValue("i", SEEK_SET));
    PyDict_SetItemString(d, "INV_SEEK_CUR", Py_BuildValue("i", SEEK_CUR));
    PyDict_SetItemString(d, "INV_SEEK_END", Py_BuildValue("i", SEEK_END));

    /*-----------------------------------------------------------------------*/

    PyDict_SetItemString(d, "INV_READ", Py_BuildValue("i", INV_READ));
    PyDict_SetItemString(d, "INV_WRITE", Py_BuildValue("i", INV_WRITE));

    /*-----------------------------------------------------------------------*/

    Py_INCREF(Pg_True); PyDict_SetItemString(d, "PG_True", Pg_True);
    Py_INCREF(Pg_False); PyDict_SetItemString(d, "PG_False", Pg_False);
    

    /*-----------------------------------------------------------------------*/

    PqErr_Warning	    = PyErr_NewException("libpq.Warning",
						 PyExc_StandardError,
						 (PyObject *)NULL);
    PqErr_Error		    = PyErr_NewException("libpq.Error",
						 PyExc_StandardError,
						 (PyObject *)NULL);
    PqErr_InterfaceError    = PyErr_NewException("libpq.InterfaceError",
						 PqErr_Error,
						 (PyObject *)NULL);
    PqErr_DatabaseError	    = PyErr_NewException("libpq.DatabaseError",
						 PqErr_Error,
						 (PyObject *)NULL);
    PqErr_DataError	    = PyErr_NewException("libpq.DataError",
						 PqErr_DatabaseError,
						 (PyObject *)NULL);
    PqErr_OperationalError  = PyErr_NewException("libpq.OperationalError",
						 PqErr_DatabaseError,
						 (PyObject *)NULL);
    PqErr_IntegrityError    = PyErr_NewException("libpq.IntegrityError",
						 PqErr_DatabaseError,
						 (PyObject *)NULL);
    PqErr_InternalError	    = PyErr_NewException("libpq.InternalError",
						 PqErr_DatabaseError,
						 (PyObject *)NULL);
    PqErr_ProgrammingError  = PyErr_NewException("libpq.ProgrammingError",
						 PqErr_DatabaseError,
						 (PyObject *)NULL);
    PqErr_NotSupportedError = PyErr_NewException("libpq.NotSupportedError",
						 PqErr_DatabaseError,
						 (PyObject *)NULL);

    PyDict_SetItemString(d, "Warning", PqErr_Warning);
    PyDict_SetItemString(d, "Error", PqErr_Error);
    PyDict_SetItemString(d, "InterfaceError", PqErr_InterfaceError);
    PyDict_SetItemString(d, "DatabaseError", PqErr_DatabaseError);
    PyDict_SetItemString(d, "DataError", PqErr_DataError);
    PyDict_SetItemString(d, "OperationalError", PqErr_OperationalError);
    PyDict_SetItemString(d, "IntegrityError", PqErr_IntegrityError);
    PyDict_SetItemString(d, "InternalError", PqErr_InternalError);
    PyDict_SetItemString(d, "ProgrammingError", PqErr_ProgrammingError);
    PyDict_SetItemString(d, "NotSupportedError", PqErr_NotSupportedError);

    /*-----------------------------------------------------------------------*/

    PyDict_SetItemString(d, "PgConnectionType", (PyObject *)&PgConnection_Type);
    PyDict_SetItemString(d, "PgResultType",	(PyObject *)&PgResult_Type);
    PyDict_SetItemString(d, "PgLargeObjectType",
			 (PyObject *)&PgLargeObject_Type);
    PyDict_SetItemString(d, "PgBooleanType",	(PyObject *)&PgBoolean_Type);
#if defined(HAVE_LONG_LONG_SUPPORT)
    PyDict_SetItemString(d, "PgInt8Type",	(PyObject *)&PgInt8_Type);
#endif
    PyDict_SetItemString(d, "PgInt2Type",	(PyObject *)&PgInt2_Type);
    PyDict_SetItemString(d, "PgVersionType",	(PyObject *)&PgVersion_Type);

    /*-----------------------------------------------------------------------*/

    if (PyErr_Occurred())
	Py_FatalError("Can't initialize module libpq.\n");
}
