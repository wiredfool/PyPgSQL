// #ident "@(#) $Id: pgresult.c,v 1.19 2006/05/28 22:29:06 ghaering Exp $"
/**(H+)*****************************************************************\
| Name:			pgresult.c												|
|																		|
| Description:	This file implements the PgResult Object for Python		|
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
| 16AUG2011 eds reordered & removed redundant includes to kill warnings |
| 29MAY2006 gh	Integrated patch #1224272. Use PyOS_ascii_strtod		|
|				instead of strtod in order to be locale-agnostic.		|
| 01OCT2002 gh	HAVE_LONG_LONG => HAVE_LONG_LONG_SUPPORT				|
| 03FEB2002 bga Change the constant that is used to determine when to	|
|				check an OID to see if it is a Large Object form 1700	|
|				(PG_NUMERIC) to 16383 (MAX_RESERVED_OID).				|
| 26OCT2001 bga [Bug #474771] Found and plugged a memory leak in the	|
|				PgResult_New() function.  An object for the value of	|
|				PGcmdStatus was being created twice, leaving an extra	|
|				copy around to consume memory.							|
| 13OCT2001 bga Added support for the pickling of pyPgSQL objects.		|
| 30SEP2001 bga Change error message returned by PgResult_ntuple_check	|
|				if no tuples were returned in the result.  The returned |
|				error message now makes sense.							|
| 24SEP2001 bga Added support for the PostgreSQL BYTEA type.			|
| 21SEP2001 bga I have removed resultErrorMessage as an attribute.	It	|
|				doesn't make sense to leave it since it will always be	|
|				None.  Why?	 Because any query that generates an error	|
|				will raise an exception, not return a PgResult.			|
| 14SEP2001 bga Removed code related to PostgreSQL 6.5.x.  We now only	|
|				support PostgreSQL 7.0 and later.						|
| 06SEP2001 bga Fixed a gcc reported int format <-> long int arguement	|
|				mis-match.												|
| 05SEP2001 bga Added check to make sure that the result was from a DQL |
|				(Data Query Language) statement in methods that only	|
|				make sense for DQL statement results (fname(), etc.).	|
| 03SEP2001 bga Methods that take field and/or tuple indices will now	|
|				raise an exception if those indices are out of range.	|
|				The previous behaviour was to return the error code		|
|				from the underlying PostgreSQL C-API function.			|
|			--- The fnumber() method will now raise an exeception if it |
|				is passed a string that is not a valid column name.		|
|				The previous behaviour was to return the error code		|
|				from the PostgreSQL C-API PQfnumber function.			|
| 31AUG2001 bga Correct some incorrect comments.						|
| 30AUG2001 bga Use PyObject_Del() instead of PyMem_Free() in dealloc() |
|				to delete the object.									|
|			--- Added a cache (implemented with a Python Dictionary)	|
|				for OIDs to hold the result of the check to see if the	|
|				OID is a large object.	This should reduce the number	|
|				queries sent to the database for this purpose.			|
|			--- Add code to not check OIDs whose value is less than or	|
|				equal to 1700 (PG_NUMERIC).	 These OIDs are not large	|
|				objects.												|
| 29AUG2001 bga Part of the logic for building the cursor.desccription	|
|				attribute retrives an OID from the database that is		|
|				usually zero (0).  This causes a query to the database	|
|				to check to see if the OID represents a large object.	|
|				An OID of zero can never be a large object, so the code |
|				was changed so that when the OID is zero, the check to	|
|				see if it is a large object is skipped.					|
| 28AUG2001 bga Picked some lint from the code.							|
| 24AUG2001 bga Change the code so that the query() method of the con-	|
|				nection is called instead of calling PQexec() directly. |
| 21AUG2001 bga Changed the return type of result.cmdTuple from a		|
|				Python String object to an Integer object.				|
|			--- The code now used pgversion.post70 as a Python Integer. |
|			--- Constant, read-only attributes are now stored as Python |
|				objects.  This way the Python object does not have to	|
|				be re-created each time they are referenced.			|
| 06AUG2001 bga Initial release by Billy G. Allie.						|
\*(H-)******************************************************************/

#include <Python.h>
#include <stddef.h>
#include <ctype.h>
#include <structmember.h>
#include "libpqmodule.h"

