// #ident "@(#) $Id: pgversion.c,v 1.24 2006/06/02 14:27:45 ghaering Exp $"
/**(H+)*****************************************************************\
| Name:			pgversion.c												|
|																		|
| Description:	This file contains code to allow Python to have access	|
|				to PostgreSQL databases via the libpq functions.  This	|
|				module, is not DB-API 2.0 compliant.  It only exposes	|
|				(most of) the PostgreSQL libpq C API to Python.			|
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
| 16AUG2011 eds removed unused pgstricmp, reordered includes to build   | 
|               without warnings.                                       |
| 02JUN2006 gh	Applied patch #882032. Vendor-extensions to version		|
|				number should not create problems any longer.			|
| 26SEP2005 gh	Minor fix for stricter C compilers (move variable		|
|				declaration to start of function).						|
| 06OCT2004 bga [Bug #786712 & #816729] Allowed for a version string	|
|				containing the words "alpha" and "beta".				|
|				Thanks to Gerhard Quell and Jeff Putman for the fixes.	|
| 26NOV2002 bga Allowed for release canidate (rcN) version strings.		|
| 16SEP2002 gh	Reflect the change to the unconditionally included		|
|				pg_strtok_r.											|
| 16APR2002 gh	Replace _tolower with the standard C tolower function,	|
|				to make this file compile on FreeBSD.					|
| 21JAN2002 bga Expanded the fix of 12JAN2002 to also handle beta ver-	|
|				sions of PostgreSQL.									|
| 12JAN2002 bga [Bug #486151] fixed a problem that prevented a connec-	|
|				tion to be made to version 7.2devel of PostgreSQL.		|
| 18SEP2001 bga Removed variables that are no longer needed/referenced. |
| 16SEP2001 bga Corrected my mis-conceptions about Python ignoring ex-	|
|				ceptions generated during coercion.						|
| 15SEP2001 bga Fixed problem where a variable in PgVersion_New() could |
|				be used before it was initialized.						|
| 12SEP2001 bga Improved detection of erronious input strings.			|
|			--- Various minor bug fixes and code cleanup.				|
|			--- Made constructed version string more closely mimic the	|
|				actual format of the PostgreSQL version string.			|
| 09SEP2001 bga Fixed the broken comparison function.  This proved to	|
|				be a not so easy task since any errors set in the ex-	|
|				tensions module's coerce() function are ignored (unless |
|				called by Python's builtin coerce() function).	Python	|
|				will clear the error and try to convert the PgVersion	|
|				object to the other type if the coersion to PgVersion	|
|				fails.	I do not want that to happen.  If the object	|
|				can't be coerced to a PgVersion, then the comparison	|
|				does not make sense, so an exception should occur.	To	|
|				get this behaviour required a bit sneaky-ness.	See the |
|				comments in the code for details.  A concequence of		|
|				this change is that coerce will always	succeed.  I can |
|				not see any way around this :-(							|
|			--- Having a __dict__ attribute and calling PyMember_Get()	|
|				in the PyVersion_getattr function causes dir() to do	|
|				strange things (like list members twice).  I've removed |
|				the __dict__ attribute and adding methods to emulate	|
|				a mapping object to PgVersion.	A PgVersion object will |
|				now act like a dictionary, so use version[key] instead	|
|				of version.__dict__[key].								|
| 08SEP2001 gh	Make PgVersion_New safe for arbitrary input strings.	|
|			--- Make the repr method really return the version string.	|
| 06SEP2001 bga Initialize a variable (value) in ver_coerce() to quite	|
|				an erroneous gcc warning message.						|
| 28AUG2001 bga Picked some lint from the code.							|
| 24AUG2001 bga Removed a variable that was no longer used/needed.		|
| 20AUG2001 bga Change code to use Py_BuildValue() for creating Python	|
|				objects from C values (where possible).					|
|			--- Change attributes that were read-only constants from a	|
|				C type to a Python Object.	This way they don't need to |
|				be converted to a Python Object each time they are ref- |
|				erenced.												|
|			--- General code clean up.	Removed some definitions that	|
|				are in the new libpqmodule.h header file.				|
| 03AUG2001 bga Change code so that memory used to duplicate strings is |
|				allocated from Python's heap.							|
| 02AUG2001 bga Initial release by Billy G. Allie.						|
\*(H-)******************************************************************/

#include <Python.h>
#include <stddef.h>
#include <ctype.h>
#include <structmember.h>
#include "libpqmodule.h"

