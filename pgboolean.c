//#ident "@(#) $Id: pgboolean.c,v 1.12 2003/06/17 01:34:20 ballie01 Exp $"
/*(H+)***************************************************************** \
| Name:			pgboolean.c												|
|																		|
| Description:	This file implements the PostgreSQL boolean type for	|
|				Pyhon.	This Python object is part of the libpq module. |
|																		|
| Note:			This type object is based on ideas and concepts in the	|
|				Python integer type object.								|
|=======================================================================|
| Copyright 2000 by Billy G. Allie.										|
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
| 05JUN2003 bga Change the name of the quoting function back to _quote. |
|				__*__ type names should be restrict to system names.	|
| 02NOV2002 bga Change the name of the quoting function to __quote__.	|
| 26SEP2001 bga Change the constructors so that they return PyObject *	|
|				instead of PgBooleanObject *.							|
| 06SEP2001 gh	Fix bug in PgBoolean_FromString; also improve and		|
|				simplify the string stripping in this method.			|
| 09AUG2001 bga Change code to use Py_BuildValue() for creating Python	|
|				objects from C values (where possible).					|
|			--- General code clean up.	Removed some definitions that	|
|				are in the new libpqmodule.h header file.				|
| 03AUG2001 bga Change code so that memory used to duplicate strings is |
|				allocated from Python's heap.							|
|			--- Closed a small memory leak.								|
| 06JUN2001 bga To the extent possible, I picked the "lint" from the	|
|				code.													|
| 04JUN2001 bga Changed PyObject_HEAD_INIT(&PyType_Type) to				|
|				PyObject_HEAD_INIT(NULL) to silence error produced by	|
|				some compilers.											|
| 22APR2001 bga Fixed a problem with the coerce routine that prevented	|
|				PgBoolean comparisons to None from working in Python2.1 |
| 31MAR2001 bga Corrected mis-spelling in error message.				|
| 14JUL2000 bga Added _quote(), changed output of str() and repr().		|
| 12JUL2000 bga Initial release by Billy G. Allie.						|
\*(H-)******************************************************************/

#include "Python.h"
#include <ctype.h>
#include "libpqmodule.h"

/* Standard Booleans */

PgBooleanObject _Pg_FalseStruct = {
		PyObject_HEAD_INIT(NULL)
		0
};

PgBooleanObject _Pg_TrueStruct= {
		PyObject_HEAD_INIT(NULL)
		1
};

PyObject *PgBoolean_FromLong(long v)
{
	PyObject *r = (v ? Pg_True : Pg_False);

	Py_INCREF(r);
	return r;
}

PyObject *PgBoolean_FromString(char *s)
{
	char *pstart, *p, *e;
	PyObject *b = (PyObject *)NULL;

	if ((pstart = PyMem_Strdup(s)) == (char *)NULL)
	{
		PyErr_SetString(PyExc_MemoryError,
						"out of memory in PgBoolean_FromString().");
		return (PyObject *)NULL;
	}
	p = pstart;

	while (*p && isspace(Py_CHARMASK((unsigned int)(*p))))
			p++;

	for (e = p; e < p + strlen(p); e++)
	{
		if (isspace((int)Py_CHARMASK((unsigned int)(*e))))
		{
			*e = 0;
			break;
		}
		*e = (unsigned char)toupper((int)Py_CHARMASK((unsigned int)(*e)));
	}

	switch (*p)
	{
		case '1':
			if (strcmp(p, "1") == 0)
				b = Pg_True;
			break;

		case 'T':
			if ((strcmp(p, "T") == 0) || (strcmp(p, "TRUE") == 0))
				b = Pg_True;
			break;

		case 'Y':
			if ((strcmp(p, "Y") == 0) || (strcmp(p, "YES") == 0))
				b = Pg_True;
			break;

		case 'O':
			if (strcmp(p, "ON") == 0)
				b = Pg_True;
			else if (strcmp(p, "OFF") == 0)
				b = Pg_False;
			break;

		case '0':
			if ((strcmp(p, "0") == 0))
				b = Pg_False;
			break;

		case 'F':
			if ((strcmp(p, "F") == 0) || (strcmp(p, "FALSE") == 0))
				b = Pg_False;
			break;

		case 'N':
			if ((strcmp(p, "N") == 0) || (strcmp(p, "NO") == 0))
				b = Pg_False;
			break;
	}

	PyMem_Free(pstart);

	if (b != (PyObject *)NULL)
	{
		Py_INCREF(b);
	}
	else
	{
		PyErr_SetString(PyExc_ValueError,
						"string does not represent a PostgreSQL boolean value");
	}

	return b;
}

