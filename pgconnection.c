// #ident "@(#) $Id: pgconnection.c,v 1.24 2005/09/26 08:08:17 ghaering Exp $"
/**(H+)*****************************************************************\
| Name:			pgconnection.c											|
|																		|
| Description:	This file implements the PgConnection Object for Python |
|=======================================================================|
| Copyright 2001 by Billy G. Allie.										|
| All rights reserved.													|
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
|=======================================================================|
| Revision History:														|
|																		|
| Date		Ini Description												|
| --------- --- ------------------------------------------------------- |
| 02SEP2011 eds Added libPQexecParams/query_params                      |
| 16AUG2011 eds reordered & removed redundant includes to kill warnings |
| 15AUG2011 eds Moved PgQuote* into PgConnection to use pqQuote*conn    |
|               functions in libpq. Fixed segfault in debug tests.      |
| 23MAR2005 bga Change code to fix some compatability problems with		|
|				PostgreSQL 8.x.											|
| 05MAR2005 bga Added missing PQclear() (fixes minor memory leak).		|
|				[Bug #987719]											|
| 09MAY2004 bga Added a 'debug' feature to the PgConnection object.		|
| 26JUN2003 bga Fixed a bug I introduced into lo_import.				|
| 15JUN2003 bga Applied patch by Laurent Pinchart to correct a problem	|
|				lo_import, lo_export, lo_unlink.						|
| 13DEC2002 gh	In case PQgetResult returns NULL, let libPQgetResult	|
|				return a Python None, like the docstring says.	This is |
|				necessary in order to be able to cancel queries, as		|
|				after cancelling a query with PQrequestCancel, we need	|
|				to read results until PQgetResult returns NULL.			|
| 03NOV2001 bga Fixed a possible memory leak.							|
| 17OCT2001 bga Added the lo_export() method.  It was overlooked in the |
|				original code.											|
| 13OCT2001 bga Added support for the pickling of PgConnection objects. |
|				In particular, a hidden method was added to return the	|
|				connection information string used to create the con-	|
|				nection.												|
| 01OCT2001 bga Changed all new style comments to original style.		|
| 30SEP2001 bga Added some brakets to clarify ambiguous else clauses.	|
| 29SEP2001 bga Fixed numerous bugs found during the development of the |
|				regression test cases for pgconnection.c.				|
|			--- The PgConnection's attributes are now set to a value of |
|				None when finish() is called.							|
| 23SEP2001 bga Fixed problem when compiling with gcc.					|
|			--- [Bug #464123] Removed pgFixEsc().  The need for it no	|
|				longer exists since we now use a different method of	|
|				quoting strings that doesn't involve repr(), which in-	|
|				serted the hex escpae sequences to begin with.			|
| 22SEP2001 bga Fixed bugs uncovered during testing with the new test	|
|				cases for regression testing.							|
|			--- Improved the error checking on the mode parameter in	|
|				lo_creat().												|
|			--- Removed lo_get().  It's functionality is superceeded by |
|				the PgLargeObject constructor added to libpqmodule.c.	|
| 20SEP2001 bga [Bug #455514] Modified pgFixEsc to that if will also	|
|				handle octal escape sequences.	If the escape sequence	|
|				comes in as octal, then a character encoding such as	|
|				Latin-2 is in use.	If the value of the character is >	|
|				127, then the escape sequence is replaced by the actual |
|				character.	This should fix the reported problem.		|
|			--- Fixed a bug I introduced into pgFixEsc.					|
| 19SEP2001 bga Re-ordered the items in PgConnection_members so that	|
|				the attributes handled directly by PgConnection_getattr |
|				are grouped together and commented appropiately.		|
| 14SEP2001 bga Removed code related to PostgreSQL 6.5.x.  We now only	|
|				support PostgreSQL 7.0 and later.						|
| 02SEP2001 bga Modified pgFixEsc so that any escaped sequence (\xHH)	|
|				whose value is greater than 127 is replaced by the		|
|				actual character.  This works around an apparent prob-	|
|				lem in PostgreSQL involving encodings.	[Bug #455514]	|
| 30AUG2001 bga Use PyObject_Del() instead of PyMem_Free() in dealloc() |
|				to delete the object.									|
| 13AUG2001 bga Changed how ibuf is defined in pgFixEsc to ensure that	|
|				the memory pointed to by ibuf is allocated from the		|
|				stack, and thus is writable memory.	 The way it was de- |
|				fined before could result in the memory being allocated |
|				in a read-only segment. [Bug #450330]					|
| 06AUG2001 bga Initial release by Billy G. Allie.						|
\*(H-)******************************************************************/

#include <Python.h>
#include <stddef.h>
#include <ctype.h>
#include <structmember.h>
#include "libpqmodule.h"

#ifdef _MSC_VER
#define strcasecmp lstrcmpiA
#endif

/*******************************************************\
| Notice Processor - Adds database notices to a list	|
\*******************************************************/

static void queueNotices(void *list, const char *msg)
{
	PyObject *str = Py_BuildValue("s", msg);

	if (!str)
	{
		PyErr_Clear();
		return;
	}

	if (PyList_Insert((PyObject *)list, 0, str))
	{
		Py_XDECREF(str);
		PyErr_Clear();
	}

	return;
}

/*----------------------------------------------------------------------*/