/***************************************\
| PgVersion object definition			|
\***************************************/

typedef struct {
	PyObject_HEAD
	PyObject *version;
	PyObject *major;
	PyObject *minor;
	PyObject *patch;
	PyObject *post70;
	PyObject *value;
} PgVersion;


/***********************************************************************\
| Name:			parseToken												|
|																		|
| Synopsis:		if (parseToken(token, result)) {...error handling...}	|
|																		|
| Description:	parseToken will extract an integer from token.	If it	|
|				can't get an integer or if the token contains something |
|				other than a number, raise an execption.				|
|																		|
| Returns:		0 if successful, 1 if it failed to parse an integer.	|
\***********************************************************************/

static int parseToken(char *token, long *result)
{
	char *last;
	int i;

	/*******************************************************************\
	| For our purposes, an error occurs when parsing a token if:		|
	| 1.  the 1st character of token is not a digit.					|
	| 2.  strtol() set errno to something other than 0.					|
	| 3.  strtol() could not translate the entire token (i.e. *last is	|
	|	  not the ASCII NUL character).									|
	\*******************************************************************/

	if (!isdigit(*token))
		return 1;

	/**
	 * Only process the numeric part of the token, ignoring the 
	 * non-numeric trailing part.  This should handle development,
	 * alpha, beta, etc versions of PostgreSQL
	 */
	for (i = 1; token[i] != 0; i++) {
		if (!isdigit(token[i])) {
			token[i] = 0;
			break;
		}
	}
	errno = 0;

	*result = strtol(token, &last, 0);

	return ((errno != 0) || (*last != (char)0));
}

/*--------------------------------------------------------------------------*/

static void PgVersion_dealloc(PgVersion *self)
{
	Py_XDECREF(self->version);
	Py_XDECREF(self->major);
	Py_XDECREF(self->minor);
	Py_XDECREF(self->patch);
	Py_XDECREF(self->post70);
	Py_XDECREF(self->value);
	PyObject_DEL(self);
}

/*--------------------------------------------------------------------------*/

PyObject *PgVersion_New(char *version)
{
	PgVersion *self;
	char *s = (char *)NULL; /* writeable temporary copy of version string */
	char *vstr;				/* The version number from the version string */
	char *token;			/* parsed token */
	char *save_ptr;			/* saves pg_strtok_r state for subsequent calls */
	long major, minor, patch, value;

	self = (PgVersion *)PyObject_New(PgVersion, &PgVersion_Type);

	if (self)
	{
		save_ptr = (char *)NULL;

		self->version = Py_BuildValue("s", version);
		s = (char *)PyMem_Strdup(version);
		if ((self->version == (PyObject *)NULL) || (s == (char *)NULL))
		{
			PyErr_NoMemory();
			goto new_error;
		}

		self->major = self->minor = self->patch = (PyObject *)NULL; 
		self->value = self->post70 = (PyObject *)NULL;

		/***************************************************************\
		| Parse out the version information from the version string.	|
		| The expected format is 'PostgreSQL M.m.p on ...' where M is	|
		| the major number, m is the minor number and p is the patch	|
		| level.														|
		\***************************************************************/

		major = minor = patch = value = 0;

		/* Pre-set the error condition.
		 * We'll clear it if everything's OK
		 */
		PyErr_SetString(PyExc_ValueError,
						"Invalid format for PgVersion construction.");


		token = pg_strtok_r(s, " \t", &save_ptr);
		if (strcmp(token, "PostgreSQL") != 0)
			goto new_error;

		vstr = pg_strtok_r((char *)NULL, " \t", &save_ptr);

		token = pg_strtok_r((char *)NULL, " \t", &save_ptr);
		if (strcmp(token, "on") != 0)
			goto new_error;

		/***************************************************************\
		| This test is in case someone tries to compares against a		|
		| string such as '7.2 on'.										|
		\***************************************************************/

		token = pg_strtok_r((char *)NULL, " \t", &save_ptr);
		if (strcmp(token, "on") == 0)
			goto new_error;

		/***************************************************************\
		| We now have the version number (as M.m.p) in vstr.  Parse it. |
		| Note: The minor number and patch level may not be present.	|
		\***************************************************************/

		save_ptr = (char *)NULL;

		/***************************************************************\
		| Vendor releases e.g. from dbexperts get forgotten.			|
		| We strip that away here.										|
		\***************************************************************/

		vstr = pg_strtok_r(vstr, "-", &save_ptr);
		save_ptr = (char *)NULL;

		token = pg_strtok_r(vstr, ".", &save_ptr);
		if (parseToken(token, &major))
			goto new_error;

		token = pg_strtok_r((char *)NULL, ".", &save_ptr);
		if ((token != (char *)NULL) && (*token != '\0') &&
												(parseToken(token, &minor)))
			goto new_error;

		token = pg_strtok_r((char *)NULL, ".-", &save_ptr);
		if ((token != (char *)NULL) && (*token != '\0') &&
			(parseToken(token, &patch)))
		{
			goto new_error;
		}
		
		value = (((major * 100) + minor) * 100) + patch;

		/* OK, the version information has been parsed,
		 * Clear the pre-set error
		 */
		(void)PyErr_Clear();

		/* And build the attributes */
		self->major = Py_BuildValue("i", major);
		self->minor = Py_BuildValue("i", minor);
		self->patch = Py_BuildValue("i", patch);
		self->value = Py_BuildValue("i", value);
		self->post70 = Py_BuildValue("i", ((value >= 70100) ? 1l : 0l));
		if (PyErr_Occurred())
			goto new_error;
	}

	PyMem_Free(s);
	return (PyObject *)self;

new_error:
	PyMem_Free(s);
	PgVersion_dealloc(self);
	return (PyObject *)NULL;
}