/* Methods */

static int bool_print(PgBooleanObject *v, FILE *fp, int flags)
{
	fprintf(fp, "%c", (v->ob_ival ? 't' : 'f'));
	return 0;
}

static PyObject *bool_repr(PgBooleanObject *v)
{
	char buf[256];

	sprintf(buf, "<PgBoolean instance at %p: Value: %s>", v,
			(v->ob_ival ? "True" : "False"));
	return Py_BuildValue("s", buf);
}

static PyObject *bool_str(PgBooleanObject *v)
{
	char buf[] = { 't', (char)0 };

	buf[0] = (v->ob_ival ? 't' : 'f');
	return Py_BuildValue("s", buf);
}

static PyObject *bool_quote(PgBooleanObject *v)
{
	char buf[] = {'\'', 't', '\'', (char)0 };

	buf[1] = (v->ob_ival ? 't' : 'f');
	return Py_BuildValue("s", buf);
}

static long bool_hash(PgBooleanObject *v)
{
	return ((v->ob_ival) ? (long)0xFEFF : (long)0xFDFF);
}

static int bool_nonzero(PgBooleanObject *v)
{
	return v->ob_ival != 0;
}

static int bool_coerce(PyObject **l, PyObject **r)
{
	if PyInt_Check(*r) {
		*r = (PyObject *)(PyInt_AsLong(*r) ? Pg_True : Pg_False);
		Py_INCREF(*l);
		Py_INCREF(*r);
		return (0);
	}
	return (1);
}

static int bool_compare(PgBooleanObject *l, PgBooleanObject *r)
{
	register long i = l->ob_ival;
	register long j = r->ob_ival;

	return ((i < j) ? -1 : ((i > j) ? 1 : 0));
}

static PyNumberMethods bool_as_number = {
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
	(inquiry)bool_nonzero,		/* nb_nonzero	*/
	(unaryfunc)NULL,			/* nb_invert	*/
	(binaryfunc)NULL,			/* nb_lshift	*/
	(binaryfunc)NULL,			/* nb_rshift	*/
	(binaryfunc)NULL,			/* nb_and		*/
	(binaryfunc)NULL,			/* nb_xor		*/
	(binaryfunc)NULL,			/* nb_or		*/
	(coercion)bool_coerce,		/* nb_coerce	*/
	(unaryfunc)NULL,			/* nb_int		*/
	(unaryfunc)NULL,			/* nb_long		*/
	(unaryfunc)NULL,			/* nb_float		*/
	(unaryfunc)NULL,			/* nb_oct		*/
	(unaryfunc)NULL				/* nb_hex		*/
};

static PyMethodDef bool_methods[] = {
	{ "_quote", (PyCFunction)bool_quote, 1 },
	{ NULL, NULL }
};

static PyObject *bool_getattr(PgBooleanObject *self, char* attr)
{
	return Py_FindMethod(bool_methods, (PyObject *)self, attr);
}

PyTypeObject PgBoolean_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME ".PgBoolean",
	sizeof(PgBooleanObject),
	0,
	0,							/* tp_dealloc			*/
	(printfunc)bool_print,		/* tp_print				*/
	(getattrfunc)bool_getattr,	/* tp_getattr			*/
	0,							/* tp_setattr			*/
	(cmpfunc)bool_compare,		/* tp_compare			*/
	(reprfunc)bool_repr,		/* tp_repr				*/
	&bool_as_number,			/* tp_as_number			*/
	0,							/* tp_as_sequence		*/
	0,							/* tp_as_mapping		*/
	(hashfunc)bool_hash,		/* tp_hash				*/
	(ternaryfunc)NULL,			/* tp_call				*/
	(reprfunc)bool_str,			/* tp_str				*/
	0L, 0L, 0L, 0L,
	NULL
};

void initpgboolean(void)
{
	_Pg_FalseStruct.ob_type = &PgBoolean_Type;
	_Pg_TrueStruct.ob_type	= &PgBoolean_Type;
	PgBoolean_Type.ob_type	= &PyType_Type;
}