PyObject *PgConnection_New(PGconn *conn)
{
	PgConnection *self;

	self = (PgConnection *)PyObject_New(PgConnection, &PgConnection_Type);

	if (self)
	{
		PGresult *res;
		char *r;

		self->conn = conn;
		if ((self->notices = Py_BuildValue("[]")) == (PyObject *)NULL)
		{
			Py_XDECREF(self);
			return (PyObject *)NULL;
		}

		r = PQhost(conn);
		if (r == NULL || *r == 0)
			r = "localhost";
		self->host		= Py_BuildValue("s", r);
		self->port		= Py_BuildValue("l", strtol(PQport(conn), NULL, 10));
		self->db		= Py_BuildValue("s", PQdb(conn));
		self->options	= Py_BuildValue("s", PQoptions(conn));
		self->tty		= Py_BuildValue("s", PQtty(conn));
		self->user		= Py_BuildValue("s", PQuser(conn));
		r = PQpass(conn);
		if (r != NULL && *r == 0) {
			self->pass = Py_None;
			Py_INCREF(Py_None);
		} else {
			self->pass	= Py_BuildValue("s", r);
		}
		self->bePID		= Py_BuildValue("i", PQbackendPID(conn));
		self->socket	= Py_BuildValue("i", PQsocket(conn));
		self->debug		= Py_None;
		Py_INCREF(Py_None);

		if (PyErr_Occurred())
		{
			Py_XDECREF(self);
			return (PyObject *)NULL;
		}

		/* We have a connection, use it to get the version of the		*/
		/* PostgreSQL backend that we are connected to.					*/

		Py_BEGIN_ALLOW_THREADS
		res = PQexec(conn, "select version()");
		Py_END_ALLOW_THREADS

		self->version = PgVersion_New((char *)PQgetvalue(res, 0, 0));

		PQclear(res);

		if (self->version == (PyObject *)NULL)
		{
			Py_XDECREF(self);
			return (PyObject *)NULL;
		}

		/* Setup a notice processor that will store notices in the		*/
		/* notice list for this connection								*/

		PQsetNoticeProcessor(conn, queueNotices, (void *)self->notices);
	}

	return (PyObject *)self;
}													 

/*----------------------------------------------------------------------*/

static void PgConnection_dealloc(PgConnection *self)
{
	if (self->conn)
		PQfinish(self->conn);

	Py_XDECREF(self->host);
	Py_XDECREF(self->port);
	Py_XDECREF(self->db);
	Py_XDECREF(self->options);
	Py_XDECREF(self->tty);
	Py_XDECREF(self->user);
	Py_XDECREF(self->pass);
	Py_XDECREF(self->bePID);
	Py_XDECREF(self->socket);
	Py_XDECREF(self->version);
	Py_XDECREF(self->notices);
	Py_XDECREF(self->debug);

	PyObject_Del(self);
}

/*----------------------------------------------------------------------*/

static PyObject *PgConnection_repr(PgConnection *self)
{
	char buf[100];

	(void)sprintf(buf, "<%s PgConnection at %p>",
				  (self->conn ? "Opened" : "Closed"), (void *)self);
	return Py_BuildValue("s", buf);
}

/*----------------------------------------------------------------------*/

int PgConnection_check(PyObject *self)
{
	if (self->ob_type != &PgConnection_Type)
	{
		PyErr_SetString(PyExc_TypeError, "not a PgConnection object");
		return FALSE;
	}

	if (!(PgConnection_Get((PgConnection *)self)))
	{
		PyErr_SetString(PqErr_InterfaceError,
						"PgConnection object is closed");
		return FALSE;
	}

	return TRUE;
}

/***********************************************************************\
| Define the PgConnection methods.										|
\***********************************************************************/

static int getResultType(PGresult *res)
{
	const char *ct;
	int	 t;

	switch (PQresultStatus(res))
	{
		case PGRES_TUPLES_OK:
			t = RESULT_DQL;
			break;
		
		case PGRES_COMMAND_OK:
		case PGRES_COPY_OUT:
		case PGRES_COPY_IN:
			t = RESULT_DDL;
			ct = PQcmdTuples(res);
			if (ct[0])
				t = RESULT_DML;
			break;
			
		case PGRES_EMPTY_QUERY:
			t = RESULT_EMPTY;
			break;
			
		default:
			t = RESULT_ERROR;
			break;
	}

	return (t);
}

/*--------------------------------------------------------------------------*/

static char *debugQuery(char *debug, char *query)
{
	if ((strcasecmp(debug, "div") == 0) ||
		(strcasecmp(debug, "pre") == 0) ||
		(strcasecmp(debug, "html") == 0))
	{
		char *_tag = NULL;
		PyObject *_query = NULL;
		PyObject *_fmt = NULL;
		PyObject *_tmp = NULL;
		PyObject *_res = NULL;
					  
		if (strcasecmp(debug, "div") == 0)
			_tag = "div";
		else
			_tag = "pre";

		_fmt = PyString_FromString("<%s style='background: #aaaaaa; "
								   "border: thin dashed #333333'>"
								   "%s</%s>");
		if (_fmt == (PyObject *)NULL) goto ErrorExit10;

		_query = PyString_FromString(query);
		if (_query == (PyObject *)NULL) goto ErrorExit10;

		/**
		 * The 'replace' method call will either return a copy of the original
		 * string (with the reference count incremented) if no replacements
		 * were made, or a copy of the string with a replacements made.
		 */
		_tmp = PyObject_CallMethod(_query, "replace", "ss", "&", "&amp;");
		if (_tmp == (PyObject *)NULL) goto ErrorExit10;
		Py_XDECREF(_query);
		_query = _tmp;
		_tmp = PyObject_CallMethod(_query, "replace", "ss", "<", "&lt;");
		if (_tmp == (PyObject *)NULL) goto ErrorExit10;
		Py_XDECREF(_query);
		_query = _tmp;
		_tmp = PyObject_CallMethod(_query, "replace", "ss", ">", "&gt;");
		if (_tmp == (PyObject *)NULL) goto ErrorExit10;
		Py_XDECREF(_query);
		_query = _tmp;
		_tmp = Py_BuildValue("(sOs)", _tag, _query, _tag);
		_res = PyString_Format(_fmt, _tmp);
		puts(PyString_AsString(_res));
ErrorExit10:
		Py_XDECREF(_fmt);
		Py_XDECREF(_query);
		Py_XDECREF(_tmp);
		Py_XDECREF(_res);
		if (PyErr_Occurred() != NULL)
			return NULL;
	} else {
		printf("QUERY: %s\n", query);
	}
	return "";
}