/*--------------------------------------------------------------------------*/

static PyObject *PgVersion_repr(PgVersion *self)
{
	Py_INCREF(self->version);
	return (PyObject *)(self->version);
}

/*--------------------------------------------------------------------------*/

static int ver_coerce(PyObject **l, PyObject **r)
{
	PgVersion *s;
	char *vstr;

	if ((vstr = PyMem_Malloc(128)) == (char *)NULL)
	{
		PyErr_NoMemory();
		return (-1);
	}

	if (PyString_Check(*r))
	{
		sprintf(vstr, "PostgreSQL %.80s on UNKNOWN, compiled by UNKNOWN",
				PyString_AsString(*r));
	}
	else
	{
		long value = 0;

		if (PgInt2_Check(*r))
		{
			value = PgInt2_AsLong((PgInt2Object *)(*r));
		}
		else if (PyInt_Check(*r))
		{
			value = PyInt_AsLong(*r);
		}
#if defined(HAVE_LONG_LONG_SUPPORT)
		if (PgInt8_Check(*r))
		{
			value = PgInt8_AsLong((PgInt8Object *)(*r));
		}
#endif
		else if (PyLong_Check(*r))
		{
			value = PyLong_AsLong(*r);
		}
		else if (PyFloat_Check(*r))
		{
			double d = PyFloat_AsDouble(*r);

			if (d > (double)INT_MAX)
			{
				PyErr_SetString(PyExc_OverflowError,
								"float too large to convert");
			}
			else
				value = (long)d;
		}

		if (PyErr_Occurred())
			goto coerce_error;

		sprintf(vstr, "PostgreSQL %ld.%ld.%ld on UNKNOWN, compiled by UNKNOWN",
				(value / 10000), ((value / 100) % 100), (value % 100));
	}

	s = (PgVersion *)PgVersion_New(vstr);

	if (PyErr_Occurred())
	{
		Py_XDECREF(s);
		goto coerce_error;
	}

	PyMem_Free(vstr);

	*r = (PyObject *)s;
	Py_XINCREF(*l);

	return (0);

coerce_error:
	PyMem_Free(vstr);
	return (-1);
}

/*--------------------------------------------------------------------------*/

static int PgVersion_coerce(PyObject **l, PyObject **r)
{
	if (PgVersion_Check(*l))
		return ver_coerce(l, r);

	return ver_coerce(r, l);
}

/*--------------------------------------------------------------------------*/

#define vOFF(x) offsetof(PgVersion, x)

static struct memberlist PgVersion_members[] = {
	{ "version",   T_OBJECT,	vOFF(version),	RO },
	{ "major",	   T_OBJECT,	vOFF(major),	RO },
	{ "minor",	   T_OBJECT,	vOFF(minor),	RO },
	{ "level",	   T_OBJECT,	vOFF(patch),	RO },
	{ "post70",	   T_OBJECT,	vOFF(post70),	RO },
	{ NULL										   }
};

/*--------------------------------------------------------------------------*/

static PyObject *PgVersion_getattr(PgVersion *self, char *attr)
{
	return PyMember_Get((char *)self, PgVersion_members, attr);
}

/*--------------------------------------------------------------------------*/