/***************************************************************\
| Build a cache for OIDs that will be used in checking to see	|
| if an OID is a large object.	Since this check requires a		|
| database query, we will cache the results in order to mini-	|
| mize the number of queries to the database.  We will use a	|
| Python Dictionary as the cache, using the OID as the key.		|
| The value will be an flag: 1 := LargeObject, 0 := OID.		|
\***************************************************************/

static PyObject *oidCache = (PyObject *)NULL;

/*--------------------------------------------------------------------------*/

#if (PY_VERSION_HEX < 0x02040000)

/* code stolen from Python 2.4 */

#include <locale.h>

#define ISSPACE(c)	((c) == ' ' || (c) == '\f' || (c) == '\n' || \
					 (c) == '\r' || (c) == '\t' || (c) == '\v')
#define ISDIGIT(c)	((c) >= '0' && (c) <= '9')
#define ISXDIGIT(c) (ISDIGIT(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))

/**
 * PyOS_ascii_strtod:
 * @nptr:	 the string to convert to a numeric value.
 * @endptr:	 if non-%NULL, it returns the character after
 *		   the last character used in the conversion.
 * 
 * Converts a string to a #gdouble value.
 * This function behaves like the standard strtod() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtod() function.
 *
 * If the correct value would cause overflow, plus or minus %HUGE_VAL
 * is returned (according to the sign of the value), and %ERANGE is
 * stored in %errno. If the correct value would cause underflow,
 * zero is returned and %ERANGE is stored in %errno.
 * 
 * This function resets %errno before calling strtod() so that
 * you can reliably detect overflow and underflow.
 *
 * Return value: the #gdouble value.
 **/
double
PyOS_ascii_strtod(const char  *nptr,
				  char		 **endptr)
{
	char *fail_pos;
	double val;
	struct lconv *locale_data;
	const char *decimal_point;
	int decimal_point_len;
	const char *p, *decimal_point_pos;
	const char *end = NULL; /* Silence gcc */

	/* g_return_val_if_fail (nptr != NULL, 0); */
	assert(nptr != NULL);

	fail_pos = NULL;

	locale_data = localeconv();
	decimal_point = locale_data->decimal_point;
	decimal_point_len = strlen(decimal_point);

	assert(decimal_point_len != 0);

	decimal_point_pos = NULL;
	if (decimal_point[0] != '.' ||
			decimal_point[1] != 0)
	{
		p = nptr;
		/* Skip leading space */
		while (ISSPACE(*p))
			p++;

		/* Skip leading optional sign */
		if (*p == '+' || *p == '-')
			p++;

		if (p[0] == '0' &&
				(p[1] == 'x' || p[1] == 'X'))
		{
			p += 2;
			/* HEX - find the (optional) decimal point */

			while (ISXDIGIT(*p))
				p++;

			if (*p == '.')
			{
				decimal_point_pos = p++;

				while (ISXDIGIT(*p))
					p++;

				if (*p == 'p' || *p == 'P')
					p++;
				if (*p == '+' || *p == '-')
					p++;
				while (ISDIGIT(*p))
					p++;
				end = p;
			}
		}
		else
		{
			while (ISDIGIT(*p))
				p++;

			if (*p == '.')
			{
				decimal_point_pos = p++;

				while (ISDIGIT(*p))
					p++;

				if (*p == 'e' || *p == 'E')
					p++;
				if (*p == '+' || *p == '-')
					p++;
				while (ISDIGIT(*p))
					p++;
				end = p;
			}
		}
		/* For the other cases, we need not convert the decimal point */
	}

	/* Set errno to zero, so that we can distinguish zero results
	   and underflows */
	errno = 0;

	if (decimal_point_pos)
	{
		char *copy, *c;

		/* We need to convert the '.' to the locale specific decimal point */
		copy = malloc(end - nptr + 1 + decimal_point_len);

		c = copy;
		memcpy(c, nptr, decimal_point_pos - nptr);
		c += decimal_point_pos - nptr;
		memcpy(c, decimal_point, decimal_point_len);
		c += decimal_point_len;
		memcpy(c, decimal_point_pos + 1, end - (decimal_point_pos + 1));
		c += end - (decimal_point_pos + 1);
		*c = 0;

		val = strtod(copy, &fail_pos);

		if (fail_pos)
		{
			if (fail_pos > decimal_point_pos)
				fail_pos = (char *)nptr + (fail_pos - copy) - (decimal_point_len - 1);
			else
				fail_pos = (char *)nptr + (fail_pos - copy);
		}

		free(copy);

	}
	else
		val = strtod(nptr, &fail_pos);

	if (endptr)
		*endptr = fail_pos;

	return val;
}