/*--------------------------------------------------------------------------*/

static char libPQexec_Doc[] =
	"query() -- Submit a query and wait for the result(s).";

static PyObject *libPQexec(PgConnection *self, PyObject *args)
{
	PGresult	*res;
	char		*query;
	int			rtype;

	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"s:query", &query)) 
		return NULL;

	if (PyString_Check(self->debug) && (self->debug != Py_None)) {
		// UNDONE some test cases aren't Py_None, yet they're not strings. 
		// Unclear why, but a little extra guarding doesn't hurt. 
		if (debugQuery(PyString_AsString(self->debug), query) == NULL)
			return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	res = PQexec(PgConnection_Get(self), query);
	Py_END_ALLOW_THREADS

	if ((rtype = getResultType(res)) == RESULT_ERROR)
	{
		PyObject *exc;
		char *errmsg = PQerrorMessage(PgConnection_Get(self));

		switch (PQresultStatus(res))
		{
			case PGRES_NONFATAL_ERROR:
				exc = PqErr_ProgrammingError;	/* Barrowed reference */
				break;

			case PGRES_FATAL_ERROR:
				/* Check to see if a referential integrity error occured and
				 * set the appropiate exception. */
				if (strstr(errmsg, "referential integrity violation") != NULL)
					exc = PqErr_IntegrityError;	  /* Barrowed reference */
				else
					exc = PqErr_OperationalError; /* Barrowed reference */
				break;

			default:
				exc = PqErr_InternalError;		/* Barrowed reference */
		}

		PyErr_SetString(exc, errmsg);
		PQclear(res);
		return NULL;
	}

	return PgResult_New(res, self, rtype);
}

/*--------------------------------------------------------------------------*/

static char libPQexecParams_Doc[] =
	"query() -- Submit a parameterized query and wait for the result(s).";

static PyObject *libPQexecParams(PgConnection *self, PyObject *args)
{
	PGresult	*res;
	char		*query;
	int			rtype;
	PyObject    *py_params;
	char        **paramValues;
	int         nParams;
	int         *paramLengths;
	int         *paramFormats;
	int         i;

	if (!PgConnection_check((PyObject *)self)){
		return NULL;
	}

	if (!PyArg_ParseTuple(args,"sO:query", &query, &py_params)) {
		return NULL;
	}

	if (!PyList_Check(py_params)) {
		// undone, check error type
		PyErr_SetString(PqErr_InternalError, "Params must be a list");
		return NULL;
	}

	nParams = (int)PyList_Size(py_params);
	if (!nParams){
		paramValues = NULL;
		paramLengths = NULL;
		paramFormats = NULL;
	} else {
		paramValues = PyMem_Malloc(sizeof(char *)*nParams);
		if (!paramValues) {
			return PyErr_NoMemory();
		}
		paramLengths = PyMem_Malloc(sizeof(int)*nParams);
		if (!paramLengths) {
			PyMem_Free(paramValues);
			return PyErr_NoMemory();
		}
		paramFormats = PyMem_Malloc(sizeof(int)*nParams);
		if (!paramValues) {
			PyMem_Free(paramLengths);
			PyMem_Free(paramValues);
			return PyErr_NoMemory();
		}
		
		PyObject *elt;
		for (i=0;i<nParams;i++){
			// List can be unquoted/escaped strings or 
			// pgBytea objects, or text format numbers &c.
			elt = PyList_GetItem(py_params, (Py_ssize_t)i);
			if (PyString_Check(elt)){
				// we've got a string or text version of a number
				paramValues[i] = PyString_AsString(elt); // nul terminiated, borrowed.
				paramLengths[i] = 0; // ignored
				paramFormats[i] = 0; // text format
			} else if (PyObject_HasAttrString(elt,"value")){
				PyObject *bytea_str = PyObject_GetAttrString(elt, "value");
				int ret;
				Py_ssize_t length;

				if (!PyString_Check(bytea_str)){
					// barf
					PyMem_Free(paramLengths);
					PyMem_Free(paramValues);
					PyMem_Free(paramFormats);
					return NULL;
				}
				ret = PyString_AsStringAndSize(bytea_str, &paramValues[i], &length);
				paramLengths[i] = (int) length;
				paramFormats[i] = 1;
				Py_XDECREF(bytea_str);
				if (ret == -1){
					//typeerror
					PyMem_Free(paramLengths);
					PyMem_Free(paramValues);
					PyMem_Free(paramFormats);
					return NULL;
				}	
			}
		}
	}

	
	if (PyString_Check(self->debug) && (self->debug != Py_None)) {
		// UNDONE some test cases aren't Py_None, yet they're not strings. 
		// Unclear why, but a little extra guarding doesn't hurt. 
		if (debugQuery(PyString_AsString(self->debug), query) == NULL)
			return NULL;
	}


	


	Py_BEGIN_ALLOW_THREADS
	res = PQexecParams(PgConnection_Get(self), 
					   query,
					   nParams,
					   NULL, /*paramTypes, in oid*/
					   (const char * const*) paramValues,
					   paramLengths, /* for binary parameters, ignored for text. */
					   paramFormats, /* 0 = text, 1=binary */
					   0); /* 0 text format returned, 1 for binary */
	Py_END_ALLOW_THREADS

	

	if ((rtype = getResultType(res)) == RESULT_ERROR)
	{
		PyObject *exc;
		char *errmsg = PQerrorMessage(PgConnection_Get(self));

		switch (PQresultStatus(res))
		{
			case PGRES_NONFATAL_ERROR:
				exc = PqErr_ProgrammingError;	/* Barrowed reference */
				break;

			case PGRES_FATAL_ERROR:
				/* Check to see if a referential integrity error occured and
				 * set the appropiate exception. */
				if (strstr(errmsg, "referential integrity violation") != NULL)
					exc = PqErr_IntegrityError;	  /* Barrowed reference */
				else
					exc = PqErr_OperationalError; /* Barrowed reference */
				break;

			default:
				exc = PqErr_InternalError;		/* Barrowed reference */
		}

		PyErr_SetString(exc, errmsg);
		PQclear(res);
		return NULL;
	}

	if (paramValues){
		PyMem_Free(paramLengths);
		PyMem_Free(paramValues);
		PyMem_Free(paramFormats);
	}

	return PgResult_New(res, self, rtype);
}