static int PgVersion_setattr(PgVersion *self, char* attr, PyObject *value)
{
	if (value == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, 
						"can't delete PgVersion attributes");
		return -1;	 
	}
	return PyMember_Set((char *)self, PgVersion_members, attr, value);
}

/*--------------------------------------------------------------------------*/

static Py_ssize_t PgVersion_length(PyObject *self)
{
	return 5;
}

/*--------------------------------------------------------------------------*/

static PyObject *PgVersion_getitem(PyObject *self, PyObject *attr)
{
	char *key;
	PyObject *value;

	if (!PyArg_Parse(attr, "s", &key)) 
	{
		return (PyObject *)NULL;
	}

	value = PyMember_Get((char *)self, PgVersion_members, key);
	if (value == (PyObject *)NULL)
	{
		PyErr_SetString(PyExc_KeyError, key);
	}

	return value;
}

/*--------------------------------------------------------------------------*/

static int PgVersion_cmp(PgVersion *s, PgVersion *o)
{
	long left = PyInt_AS_LONG(s->value);
	long right = PyInt_AS_LONG(o->value);

	return (left < right) ? -1 : (left > right) ? 1 : 0;
}

/*--------------------------------------------------------------------------*/

static PyMappingMethods ver_as_mapping = {
		PgVersion_length,				/*mp_length*/
		(binaryfunc)PgVersion_getitem,			/*mp_subscript*/
		(objobjargproc)NULL,		            /*mp_ass_subscript*/
};

/*--------------------------------------------------------------------------*/

static PyNumberMethods ver_as_number = {
	(binaryfunc)NULL,			/* nb_add		*/
	(binaryfunc)NULL,			/* nb_subtract	*/
	(binaryfunc)NULL,			/* nb_multiply	*/
	(binaryfunc)NULL,			/* nb_divide	*/
	(binaryfunc)NULL,			/* nb_remainder */
	(binaryfunc)NULL,			/* nb_divmod	*/
	(ternaryfunc)NULL,			/* nb_power		*/
	(unaryfunc)NULL,			/* nb_negative	*/
	(unaryfunc)NULL,			/* nb_positive	*/
	(unaryfunc)NULL,			/* nb_absolute	*/
	(inquiry)NULL,				/* nb_nonzero	*/
	(unaryfunc)NULL,			/* nb_invert	*/
	(binaryfunc)NULL,			/* nb_lshift	*/
	(binaryfunc)NULL,			/* nb_rshift	*/
	(binaryfunc)NULL,			/* nb_and		*/
	(binaryfunc)NULL,			/* nb_xor		*/
	(binaryfunc)NULL,			/* nb_or		*/
	(coercion)PgVersion_coerce, /* nb_coerce	*/
	(unaryfunc)NULL,			/* nb_int		*/
	(unaryfunc)NULL,			/* nb_long		*/
	(unaryfunc)NULL,			/* nb_float		*/
	(unaryfunc)NULL,			/* nb_oct		*/
	(unaryfunc)NULL				/* nb_hex		*/
};

/*--------------------------------------------------------------------------*/

static char PgVersion_Type_Doc[] = "This is the type of PgVersion objects";

PyTypeObject PgVersion_Type = {
	PyObject_HEAD_INIT(NULL)
	(Py_ssize_t)NULL,						/* ob_size				*/
	MODULE_NAME ".PgVersion",			/* tp_name				*/
	sizeof(PgVersion),					/* tp_basicsize			*/
	(Py_ssize_t)NULL,						/* tp_itemsize			*/
	(destructor)PgVersion_dealloc,		/* tp_dealloc			*/
	(printfunc)NULL,					/* tp_print				*/
	(getattrfunc)PgVersion_getattr,		/* tp_getattr			*/
	(setattrfunc)PgVersion_setattr,		/* tp_setattr			*/
	(cmpfunc)PgVersion_cmp,				/* tp_compare			*/
	(reprfunc)PgVersion_repr,			/* tp_repr				*/
	&ver_as_number,						/* tp_as_number			*/
	NULL,								/* tp_as_sequence		*/
	&ver_as_mapping,					/* tp_as_mapping		*/
	(hashfunc)NULL,						/* tp_hash				*/
	(ternaryfunc)NULL,					/* tp_call				*/
	(reprfunc)NULL,						/* tp_str				*/
	NULL, NULL, NULL, (Py_ssize_t)NULL,
	PgVersion_Type_Doc
};												  

void initpgversion(void)
{
	PgVersion_Type.ob_type		= &PyType_Type;
}