#endif

/*--------------------------------------------------------------------------*/

PyObject *PgResult_New(PGresult *res, PgConnection *conn, int type)
{
	PgResult *self;

	if (!res)
	{
		PyErr_SetString(PqErr_OperationalError,
							PQerrorMessage(PgConnection_Get(conn)));
		return (PyObject *)NULL;
	}

	self = (PgResult *)PyObject_New(PgResult, &PgResult_Type);

	if (self)
	{
		char *m;
		Oid	  o;

		self->res = res;
		Py_INCREF(conn); self->conn = conn;
		self->type	  = Py_BuildValue("i", type);
		self->status  = Py_BuildValue("i", PQresultStatus(res));
		self->ntuples = Py_BuildValue("i", PQntuples(res));
		self->nfields = Py_BuildValue("i", PQnfields(res));
		self->btuples = Py_BuildValue("i", PQbinaryTuples(res));
		if (*(m = PQcmdStatus(res)) == (char)0)
		{
			Py_INCREF(Py_None);
			self->cstatus = Py_None;
		}
		else
			self->cstatus  = Py_BuildValue("s", m);
		if (*(m = PQcmdTuples(res)) == (char)0)
		{
			Py_INCREF(Py_None);
			self->ctuples = Py_None;
		}
		else
			self->ctuples  = Py_BuildValue("l", strtol(m, NULL, 10));
		o = PQoidValue(res);
		if (o == InvalidOid)
		{
			Py_INCREF(Py_None);
			self->oidval = Py_None;
		}
		else
			self->oidval = Py_BuildValue("l", (long)o);
	}

	return (PyObject *)self;
}

/*--------------------------------------------------------------------------*/

static void PgResult_dealloc(PgResult *self)
{
	if (self->res)
		PQclear(self->res);

	Py_XDECREF(self->conn);
	Py_XDECREF(self->type);
	Py_XDECREF(self->status);
	Py_XDECREF(self->ntuples);
	Py_XDECREF(self->nfields);
	Py_XDECREF(self->btuples);
	Py_XDECREF(self->cstatus);
	Py_XDECREF(self->ctuples);
	Py_XDECREF(self->oidval);

	PyObject_Del((PyObject *)self);
}

/*--------------------------------------------------------------------------*/

static PyObject *PgResult_repr(PgResult *self)
{
	char buf[100];

	(void)sprintf(buf, "<PgResult instance at %p>", (void *)self);

	return Py_BuildValue("s", buf);
}

/*--------------------------------------------------------------------------*/

int PgResult_check(PyObject *self)
{
	if (!PgResult_Check(self))
	{
		PyErr_SetString(PyExc_TypeError, "not a PgResult object");
		return FALSE;
	}

	if (!(PgResult_Get((PgResult *)self)))
	{
		PyErr_SetString(PqErr_InterfaceError,
							"PgResult object has been cleared");
		return FALSE;
	}

	return TRUE;
}

/*--------------------------------------------------------------------------*/

static int PgResult_is_DQL(PgResult *self)
{
	/*******************************************************************\
	| Since this is only called from within pgresult.c, and we can en-	|
	| sure that it is only called with a PgResult object, we don't have |
	| to check self.													|
	\*******************************************************************/

	if (PyInt_AS_LONG(self->type) != RESULT_DQL)
	{
		PyErr_SetString(PqErr_InterfaceError,
						"PgResult object was not generated by a DQL statement");
		return FALSE;
	}

	return TRUE;
}

/*--------------------------------------------------------------------------*/

static int PgResult_ntuple_check(PgResult *self, int tnum)
{
	/*******************************************************************\
	| Since this is only called from within pgresult.c, and we can en-	|
	| sure that it is only called with a PgResult object, we don't have |
	| to check self.													|
	\*******************************************************************/

	if (!(0 <= tnum && tnum < PyInt_AS_LONG(self->ntuples)))
	{
		char buf[256];

		if (PyInt_AS_LONG(self->ntuples) > 0)
			sprintf(buf, "tuple index outside valid range of 0..%ld.",
					(PyInt_AS_LONG(self->ntuples) - 1));
		else
			strcpy(buf, "result does not contain any tuples.");
		PyErr_SetString(PyExc_ValueError, buf);
		return FALSE;
	}

	return TRUE;
}