/*--------------------------------------------------------------------------*/

static char libPQquoteString_Doc[] =
    "PgQuoteString(string) -> string\n"
    "    This is a helper function that will quote a Python String in a "
    "manner\n    that is acceptable to PostgreSQL";


static PyObject *libPQquoteString(PgConnection *self, PyObject *args) {
    int error,  addl, i, j, forArray=0;
	size_t ct_converted, slen;
    char *sin;
    char *sout;
	char *sforarray = 0;
    PyObject *result;
	PGconn *conn;

	if (!PgConnection_check((PyObject *)self))
		return NULL;

	conn = PgConnection_Get(self);

    if (!PyArg_ParseTuple(args,"s|i:PgQuoteString", &sin, &forArray)) 
        return NULL;

    /* Calculate the size for the new memory string. */
    slen = strlen((char *)sin);

	if (forArray) {
		// figure out how much for the double encoding  
		// just pre-double encode the important bits. 
		// It's 3 passes, and it's roughly double the memory, but 
		// forarray used to take a 7x, so it's probably a little better. 
		addl = 0;
		for (i = 0; i < slen; i++) {
			switch (sin[i]){
			case '\\':
            case '"':
				addl++;
				break;
			}
		}
		sforarray = (char *)PyMem_Malloc(slen + addl + 1); 
		if (sforarray == (char *)NULL)
			return PyErr_NoMemory();
		for (i=0, j=0; i < slen; i++) {
			switch (sin[i]){
			case '\\':
            case '"':
				sforarray[j++] = '\\';
                sforarray[j++] = sin[i];
				break;
			default:
				sforarray[j++] = sin[i];
			}
		}
		sforarray[j] = (char)0;
		slen = slen + addl;
		sin = sforarray; /* swap me some pointers */
	}
				
    i = (slen * 2) + 4;

    sout = (char *)PyMem_Malloc(i); /* Assume every char is quoted */
    if (sout == (char *)NULL)
        return PyErr_NoMemory();
    i=0;
	if (!forArray) {
		sout[i++] = 'E';
	}
    sout[i++] = (forArray ? '"' : '\'');
	ct_converted = PQescapeStringConn(conn, sout + i, sin, slen, &error);
	sout[i + ct_converted] = (forArray ? '"' : '\'');
	sout[i + ct_converted +1 ] = (char)0;

	if (sforarray) {
		PyMem_Free(sforarray);
	}

	if (error) {
		/* punt. on the thames */
	} 
	
    result = Py_BuildValue("s#", sout, ct_converted + i + 1);
    PyMem_Free(sout);

    return result;
}

/*--------------------------------------------------------------------------*/

static char libPQquoteBytea_Doc[] =
    "PgQuoteBytea(string, forArray) -> string\n"
    "    This is a helper function that will quote a Python String (that can "
    "contain embedded NUL bytes) in a manner\n    that is acceptable to "
    "PostgreSQL";

static PyObject *libPQquoteBytea(PgConnection *self, PyObject *args)
{
    int i, slen, forArray = 0;
	size_t to_length;
    unsigned char *sin;
	unsigned char *sint;
    unsigned char *sout;
    PyObject *result;
	PGconn *conn;
 
	if (!PgConnection_check((PyObject *)self))
		return NULL;

	conn = PgConnection_Get(self);

    if (!PyArg_ParseTuple(args,"s#|i:PgQuoteBytea", &sin, &slen, &forArray)) 
        return NULL;

	sint = PQescapeByteaConn(conn, sin, slen, &to_length);

    sout = (unsigned char *)PyMem_Malloc(to_length + 4);
    if (sout == (unsigned char *)NULL){
        return PyErr_NoMemory();
	}

	i = 0;
	if (!forArray) {
		sout[i++] = 'E';
	}
    sout[i++] = (forArray ? '"' : '\'');
	memcpy(sout + i, sint, to_length);
    PQfreemem(sint);

	/* overwrite the null with a ' */
	sout[to_length + i -1] = (forArray ? '"' : '\''); 

	/* s# doesn't need a null value at the end, since it's length checked */
    result = Py_BuildValue("s#", sout, to_length + i);
	PyMem_Free(sout);

    return result;
}



/*--------------------------------------------------------------------------*/

static char libPQsetnonblocking_Doc[] =
	"setnonblocking(onOff) -- Make the connection non-blocking if 'onOff' is "
	"TRUE,\n\t\t\t otherwise make the connection blocking.\n";

