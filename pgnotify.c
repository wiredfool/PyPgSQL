// #ident "@(#) $Id: pgnotify.c,v 1.8 2003/07/14 21:27:38 ballie01 Exp $"
/* vi:set sw=4 ts=8 showmode ai: */
/**(H+)*****************************************************************\
| Name:			pgnotify.c												|
|																		|
| Description:	This file contains code to implement the backend notify |
|				object in Python.										|
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
| 21MAY2012 eds Added support for extra parameter from notify           |
| 16AUG2011 eds reordered & removed redundant includes to kill warnings |
| 28MAY2003 bga Fixed a bug in the code.  The code in question use to	|
|				work, but doesn't anymore (possible change to libpq?).	|
| 06SEP2001 bga Clarified an embedded assignment within an if test.		|
| 30AUG2001 bga Use PyObject_Del() instead of PyMem_Free() in dealloc() |
|				to delete the object.									|
| 28AUG2001 bga Changed calls to PyObject_DEL() to	Py_XDECREF() when	|
|				deleting objects embedded in my extension objects.		|
|				This corrects a problem expirenced on MS Windows.		|
|			--- Picked some lint from the code.							|
| 20AUG2001 bga Constant, read-only attributes are now stored as Python |
|				objects.  This way they do not have to be created each	|
|				time they are referenced.								|
| 06AUG2001 bga Initial release by Billy G. Allie.						|
\*(H-)******************************************************************/

#include <Python.h>
#include <stddef.h>
#include <ctype.h>
#include <structmember.h>
#include <fileobject.h>
#include "libpqmodule.h"

PyObject *PgNotify_New(PGnotify *note)
{
	PgNotify *self = (PgNotify *)NULL;

	if (note == (PGnotify *)NULL)
	{
		Py_INCREF(Py_None);
		self = (PgNotify *)Py_None;
	}
	else if ((self = (PgNotify *)PyObject_New(PgNotify, &PgNotify_Type)) != 
			 (PgNotify *)NULL)
	{
		self->relname = Py_BuildValue("s", note->relname);
		self->be_pid = Py_BuildValue("i", note->be_pid);
		self->extra = Py_BuildValue("s", note->extra);
		free(note);
		if (PyErr_Occurred())
		{
			Py_XDECREF(self->relname);
			Py_XDECREF(self->be_pid);
			Py_XDECREF(self->extra);
			PyObject_Del(self);
			return (PyObject *)NULL;
		}
	}

	return (PyObject *)self;
}

static void PgNotify_dealloc(PgNotify *self)
{
	Py_XDECREF(self->relname);
	Py_XDECREF(self->be_pid);
	Py_XDECREF(self->extra);

	PyObject_Del((PyObject *)self);
}
						
static PyObject *PgNotify_repr(PgNotify *self)
{
	char buf[100];

	(void)sprintf(buf, "<PgNotify instance at %p of %s from backend pid %ld>",
					   (void *)self, PyString_AsString(self->relname),
					   PyInt_AsLong(self->be_pid));

	return Py_BuildValue("s", buf);
}

/***********************************************************************\
| Define the PgNotify methods.											|
\***********************************************************************/

/* THERE ARE NO PgNotify METHODS! */

#define NoOFF(x) offsetof(PgNotify, x)

static struct memberlist PgNotify_members[] = {
	{ "relname",   T_OBJECT,	NoOFF(relname),			RO },
	{ "be_pid",	   T_OBJECT,	NoOFF(be_pid),			RO },
	{ "extra",	   T_OBJECT,	NoOFF(extra),			RO },
	{ NULL												   }
};

/*--------------------------------------------------------------------------*/
 
static PyObject *PgNotify_getattr(PgNotify *self, char* attr)
{
	if (!strcmp(attr, "__module__"))
		return Py_BuildValue("s", MODULE_NAME);

	if (!strcmp(attr, "__class__"))
		return Py_BuildValue("s", self->ob_type->tp_name);

	return PyMember_Get((char *)self, PgNotify_members, attr);
}

/*--------------------------------------------------------------------------*/

static char PgNotify_Type_Doc[] = "This is the type of PgNotify objects";

PyTypeObject PgNotify_Type = {
	PyObject_HEAD_INIT(NULL)
	(int)NULL,							/*ob_size*/
	MODULE_NAME ".PgNotify",			/*tp_name*/
	sizeof(PgNotify),					/*tp_basicsize*/
	(int)NULL,							/*tp_itemsize*/
	(destructor)PgNotify_dealloc,		/*tp_dealloc*/
	(printfunc)NULL,					/*tp_print*/
	(getattrfunc)PgNotify_getattr,		/*tp_getattr*/
	(setattrfunc)NULL,					/*tp_setattr*/
	NULL,								/*tp_compare*/
	(reprfunc)PgNotify_repr,			/*tp_repr*/
	NULL,								/*tp_as_number*/
	NULL,								/*tp_as_sequence*/
	NULL,								/*tp_as_mapping*/
	(hashfunc)NULL,						/*tp_hash*/
	(ternaryfunc)NULL,					/*tp_call*/
	(reprfunc)NULL,						/*tp_str*/
	NULL, NULL, NULL, (int)NULL,
	PgNotify_Type_Doc
};

void initpgnotify(void)
{
	PgNotify_Type.ob_type		= &PyType_Type;
}