/*--------------------------------------------------------------------------*/

static int PgResult_nfield_check(PgResult *self, int fnum)
{
	/*******************************************************************\
	| Since this is only called from within pgresult.c, and we can en-	|
	| sure that it is only called with a PgResult object, we don't have |
	| to check self.													|
	\*******************************************************************/

	if (!(0 <= fnum && fnum < PyInt_AS_LONG(self->nfields)))
	{
		char buf[256];

		sprintf(buf, "field index outside valid range of 0..%ld.",
				(PyInt_AS_LONG(self->nfields) - 1));
		PyErr_SetString(PyExc_ValueError, buf);
		return FALSE;
	}

	return TRUE;
}

/***********************************************************************\
| Define the PgResult methods.											|
\***********************************************************************/

static char libPQfname_Doc[] =
	"fname(idx) -- Returns the field (attribute) name associated with the "
	"given\n\t\tfield index.  Field indices start at 0.";

static PyObject *libPQfname(PgResult *self, PyObject *args)
{
	int fnum;

	if (!PgResult_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PgResult_is_DQL(self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args, "i:fname", &fnum)) 
		return (PyObject *)NULL;

	if (!PgResult_nfield_check(self, fnum))
		return (PyObject *)NULL;

	return Py_BuildValue("s", PQfname(PgResult_Get(self), fnum));
}

/*--------------------------------------------------------------------------*/

static char libPQfnumber_Doc[] =
	"fnumber(name) -- Returns the field (attribute) index associated with "
	"the\n\t\t given field name.";

static PyObject *libPQfnumber(PgResult *self, PyObject *args)
{
	char *fieldName;
	int	 fnum;

	if (!PgResult_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PgResult_is_DQL(self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args, "s:fnumber", &fieldName)) 
		return (PyObject *)NULL;

	if ((fnum = PQfnumber(PgResult_Get(self), fieldName)) < 0)
	{
		char buf[128];

		sprintf(buf, "'%.32s' is not a valid column name.", fieldName);
		PyErr_SetString(PyExc_ValueError, buf);
		return (PyObject *)NULL;
	}

	return Py_BuildValue("i", PQfnumber(PgResult_Get(self), fieldName));
}

/*--------------------------------------------------------------------------*/

static char libPQftype_Doc[] =
	"ftype(idx) -- Returns an internal coding of the field type associated "
	"with\n\t\tthe given field index.  Field indices start at 0.";

static PyObject *libPQftype(PgResult *self, PyObject *args)
{
	int fnum;

	if (!PgResult_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PgResult_is_DQL(self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args, "i:ftype", &fnum)) 
		return (PyObject *)NULL;

	if (!PgResult_nfield_check(self, fnum))
		return (PyObject *)NULL;

	return Py_BuildValue("i", PQftype(PgResult_Get(self), fnum));
}

/*--------------------------------------------------------------------------*/

static char libPQfsize_Doc[] =
	"fsize(idx) -- Returns the size (in bytes) of the field associated with "
	"the\n\t\tgiven field index.  Field indices start at 0.\n\nPQfsize "
	"returns the space allocated for this field in a database tuple (i.e.,\n"
	"the size of the server's binary representation of the data type. A -1 "
	"is\nreturned for variable length fields.";

static PyObject *libPQfsize(PgResult *self, PyObject *args)
{
	int fnum;

	if (!PgResult_check((PyObject *)self))
		return (PyObject *)NULL;

	if (!PgResult_is_DQL(self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args, "i:fsize", &fnum)) 
		return (PyObject *)NULL;

	if (!PgResult_nfield_check(self, fnum))
		return (PyObject *)NULL;

	return Py_BuildValue("i", PQfsize(PgResult_Get(self), fnum));
}

/*--------------------------------------------------------------------------*/

static char libPQfmod_Doc[] =
	"fmod(idx) -- Returns the type-specific modification data of the field\n"
	"\t		  associated with the given field index.  Field indices start "
	"at 0.";