static PyObject *libPQsetnonblocking(PgConnection *self, PyObject *args)
{
	int onOff;

	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args, "i:setnonblocking", &onOff)) 
		return NULL;

	onOff = ((onOff != 0) ? 1 : 0);

	if (PQsetnonblocking(PgConnection_Get(self), onOff))
	{
		PyErr_SetString(PqErr_InternalError,
						PQerrorMessage(PgConnection_Get(self)));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQsendQuery_Doc[] =
	"sendQuery() -- Submit a query to PostgreSQL without waiting for the "
	"result(s).\n";

static PyObject *libPQsendQuery(PgConnection *self, PyObject *args)
{
	char		*query;

	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"s:sendQuery", &query)) 
		return NULL;

	if ( PyString_Check(self->debug) && (self->debug != Py_None)) {
		// UNDONE some test cases aren't Py_None, yet they're not strings. 
		// Unclear why, but a little extra guarding doesn't hurt. 
		if (debugQuery(PyString_AsString(self->debug), query) == NULL)
			return NULL;
	}

	if (!PQsendQuery(PgConnection_Get(self), query))
	{
		PyErr_SetString(PqErr_InternalError,
						PQerrorMessage(PgConnection_Get(self)));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQgetResult_Doc[] =
	"getResult() -- Wait for the next result from a prior PQsendQuery().\n\n"
	"getResult will return a PgResult object, or None indicating the query is "
	"done.";

static PyObject *libPQgetResult(PgConnection *self, PyObject *args)
{
	PGresult	*res;
	int			rtype;

	if (!PgConnection_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"getResult() takes no parameters");
		return (PyObject *)NULL;
	}

	res = PQgetResult(PgConnection_Get(self));

	if (res == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	if ((rtype = getResultType(res)) == RESULT_ERROR)
	{
		PyObject *exc;

		switch (PQresultStatus(res))
		{
			case PGRES_NONFATAL_ERROR:
				exc = PqErr_ProgrammingError;	/* Barrowed reference */
				break;

			case PGRES_FATAL_ERROR:
				exc = PqErr_OperationalError;	/* Barrowed reference */
				break;

			default:
				exc = PqErr_InternalError;		/* Barrowed reference */
		}

		PyErr_SetString(exc, PQerrorMessage(PgConnection_Get(self)));
		PQclear(res);
		return (PyObject *)NULL;
	}

	return (PyObject *)PgResult_New(res, self, rtype);
}

/*--------------------------------------------------------------------------*/

static char libPQconsumeInput_Doc[] =
	"consumeInput() -- If input is available from the backend, consume it.";

static PyObject *libPQconsumeInput(PgConnection *self, PyObject *args)
{
	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args, "")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"consumeInput() takes no parameters");
		return NULL;
	}

	if (!PQconsumeInput(PgConnection_Get(self)))
	{
		PyErr_SetString(PqErr_InternalError,
						PQerrorMessage(PgConnection_Get(self)));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQflush_Doc[] =
	"flush() -- Attempt to flush any data queued on the backend.";

static PyObject *libPQflush(PgConnection *self, PyObject *args)
{
	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args, "")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"flush() takes no parameters");
		return NULL;
	}

	if (PQflush(PgConnection_Get(self)))
	{
		PyErr_SetString(PqErr_InternalError,
						PQerrorMessage(PgConnection_Get(self)));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQconnectPoll_Doc[] =
	"PQconnectPoll(PgConnection) -> Integer\n"
	"	 Poll the libpq C API for connection status.";

static PyObject *libPQconnectPoll(PgConnection *self, PyObject *args)
{
	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"connectPoll() takes no parameters");
		return NULL;
	}

	return Py_BuildValue("i", PQconnectPoll(PgConnection_Get(self)));
}

/*--------------------------------------------------------------------------*/

static char libPQrequestCancel_Doc[] =
	"requestCancel() -- Request the the current query be canceled\n\n"
	"requestCancel sends a cancel request to the backend.  Note that "
	"successfully\ndispatching the request does not mean the request will "
	"be cancelled.\n";

static PyObject *libPQrequestCancel(PgConnection *self, PyObject *args)
{
	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args, "")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"requestCancel() takes no parameters");
		return NULL;
	}

	if (!PQrequestCancel(PgConnection_Get(self)))
	{
		PyErr_SetString(PqErr_InternalError,
						PQerrorMessage(PgConnection_Get(self)));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQfinish_Doc[] =
	"finish() -- Close the connection to the backend.\n\nFinish closes "
	"the connection to the backend and frees the memory used by\nthe PGconn "
	"object.";

static PyObject * libPQfinish(PgConnection *self, PyObject *args)
{
	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"finish() takes no parameters");
		return NULL;
	}

	PQfinish(PgConnection_Get(self));

	self->conn = (PGconn *)NULL;
	Py_XDECREF(self->host); self->host = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->port); self->port = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->db); self->db = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->options); self->options = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->tty); self->tty = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->user); self->user = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->pass); self->pass = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->bePID); self->bePID = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->socket); self->socket = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->version); self->version = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->notices); self->notices = Py_None; Py_XINCREF(Py_None);
	Py_XDECREF(self->debug); self->debug = Py_None; Py_XINCREF(Py_None);

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQreset_Doc[] =
  "reset() -- Reset the communication port with the backend.\n\n"
  "reset will close the connection to the backend and attempt to reestablish "
  "\na new connection with the same postmaster, using the parameter "
  "previously used.";

static PyObject *libPQreset(PgConnection *self, PyObject *args)
{
	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"reset() takes no parameters");
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	PQreset(PgConnection_Get(self));
	Py_END_ALLOW_THREADS

	if (PQstatus(PgConnection_Get(self)) != CONNECTION_OK)
	{
		PyErr_SetString(PqErr_DatabaseError,
						PQerrorMessage(PgConnection_Get(self)));
		PQfinish(PgConnection_Get(self));
		PgConnection_Get(self) = (PGconn *)NULL;
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQnotifies_Doc[] =
	"notifies() -- Returns the next notification from a list of unhandled "
	"notifi-\n\t	  cation messages received from the backend or None.";

static PyObject *libPQnotifies(PgConnection *self, PyObject *args)
{
	if (!PgConnection_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"notifies() takes no parameters");
		return (PyObject *)NULL;
	}

	return PgNotify_New(PQnotifies(PgConnection_Get(self)));
}

/*--------------------------------------------------------------------------*/

static char libPQgetline_Doc[] =
	"getline() -- Reads a newline-terminated line of characters from the "
	"backend\n";

static PyObject *libPQgetline(PgConnection *self, PyObject *args)
{
	char *ci_buf = (char *)NULL;
	int	 size, idx, res;
	PyObject *result;
	PGconn *cnx;

	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"getline() takes no parameters");
		return NULL;
	}

	cnx = PgConnection_Get(self);

	for (size = idx = 0, res = 1; res > 0; idx = (size - 1))
	{
		size += MAX_BUFFER_SIZE;
		if ((ci_buf = (char *)PyMem_Realloc(ci_buf, size)) == (char *)NULL)
			return PyErr_NoMemory();

		Py_BEGIN_ALLOW_THREADS
		res = PQgetline(cnx, &ci_buf[idx], (size - idx));
		Py_END_ALLOW_THREADS
	}

	if (res == EOF)
	{
		Py_INCREF(Py_None);
		result = Py_None;
	}
	else
		result = Py_BuildValue("s", ci_buf);

	PyMem_Free(ci_buf);
	
	return result;
}

/*--------------------------------------------------------------------------*/

static char libPQgetlineAsync_Doc[] =
	"getlineAsync() -- Reads a newline-terminated line of characters from the "
	"backend\n\t\t	without blocking.  Returns None if no data is present, "
	"'\\.' if\n\t\t	 the end-of-copy-data marker is seen, or the line of "
	"characters\n\t\t  read";

static PyObject *libPQgetlineAsync(PgConnection *self, PyObject *args)
{
	char *ci_buf = (char *)NULL;
	int	 size, idx, res, leave;
	PyObject *result = (PyObject *)NULL;
	PGconn *cnx;

	if (!PgConnection_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"getlineAsync() takes no parameters");
		return (PyObject *)NULL;
	}

	cnx = PgConnection_Get(self);

	for (size = idx = leave = 0; !leave; idx = size)
	{
		size += MAX_BUFFER_SIZE;

		if ((ci_buf = (char *)PyMem_Realloc(ci_buf, size)) == (char *)NULL)
			return PyErr_NoMemory();

		if (!PQconsumeInput(cnx))
		{
			PyErr_SetString(PqErr_InternalError, PQerrorMessage(cnx));
			PyMem_Free(ci_buf);
			return (PyObject *)NULL;
		}

		res = PQgetlineAsync(cnx, &ci_buf[idx], size);
		switch (res)
		{
			case -1:
				leave = 1;
				result = Py_BuildValue("s", "\\."); /* return EOD indicator */
				break;
				
			case 0:
				leave = 1;
				Py_INCREF(Py_None);
				result = Py_None;
				break;

			default:
				if (ci_buf[idx + res - 1] == '\n')
				{
					leave = 1;
					ci_buf[idx + res - 1] = 0;
					result = Py_BuildValue("s", ci_buf);
				}
		}
	}

	PyMem_Free(ci_buf);

	if (PyErr_Occurred())
		return (PyObject *)NULL;

	return result;
}

/*--------------------------------------------------------------------------*/

static char libPQputline_Doc[] =
	"putline(string) -- Sends a null-terminated string to the backend server\n";

static PyObject *libPQputline(PgConnection *self, PyObject *args)
{
	char *line;
	int	 res;

	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"s:putline", &line)) 
		return NULL;

	Pg_BEGIN_ALLOW_THREADS(PgConnection_Get(self))
	res = PQputline(PgConnection_Get(self), line);
	Pg_END_ALLOW_THREADS(PgConnection_Get(self))

	if (res)
	{
		PyErr_SetString(PqErr_InternalError,
						PQerrorMessage(PgConnection_Get(self)));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQendcopy_Doc[] =
	"endcopy() -- Syncs with the backend\n";

static PyObject *libPQendcopy(PgConnection *self, PyObject *args)
{
	int res;

	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"endcopy() takes no parameters");
		return NULL;
	}

	Pg_BEGIN_ALLOW_THREADS(PgConnection_Get(self))
	res = PQendcopy(PgConnection_Get(self));
	Pg_END_ALLOW_THREADS(PgConnection_Get(self))

	if (res)
	{
		PyErr_SetString(PqErr_InternalError,
						PQerrorMessage(PgConnection_Get(self)));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQtrace_Doc[] =
	"trace(fileObj) -- Enable tracing of frontend/backend communications to a "
	"file\n\t\t	 stream\n";

static PyObject *libPQtrace(PgConnection *self, PyObject *args)
{
	PyObject	*out;

	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"O!:trace", &PyFile_Type, &out)) 
		return NULL;
	
	PQtrace(PgConnection_Get(self), PyFile_AsFile(out));

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char libPQuntrace_Doc[] =
	"untrace() -- Disable tracing started by trace()\n";

static PyObject *libPQuntrace(PgConnection *self, PyObject *args)
{
	if (!PgConnection_check((PyObject *)self))
		return NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"untrace() takes no parameters");
		return NULL;
	}

	PQuntrace(PgConnection_Get(self));

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

/* The following define is only used internally, to track if the 'b'
 * flag in the mode is seen.
 */
#define INV_BIN 0x00010000

extern struct {
	char *name;
	int	 mode;
} validmodes[];

static char PgLo_creat_Doc[] =
	"lo_creat(mode) -- Creates a PgLargeObject with the attribues given in "
	"mode.";