static PyObject *libPQfmod(PgResult *self, PyObject *args)
{
	int fnum;

	if (!PgResult_check((PyObject *)self))
		return NULL;

	if (!PgResult_is_DQL(self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args, "i:fmod", &fnum)) 
		return NULL;

	if (!PgResult_nfield_check(self, fnum))
		return (PyObject *)NULL;

	return Py_BuildValue("i", PQfmod(PgResult_Get(self), fnum));
}

/*--------------------------------------------------------------------------*/

extern PyObject *unQuoteBytea(char *);

static char libPQgetvalue_Doc[] =
	"getvalue(tidx, fidx) -- Returns a single field (attribute) value of "
	"one\n\t\t\t  tuple of a PgResult.	Tuple and field indices start\n\t\t"
	"\t	 at 0.";

static PyObject *libPQgetvalue(PgResult *self, PyObject *args)
{
	PyObject	*valueObj;
	PGresult	*res;
	char		*s, *p;
	int			tnum;
	int			fnum;

	if (!PgResult_check((PyObject *)self))
		return NULL;

	if (!PgResult_is_DQL(self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args, "ii:getvalue", &tnum, &fnum)) 
		return NULL;

	if (!PgResult_ntuple_check(self, tnum))
		return (PyObject *)NULL;

	if (!PgResult_nfield_check(self, fnum))
		return (PyObject *)NULL;

	res = PgResult_Get(self);

	if (PQgetisnull(res, tnum, fnum))
	{
		Py_INCREF(Py_None);
		valueObj = Py_None;
	}
	else
	{
		char *value = (char *)PQgetvalue(res, tnum, fnum);

		switch (PQftype(res, fnum)) {
			case PG_BOOL:
				valueObj = (*value == 't' ? Pg_True : Pg_False);
				Py_INCREF(valueObj);
				break;

			case PG_OID:
				/*******************************************************\
				| We have an OID column, so we have to determine if is	|
				| a postgreSQL large object.  We do this:				|
				| (PostgreSQL prior to version 7.1)						|
				|	  by querying the pg_attribute table to see if this |
				|	  OID has an attribute of 'odata'.					|
				| (PostgreSQL version 7.1 and later)					|
				|	  by querying the pg_largeobject table to see if	|
				|	  this OID exist in the table.						|
				|														|
				| Note: Any OID <= MAX_RESERVED_OID (16383) can not be	|
				|		a Large Object.									|
				|														|
				| Note: We cache weither or not an OID is a LargeObject |
				|		in order to reduce the number of times we need	|
				|		to query the database.							|
				\*******************************************************/

				valueObj = Py_BuildValue("l", strtol(value, (char **)NULL, 10));
				if (valueObj == (PyObject *)NULL)
					return valueObj;

				if (PyInt_AS_LONG(valueObj) <= MAX_RESERVED_OID)
					break;

				/*******************************************************\
				| Check the cache to see if we already looked up the OID|
				| in the database.	If we have, used the cached results.|
				\*******************************************************/

				if (PyDict_Check(oidCache))
				{
					PyObject *rVal = PyDict_GetItem(oidCache, valueObj);
					if (rVal != (PyObject *)NULL)
					{
						if (PyInt_AS_LONG(rVal) == 1)
							valueObj = 
					  (PyObject *)PgLargeObject_New((PyObject *)self->conn,
													PyInt_AS_LONG(valueObj), 0);
						break;
					}
				}

				/*******************************************************\
				| If we got to this point, then the OID is not in the	|
				| cache.  Query the database and place the results in	|
				| the cache.											|
				\*******************************************************/

				if (PgConnection_check((PyObject *)(self->conn)))
				{
					int y;
					PyObject *pgres;
					PyObject *post70 =
						PyObject_GetAttrString(self->conn->version, "post70");

					/***************************************************\
					| The query to use to tell if an OID is a large ob- |
					| changed in version 7.1.  Use PgVersion.post70 to	|
					| determine which query to use.						|
					\***************************************************/

					if ((post70 != (PyObject *)NULL) && PyInt_AsLong(post70))
					{
						s = "SELECT loid FROM pg_largeobject "
							"WHERE loid = %s LIMIT 1";
					}
					else
					{
						s = "SELECT * FROM pg_attribute "
							"WHERE attrelid = %s AND attname = 'odata'";
					}

					Py_XDECREF(post70);

					if ((p = (char *)PyMem_Malloc(strlen(s) +
												  strlen(value) + 1)) ==
						(char *)NULL)
					{
						PyErr_SetString(PyExc_MemoryError,
									   "Can't allocate memory in getvalue().");
						return (PyObject *)NULL;
					}
					(void)sprintf(p, s, value);
					pgres = PyObject_CallMethod((PyObject *)self->conn,
												"query", "s", p);
					PyMem_Free(p);
					if (pgres == (PyObject *)NULL)
					{
						return (PyObject *)NULL;
					}

					y = ((PQntuples(((PgResult *)pgres)->res) > 0) ? 1 : 0);

					/***************************************************\
					| Add the OID and the flag to the cache.			|
					| Note: PyDict_SetItem() will Py_INCREF valueObj	|
					|		and the flag.								|
					\***************************************************/

					if (oidCache)
						PyDict_SetItem(oidCache, valueObj,
									   (y ? Pg_True : Pg_False));

					if (y)
						valueObj =
					  (PyObject *)PgLargeObject_New((PyObject *)self->conn,
													PyInt_AS_LONG(valueObj), 0);

					Py_XDECREF(pgres);
					break;
				}
				else
					PyErr_Clear();
				break;

			case PG_INT4:
				valueObj = Py_BuildValue("l", strtol(value, (char **)NULL, 10));
				break;

			case PG_INT2:
				valueObj = (PyObject *)PgInt2_FromString(value,
														 (char **)NULL, 10);
				break;

			case PG_INT8:
#if defined(HAVE_LONG_LONG_SUPPORT)
				valueObj = (PyObject *)PgInt8_FromString(value,
														 (char **)NULL, 10);
#else
				valueObj = PyLong_FromString(value, NULL, 10);
#endif
				break;

			case PG_CASH:
				s = value;
				if (*s == '-' || *s == '(')
					*s++ = '-';
				p = s;
				while (*s)
				{
					if (*s == '$' || *s == ',' || *s == ')')
						s++;
					else
						*p++ = *s++;
				}
				*p = 0;
				/*FALLTHRU*/

			case PG_FLOAT4:
				/*FALLTHRU*/

			case PG_FLOAT8:
				valueObj = Py_BuildValue("d", PyOS_ascii_strtod(value, NULL));
				break;

			case PG_BYTEA:
				valueObj = unQuoteBytea(value);
				break;

			default:
				valueObj = Py_BuildValue("s", value);
		}
	}

	return valueObj;
}