static PyObject *PgLo_creat(PgConnection *self, PyObject *args)
{
	Oid oid;
	int	 i;
	int	 mode;			/* mode					*/
	char *mname;		/* mode (as string)		*/

	if (!PgConnection_check((PyObject *)self))
		return (PyObject *)NULL;

	mname = (char *)NULL;
	mode = 0;

	if (!PyArg_ParseTuple(args,"s:lo_creat", &mname))
	{
		PyErr_Clear();
		if (!PyArg_ParseTuple(args, "i:lo_creat", &mode))
			return (PyObject *)NULL;
	}

	if (mname != (char *)NULL)
		for (i = 0; validmodes[i].name != (char *)NULL; i++)
			if (strcmp(mname, validmodes[i].name) == 0)
			{
				mode = validmodes[i].mode;
				break;
			}

	/* This loop will look up the text version of mode, if passed in as an
	 * integer, or normalize the text version of mode if passed in as a
	 * string.
	 */
	for (i = 0; validmodes[i].name != (char *)NULL; i++)
		if (mode == validmodes[i].mode)
		{
			mname = validmodes[i].name;
			break;
		}

	if (validmodes[i].name == (char *)NULL)
	{
		PyErr_SetString(PyExc_ValueError, "invalid mode for lo_creat()");
		return (PyObject *)NULL;
	}

	/* Make sure that INV_BIN is not set. */
	mode &= (INV_READ | INV_WRITE);

	if (!(oid = lo_creat(PgConnection_Get(self), mode)))
	{
		PyErr_SetString(PqErr_OperationalError, "Can't create large object.");
		return (PyObject *)NULL;
	}

	return PgLargeObject_New((PyObject *)self, oid, 0);
}

/*--------------------------------------------------------------------------*/

static char PgLo_import_Doc[] =
	"lo_import(filename) -- Import a file as a large object.";

static PyObject *PgLo_import(PgConnection *self, PyObject *args) {
	Oid			oid;
	char		*filename = (char *)NULL;

	if (!PgConnection_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"s:lo_import", &filename)) 
		return (PyObject *)NULL;

	if ((oid = lo_import(PgConnection_Get(self), filename)) <= 0)
	{
		PyErr_SetString(PqErr_OperationalError, "Can't import large object.");
		return (PyObject *)NULL;
	}

	return PgLargeObject_New((PyObject *)self, oid, 0);
}


/*--------------------------------------------------------------------------*/

static char PgLo_export_Doc[] =
	"lo_export(oid, filename) -- Export a large object to a file.";

static PyObject *PgLo_export(PgConnection *self, PyObject *args) {
	Oid			oid;
	char		*filename = (char *)NULL;

	if (!PgConnection_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"is:lo_export", &oid, &filename)) 
		return (PyObject *)NULL;

	if (lo_export(PgConnection_Get(self), oid, filename) < 0)
	{
		PyErr_SetString(PqErr_OperationalError, "Can't export large object.");
		return (PyObject *)NULL;
	}

	Py_XINCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char PgLo_unlink_Doc[] =
	"unlink() -- Removes a large object from the database.";