/*--------------------------------------------------------------------------*/

static char libPQgetlength_Doc[] =
	"getlength(tidx, fidx) -- Returns the length of a field (attribute) in "
	"bytes.\n\t\t\t	  Tuple and field indices start at 0.\n\nGetlength "
	"returns the actual data length for the particular data value, that\nis "
	"the size of the object returned to by getvaule.";

static PyObject *libPQgetlength(PgResult *self, PyObject *args)
{
	int			tnum;
	int			fnum;

	if (!PgResult_check((PyObject *)self))
		return NULL;

	if (!PgResult_is_DQL(self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args, "ii:getlength", &tnum, &fnum)) 
		return NULL;

	if (!PgResult_ntuple_check(self, tnum))
		return (PyObject *)NULL;

	if (!PgResult_nfield_check(self, fnum))
		return (PyObject *)NULL;

	return Py_BuildValue("i", PQgetlength(PgResult_Get(self), tnum, fnum));
}

/*--------------------------------------------------------------------------*/

static char libPQgetisnull_Doc[] =
	"getisnull(tidx, fidx) -- Tests a field for a NULL entry.  Tuple and "
	"field\n\t\t\t	 indices start at 0.";

static PyObject *libPQgetisnull(PgResult *self, PyObject *args)
{
	int			tnum;
	int			fnum;

	if (!PgResult_check((PyObject *)self))
		return NULL;

	if (!PgResult_is_DQL(self))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args, "ii:getisnull", &tnum, &fnum)) 
		return NULL;

	if (!PgResult_ntuple_check(self, tnum))
		return (PyObject *)NULL;

	if (!PgResult_nfield_check(self, fnum))
		return (PyObject *)NULL;

	return Py_BuildValue("i", PQgetisnull(PgResult_Get(self), tnum, fnum));
}

/*--------------------------------------------------------------------------*/

static char libPQclear_Doc[] =
	"clear() -- Frees the storage associated with the query result.";

static PyObject * libPQclear(PgResult *self, PyObject *args)
{
	PGresult	*res;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError, "clear() takes no parameters");
		return NULL;
	}

	if (!PgResult_check((PyObject *)self))
		return NULL;

	res = PgResult_Get(self);

	if (res)
		PQclear(res);

	self->res = (PGresult *)NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static PyMethodDef PgResult_methods[] = {
	{ "fname",	   (PyCFunction)libPQfname,		1, libPQfname_Doc },
	{ "fnumber",   (PyCFunction)libPQfnumber,	1, libPQfnumber_Doc },
	{ "ftype",	   (PyCFunction)libPQftype,		1, libPQftype_Doc },
	{ "fsize",	   (PyCFunction)libPQfsize,		1, libPQfsize_Doc },
	{ "fmod",	   (PyCFunction)libPQfmod,		1, libPQfmod_Doc },
	{ "getvalue",  (PyCFunction)libPQgetvalue,	1, libPQgetvalue_Doc },
	{ "getlength", (PyCFunction)libPQgetlength, 1, libPQgetlength_Doc },
	{ "getisnull", (PyCFunction)libPQgetisnull, 1, libPQgetisnull_Doc },
	{ "clear",	   (PyCFunction)libPQclear,		1, libPQclear_Doc },
	{ NULL, NULL }
};

/*--------------------------------------------------------------------------*/

#define RsOFF(x) offsetof(PgResult, x)

static struct memberlist PgResult_members[] = {
	{ "resultType",		T_OBJECT,		RsOFF(type),	RO },
	{ "resultStatus",	T_OBJECT,		RsOFF(status),	RO },
	{ "ntuples",		T_OBJECT,		RsOFF(ntuples), RO },
	{ "nfields",		T_OBJECT,		RsOFF(nfields), RO },
	{ "binaryTuples",	T_OBJECT,		RsOFF(btuples), RO },
	{ "cmdStatus",		T_OBJECT,		RsOFF(cstatus), RO },
	{ "cmdTuples",		T_OBJECT,		RsOFF(ctuples), RO },
	{ "oidValue",		T_OBJECT,		RsOFF(oidval),	RO },
	{ NULL												   }
};

/*--------------------------------------------------------------------------*/

static PyObject *PgResult_getattr(PgResult *self, char* attr)
{
	PyObject *a = Py_FindMethod(PgResult_methods, (PyObject *)self, attr);

	if (a != (PyObject *)NULL)
		return a;

	PyErr_Clear();

	if (!strcmp(attr, "__module__"))
		return Py_BuildValue("s", MODULE_NAME);

	if (!strcmp(attr, "__class__"))
		return Py_BuildValue("s", self->ob_type->tp_name);

	return PyMember_Get((char *)self, PgResult_members, attr);
}

/*--------------------------------------------------------------------------*/

static char PgResult_Type_Doc[] = "This is the type of PgResult objects";

PyTypeObject PgResult_Type = {
	PyObject_HEAD_INIT(NULL)
	(int)NULL,							/*ob_size*/
	MODULE_NAME ".PgResult",			/*tp_name*/
	sizeof(PgResult),					/*tp_basicsize*/
	(int)NULL,							/*tp_itemsize*/
	(destructor)PgResult_dealloc,		/*tp_dealloc*/
	(printfunc)NULL,					/*tp_print*/
	(getattrfunc)PgResult_getattr,		/*tp_getattr*/
	(setattrfunc)NULL,					/*tp_setattr*/
	NULL,								/*tp_compare*/
	(reprfunc)PgResult_repr,			/*tp_repr*/
	NULL,								/*tp_as_number*/
	NULL,								/*tp_as_sequence*/
	NULL,								/*tp_as_mapping*/
	(hashfunc)NULL,						/*tp_hash*/
	(ternaryfunc)NULL,					/*tp_call*/
	(reprfunc)NULL,						/*tp_str*/
	NULL, NULL, NULL, (int)NULL,
	PgResult_Type_Doc
};

void initpgresult(void)
{
	PgResult_Type.ob_type		= &PyType_Type;

	oidCache = Py_BuildValue("{}");
}