static PyObject *PgLo_unlink(PgConnection *self, PyObject *args)
{
	Oid oid;

	if (!PgConnection_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"i:lo_unlink", &oid)) 
		return (PyObject *)NULL;

	if (lo_unlink(PgConnection_Get(self), oid) < 0)
	{
		PyErr_SetString(PyExc_IOError, "error unlinking large object");
		return (PyObject *)NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static PyMethodDef PgConnection_methods[] = {
	{"consumeInput",(PyCFunction)libPQconsumeInput,1,libPQconsumeInput_Doc},
	{"endcopy", (PyCFunction)libPQendcopy, 1, libPQendcopy_Doc},
	{"finish", (PyCFunction)libPQfinish, 1, libPQfinish_Doc},
	{"getResult", (PyCFunction)libPQgetResult, 1, libPQgetResult_Doc},
	{"getline", (PyCFunction)libPQgetline, 1, libPQgetline_Doc},
	{"getlineAsync", (PyCFunction)libPQgetlineAsync, 1, libPQgetlineAsync_Doc},
	{"lo_creat", (PyCFunction)PgLo_creat, 1, PgLo_creat_Doc},
	{"lo_import", (PyCFunction)PgLo_import, 1, PgLo_import_Doc},
	{"lo_export", (PyCFunction)PgLo_export, 1, PgLo_export_Doc},
	{"lo_unlink", (PyCFunction)PgLo_unlink, 1, PgLo_unlink_Doc},
	{"notifies", (PyCFunction)libPQnotifies, 1, libPQnotifies_Doc},
	{"putline", (PyCFunction)libPQputline, 1, libPQputline_Doc},
	{"query", (PyCFunction)libPQexec, 1, libPQexec_Doc},
	{"query_params", (PyCFunction)libPQexecParams, 1, libPQexecParams_Doc},
	{"requestCancel",(PyCFunction)libPQrequestCancel,1,libPQrequestCancel_Doc},
	{"reset", (PyCFunction)libPQreset, 1, libPQreset_Doc},
	{"sendQuery", (PyCFunction)libPQsendQuery, 1, libPQsendQuery_Doc},
	{"trace", (PyCFunction)libPQtrace, 1, libPQtrace_Doc},
	{"untrace", (PyCFunction)libPQuntrace, 1, libPQuntrace_Doc},
	{"connectPoll", (PyCFunction)libPQconnectPoll, 1, libPQconnectPoll_Doc},
	{"flush", (PyCFunction)libPQflush, 1, libPQflush_Doc},
	{"setnonblocking", (PyCFunction)libPQsetnonblocking, 1,
													libPQsetnonblocking_Doc},
    { "PgQuoteString", (PyCFunction)libPQquoteString, 1, libPQquoteString_Doc },
    { "PgQuoteBytea", (PyCFunction)libPQquoteBytea, 1, libPQquoteBytea_Doc },
	{ NULL, NULL }
};
 
/*--------------------------------------------------------------------------*/

#define CoOFF(x) offsetof(PgConnection, x)

static struct memberlist PgConnection_members[] = {
	{ "host",			T_OBJECT,		CoOFF(host),	RO },
	{ "port",			T_OBJECT,		CoOFF(port),	RO },
	{ "db",				T_OBJECT,		CoOFF(db),		RO },
	{ "tty",			T_OBJECT,		CoOFF(tty),		RO },
	{ "user",			T_OBJECT,		CoOFF(user),	RO },
	{ "password",		T_OBJECT,		CoOFF(pass),	RO },
	{ "backendPID",		T_OBJECT,		CoOFF(bePID),	RO },
	{ "socket",			T_OBJECT,		CoOFF(socket),	RO },
	{ "notices",		T_OBJECT,		CoOFF(notices), RO },
	{ "version",		T_OBJECT,		CoOFF(version), RO },
/*	The remaining attributes are handles directly in PgConnection_getattr() */
	{ "status",			T_INT,			0,				RO },
	{ "errorMessage",	T_STRING,		0,				RO },
	{ "isBusy",			T_INT,			0,				RO },
	{ "isnonblocking",	T_INT,			0,				RO },
	{ "debug",			T_OBJECT,		CoOFF(debug),	0  },
	{ NULL												   }
};

/*--------------------------------------------------------------------------*/

static PyObject *PgConnection_getattr(PgConnection *self, char* attr)
{
	PGconn *cnx;

	/* See if the attribute is a method.  If so, return it. */

	PyObject *a = Py_FindMethod(PgConnection_methods, (PyObject *)self, attr);

	if (a != (PyObject *)NULL)
		return a;

	PyErr_Clear();

	/* Ok, It's not a method.  Let's process it as a member. */

	cnx = PgConnection_Get(self);

	if (!strcmp(attr, "status"))
	{
		if (cnx == (PGconn *)NULL)
		{
			Py_XINCREF(Py_None);
			return Py_None;
		}
		else
			return Py_BuildValue("i", PQstatus(cnx));
	}

	if (!strcmp(attr, "errorMessage"))
	{
		char *m;

		if (cnx == (PGconn *)NULL)
		{
			Py_XINCREF(Py_None);
			return Py_None;
		}

		m = PQerrorMessage(cnx);

		if (m != NULL && *m == (char)0)
		{
			Py_INCREF(Py_None);
			return Py_None;
		}
		else
			return Py_BuildValue("s", m);
	}

	if (!strcmp(attr, "isBusy"))
	{
		if (cnx == (PGconn *)NULL)
		{
			Py_XINCREF(Py_None);
			return Py_None;
		}
		else
			return Py_BuildValue("i", PQisBusy(cnx));
	}

	if (!strcmp(attr, "isnonblocking"))
	{
		if (cnx == (PGconn *)NULL)
		{
			Py_XINCREF(Py_None);
			return Py_None;
		}
		else
			return Py_BuildValue("i", PQisnonblocking(cnx));
	}

	if (!strcmp(attr, "__module__"))
		return Py_BuildValue("s", MODULE_NAME);

	if (!strcmp(attr, "__class__"))
		return Py_BuildValue("s", self->ob_type->tp_name);

	if (!strcmp(attr, "_conninfo"))
		return self->cinfo;

	/***********************************************************\
	| This is a hidden member that, when accessed, toggles the	|
	| showQuery flag and returns it's new state.  It's handy	|
	| for trouble shooting.										|
	\***********************************************************/

	if (!strcmp(attr, "toggleShowQuery"))
	{
		if (self->debug != Py_None)
		{
			self->debug = Py_None;
			Py_INCREF(Py_None);
		}
		else
		{
			self->debug = PyString_FromString("text");
			Py_DECREF(Py_None);
		}
		return self->debug;
	}

	return PyMember_Get((char *)self, PgConnection_members, attr);
}
 
static int PgConnection_setattr(PgConnection *self, char* attr,
									  PyObject *val)
{
	return PyMember_Set((char *)self, PgConnection_members, attr, val);
}

/*--------------------------------------------------------------------------*/

static char PgConnection_Type_Doc[] = "This is the type of PgConnection objects";

PyTypeObject PgConnection_Type = {
	PyObject_HEAD_INIT(NULL)
	(Py_ssize_t)NULL,							/* ob_size				*/
	MODULE_NAME ".PgConnection",		/* tp_name				*/
	sizeof(PgConnection),				/* tp_basicsize			*/
	(Py_ssize_t)NULL,							/* tp_itemsize			*/
	(destructor)PgConnection_dealloc,	/* tp_dealloc			*/
	(printfunc)NULL,					/* tp_print				*/
	(getattrfunc)PgConnection_getattr,	/* tp_getattr			*/
	(setattrfunc)PgConnection_setattr,	/* tp_setattr			*/
	NULL,								/* tp_compare			*/
	(reprfunc)PgConnection_repr,		/* tp_repr				*/
	NULL,								/* tp_as_number			*/
	NULL,								/* tp_as_sequence		*/
	NULL,								/* tp_as_mapping		*/
	(hashfunc)NULL,						/* tp_hash				*/
	(ternaryfunc)NULL,					/* tp_call				*/
	(reprfunc)NULL,						/* tp_str				*/
	NULL, NULL, NULL, (Py_ssize_t)NULL,
	PgConnection_Type_Doc
};												  

void initpgconnection(void)
{
	PgConnection_Type.ob_type	= &PyType_Type;
}
