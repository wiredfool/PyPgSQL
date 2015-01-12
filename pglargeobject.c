// #ident "@(#) $Id: pglargeobject.c,v 1.15 2003/06/17 01:36:42 ballie01 Exp $"
/**(H+)*****************************************************************\
| Name:			pglargeobject.c											|
|																		|
| Description:	This file contains code that implements large object	|
|				(PostgreSQL's BLOB) support for Python.					|
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
| 05JUN2003 bga Change the name of the quoting function back to _quote. |
|				__*__ type names should be restrict to system names.	|
| 02NOV2002 bga Change the name of the quoting function to __quote__.	|
| 19OCT2002 gh	Made the 'closed' attribute of PgLargeObject an int		|
|				instead of a long.										|
| 03NOV2001 bga Closed a couple of possible memory leaks.  These leaks	|
|				had a low probability of occuring.						|
| 13OCT2001 bga Added support for the pickling of large objects.		|
|				In particular, a method was added to retrieve the info	|
|				needed to recreate the large object.  Also, the open()	|
|				and close() methods were changed to create and end		|
|				transaction in the un-pickled large objects in order to |
|				create the context in which large object must be used.	|
| 24SEP2001 bga Added a _quote() funtion.								|
| 22SEP2001 bga Ensure that INV_BIN is not in the mode variable when	|
|				lo_open is called.										|
|			--- Removed static declaration from validmodes[].  It's now |
|				used by lo_create in pgconnection.c also.				|
| 06SEP2001 bga Removed an un-used variable.							|
| 30AUG2001 bga Use PyObject_Del() instead of PyMem_Free() in dealloc() |
|				to delete the object.									|
| 28AUG2001 bga Why did I introduce a new variable (save) and execute	|
|				another call to lo_tell() when it wasn't needed?  (I	|
|				have to stop working on code late at night. :-)			|
|			--- Fixed another bug in PgLo_read().  This one would only	|
|				rear it's ugly head when read() was called with no arg- |
|				uments, and then, only sometimes.  (I REALLY need to	|
|				stop working on code late at night [or is that early in |
|				the morning] :-).										|
|			--- Picked some lint from the code.							|
| 24AUG2001 bga Fixed a small bug in PgLo_read() relating to a change	|
|				that was made to fulfill small read request from the	|
|				internal buffer, if possible.							|
| 23AUG2001 bga Improved the performance of PgLo_read() by using the	|
|				internal buffer to fulfill small read request.	The way |
|				it use to work was to always read from the database		|
|				connection, ignoring the internal buffer.				|
| 06AUG2001 bga Initial release by Billy G. Allie.						|
\*(H-)******************************************************************/

#include <Python.h>
#include <stddef.h>
#include <ctype.h>
#include <structmember.h>
#include <fileobject.h>
#include "libpqmodule.h"

#define LO_DEBUG		(1)

/***************************************\
| PgLargeObject object definition		|
\***************************************/

#define PgLargeObject_Get(v) ((v)->lo_oid)

PyObject *PgLargeObject_New(PyObject *conn, Oid lo_oid, int flag)
{
	PgLargeObject *self;
	char buf[32];

	if (!PgConnection_Check(conn))
	{
		PyErr_SetString(PyExc_TypeError, "PgLargeObject_New must be given a "
										 "valid PgConnection object.");
		return (PyObject *)NULL;
	}

	self = (PgLargeObject *)PyObject_New(PgLargeObject, &PgLargeObject_Type);

	if (self)
	{
		self->lo_mode = self->lo_softspace = self->lo_offset = 0;
		self->lo_dirty = 0;
		self->lo_fd = self->lo_size = -1;
		self->lo_buf = (char *)NULL;
		self->lo_idx = MAX_BUFFER_SIZE;
		self->lo_oid = lo_oid;

		self->lo_conn = (PgConnection *)conn; Py_INCREF(conn);
		self->lo_mname = Py_None;  Py_INCREF(Py_None);
		self->lo_closed = Py_True; Py_INCREF(Py_True);

		(void)sprintf(buf, "%d", self->lo_oid);
		self->lo_name = Py_BuildValue("s", buf);
		if (PyErr_Occurred())
		{
			Py_DECREF(self);
			return (PyObject *)NULL;
		}

		if (flag)
			self->need_commit = 0;		/* Called from PgLargeObject */
		else
			self->need_commit = -1;		/* Called by getvalue() */
	}

	return (PyObject *)self;
}													 

staticforward int lo_flush(PgLargeObject *);

static void PgLargeObject_dealloc(PgLargeObject *self)
{
	PGconn		*cnx;
	int			fd;

	cnx = PgConnection_Get(self->lo_conn);
	fd = self->lo_fd;

	if (fd >= 0)
	{
		(void)lo_flush(self);
		if (lo_close(cnx, fd))
			PyErr_SetString(PyExc_IOError, "error closing PgLargeObject");
	}

	self->lo_fd = -1;
	self->lo_oid = self->lo_size = self->lo_idx = self->lo_dirty = 0;
	self->lo_mode = self->lo_softspace = 0;

	if (self->lo_buf);
	{
		PyMem_Free(self->lo_buf);
		self->lo_buf = (char *)NULL;
	}

	Py_XDECREF(self->lo_conn);
	Py_XDECREF(self->lo_name);
	Py_XDECREF(self->lo_mname);
	Py_XDECREF(self->lo_closed);

	PyObject_Del((PyObject *)self);
}					   

static PyObject *PgLargeObject_repr(PgLargeObject *self)
{
	char buf[128], *state, *mode = "";

	if (self->lo_mname != Py_None)
		mode = PyString_AsString(self->lo_mname);

	if (self->lo_fd < 0)
	{
		state = "<closed PgLargeObject %d%s at %p>";
	}
	else
	{
		state = "<open PgLargeObject %d, mode '%s' at %p>";
	}

	(void)sprintf(buf, state, self->lo_oid, mode, (void *)self);
	return Py_BuildValue("s", buf);
}

static PyObject *PgLargeObject_prnt(PgLargeObject *self)
{
	char buf[128];

	(void)sprintf(buf, "%d", self->lo_oid);
	return Py_BuildValue("s", buf);
}

static PyObject *PgLo_quote(PgLargeObject *self)
{
	return PgLargeObject_prnt(self);
}

typedef enum {
	PGRES_LO_OPENED = 1,
	PGRES_LO_CLOSED = 2,
	PGRES_LO_READ	= 4,
	PGRES_LO_WRITE	= 8
} LoStatusType;

int PgLargeObject_check(PyObject *self, int mode)
{
	PgLargeObject *lo = (PgLargeObject *)self;

	if (!PgLargeObject_Check(lo))
	{
		PyErr_SetString(PyExc_TypeError, "not a PgLargeObject");
		return FALSE;
	}

	if (!lo->lo_oid)
	{
		PyErr_SetString(PqErr_InterfaceError,
						"PgLargeObject is not valid (null oid)");
		return FALSE;
	}

	if (PgConnection_Check(lo->lo_conn))
	{
		if (PgConnection_Get(lo->lo_conn) == (PGconn *)NULL)
		{
			PyErr_SetString(PqErr_InterfaceError,
							"object references a closed PgConnection object");
			return FALSE;
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError,
						"object references an invalid PgConnection object");
		return FALSE;
	}

	if ((unsigned)mode & (unsigned)PGRES_LO_OPENED)
	{
		if (lo->lo_fd < 0)
		{
			PyErr_SetString(PqErr_InterfaceError,
							"PgLargeObject is not opened");
			return FALSE;
		}
	}

	if ((unsigned)mode & (unsigned)PGRES_LO_CLOSED)
	{
		if (lo->lo_fd >= 0)
		{
			PyErr_SetString(PqErr_InterfaceError,
							"PgLargeObject is already opened");
			return FALSE;
		}
	}

	if ((unsigned)mode & (unsigned)PGRES_LO_READ)
	{
		if (!((unsigned)lo->lo_mode & (unsigned)INV_READ))
		{
			PyErr_SetString(PqErr_InterfaceError,
							"PgLargeObject is not opened for reading");
			return FALSE;
		}
	}

	if ((unsigned)mode & (unsigned)PGRES_LO_WRITE)
	{
		if (!((unsigned)lo->lo_mode & (unsigned)INV_WRITE))
		{
			PyErr_SetString(PqErr_InterfaceError,
							"PgLargeObject is not opened for writing");
			return FALSE;
		}
	}

	return TRUE;
}

/***********************************************************************\
| Define the PgLargeObject methods.										|
\***********************************************************************/

static int lo_flush(PgLargeObject *self)
{
	if (self->lo_dirty)
	{
		PGconn	*cnx;
		int		fd;

		cnx = PgConnection_Get(self->lo_conn);
		fd = self->lo_fd;

		if (self->lo_offset != -1)
			if (lo_lseek(cnx, fd, self->lo_offset, SEEK_END) < 0)
			{
				PyErr_SetString(PyExc_IOError,
								"error seeking in PgLargeObject");
				return 1;
			}
			
		if ((lo_write(cnx, fd, self->lo_buf, self->lo_size)) < self->lo_size)
		{
			PyErr_SetString(PyExc_IOError, "error writing to PgLargeObject");
			return 1;
		}

		self->lo_dirty = self->lo_idx = self->lo_size = 0;
		self->lo_offset = -1;
	}

	return 0;
}

/*--------------------------------------------------------------------------*/

static int lo_getch(PgLargeObject *self)
{
	PGconn		*cnx;
	int			fd;

	cnx = PgConnection_Get(self->lo_conn);
	fd = self->lo_fd;

	if ((self->lo_idx >= self->lo_size))
	{
		if (lo_flush(self))
			return -2;

		self->lo_offset = lo_tell(cnx, fd);

		if ((self->lo_size =
				lo_read(cnx, fd, self->lo_buf, MAX_BUFFER_SIZE)) < 0)
		{
			PyErr_SetString(PyExc_IOError,
							"error while reading PgLargeObject");
			return -2;
		}
		self->lo_idx = 0;
	}

	if (self->lo_size == 0)
		return -1;
	else
		return (int)(self->lo_buf[self->lo_idx++]);
}

/*--------------------------------------------------------------------------*/

/* The following define is only used internally, to track if the 'b'
 * flag in the mode is seen.
 */
#define INV_BIN 0x00010000

struct {
	char *name;
	int	 mode;
} validmodes[] = {
	/* Common (normalized) mode strings */
	{ "r",		INV_READ						},
	{ "rb",		INV_READ  | INV_BIN				},
	{ "r+",		INV_READ  | INV_WRITE			},
	{ "r+b",	INV_READ  | INV_WRITE | INV_BIN },
	{ "w",		INV_WRITE						},
	{ "wb",		INV_WRITE | INV_BIN				},
	{ "w+",		INV_WRITE | INV_READ			},
	{ "w+b",	INV_WRITE | INV_READ			},

	/* The rest of the possible mode strings */
	{ "br",		INV_READ  | INV_BIN				},
	{ "+r",		INV_READ  | INV_WRITE			},
	{ "rb+",	INV_READ  | INV_WRITE | INV_BIN },
	{ "br+",	INV_READ  | INV_WRITE | INV_BIN },
	{ "+rb",	INV_READ  | INV_WRITE | INV_BIN },
	{ "+br",	INV_READ  | INV_WRITE | INV_BIN },
	{ "b+r",	INV_READ  | INV_WRITE | INV_BIN },
	{ "bw",		INV_WRITE | INV_BIN				},
	{ "+w",		INV_WRITE | INV_READ			},
	{ "wb+",	INV_WRITE | INV_READ  | INV_BIN },
	{ "bw+",	INV_WRITE | INV_READ  | INV_BIN },
	{ "+wb",	INV_WRITE | INV_READ  | INV_BIN },
	{ "+bw",	INV_WRITE | INV_READ  | INV_BIN },
	{ "b+w",	INV_WRITE | INV_READ  | INV_BIN },
	{ "rw",		INV_READ  | INV_WRITE			},
	{ "wr",		INV_READ  | INV_WRITE			},
	{ "rwb",	INV_READ  | INV_WRITE | INV_BIN },
	{ "wrb",	INV_READ  | INV_WRITE | INV_BIN },
	{ "wbr",	INV_READ  | INV_WRITE | INV_BIN },
	{ "rbw",	INV_READ  | INV_WRITE | INV_BIN },
	{ "brw",	INV_READ  | INV_WRITE | INV_BIN },
	{ "bwr",	INV_READ  | INV_WRITE | INV_BIN },
	{ NULL }
};

static char PgLo_open_Doc[] =
	"open(mode) -- Opens an PgLargeObject for reading, writing, or both "
	"as\n\t\t determined by 'mode'.";

static PyObject *PgLo_open(PgLargeObject *self, PyObject *args)
{
	PGconn		*cnx;
	Oid			oid;			/* Large Object OID		*/
	int			mode;			/* mode					*/
	char		*mname;			/* mode (as string)		*/
	int			i;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_CLOSED))
		return (PyObject *)NULL;

	mname = (char *)NULL;
	mode = 0;

	if (!PyArg_ParseTuple(args,"s:open", &mname))
	{
		PyErr_Clear();
		if (!PyArg_ParseTuple(args, "i:open", &mode))
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
		PyErr_SetString(PyExc_ValueError, "invalid mode for open()");
		return (PyObject *)NULL;
	}

	cnx = PgConnection_Get(self->lo_conn);
	oid = PgLargeObject_Get(self);

	if ((self->lo_fd = lo_open(cnx, oid, (mode & (~INV_BIN)))) < 0)
	{
		PGresult *res;

		if (self->need_commit < 0)
		{
			PyErr_SetString(PyExc_IOError, "can't open PgLargeObject");
			return (PyObject *)NULL;
		}

		Py_BEGIN_ALLOW_THREADS
		res = PQexec(cnx, "BEGIN WORK");
		Py_END_ALLOW_THREADS

		if (res == (PGresult *)NULL)
		{
			PyErr_SetString(PyExc_IOError, "can't open PgLargeObject (begin)");
			return (PyObject *)NULL;
		}

		PQclear(res);

		if ((self->lo_fd = lo_open(cnx, oid, (mode & (~INV_BIN)))) < 0)
		{
			PyErr_SetString(PyExc_IOError, "can't open PgLargeObject");
			return (PyObject *)NULL;
		}

		self->need_commit = 1;
	}

	self->lo_buf = (char *)PyMem_Realloc(self->lo_buf, MAX_BUFFER_SIZE);
	if (self->lo_buf == (char *)NULL)
	{
		PyErr_SetString(PyExc_MemoryError,
						"Can't allocate buffer in open().");
		goto lo_open_error_exit;
	}
	self->lo_dirty = self->lo_idx = self->lo_size = 0;
	self->lo_mode = mode & (~INV_BIN);

	/* Update the mode (as text) and the closed flag.
	 * Since these are Python Objects, decrement the reference count of the
	 * previous values before assigning new values to them.
	 */
	Py_XDECREF(self->lo_mname);
	self->lo_mname = Py_BuildValue("s", mname);
	if (PyErr_Occurred())
		goto lo_open_error_exit;
	Py_XDECREF(self->lo_closed);
	self->lo_closed = Py_False; Py_INCREF(Py_False);

	Py_INCREF(Py_None);
	return Py_None;

 lo_open_error_exit: 
	(void)lo_close(cnx, self->lo_fd);
	Py_XDECREF(self->lo_mname);
	self->lo_mname = Py_None; Py_INCREF(Py_None);
	if (self->lo_buf)
	{
		PyMem_Free(self->lo_buf);
		self->lo_buf = (char *)NULL;
	}
	if (self->need_commit > 0)
	{
		Py_BEGIN_ALLOW_THREADS
		PQclear(PQexec(cnx, "ROLLBACK WORK"));
		Py_END_ALLOW_THREADS
		self->need_commit = 0;
	}
	return (PyObject *)NULL;
}

/*--------------------------------------------------------------------------*/

static char PgLo_close_Doc[] =
	"close() -- Closes an opened PgLargeObject.";

static PyObject *PgLo_close(PgLargeObject *self, PyObject *args)
{
	PGconn		*cnx;
	int			rollback = 0;
	int			fd;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED))
		return (PyObject *)NULL;

	if (self->need_commit > 0)
	{
		if (!PyArg_ParseTuple(args, "|i", &rollback)) 
		{
			PyErr_SetString(PqErr_InterfaceError,
							"close() takes an optional integer parameters");
			return (PyObject *)NULL;
		}
	}
	else
	{
		if (!PyArg_ParseTuple(args, "")) 
		{
			PyErr_SetString(PqErr_InterfaceError,
							"close() takes no parameters");
			return (PyObject *)NULL;
		}
	}

	cnx = PgConnection_Get(self->lo_conn);
	fd = self->lo_fd;

	if (lo_flush(self))
		return (PyObject *)NULL;

	if (lo_close(cnx, fd))
	{
		PyErr_SetString(PyExc_IOError, "error closing PgLargeObject");
		return (PyObject *)NULL;
	}

	if (self->need_commit > 0)
	{
		Py_BEGIN_ALLOW_THREADS
		if (rollback)
			PQclear(PQexec(cnx, "ROLLBACK WORK"));
		else
			PQclear(PQexec(cnx, "COMMIT WORK"));
		Py_END_ALLOW_THREADS
		self->need_commit = 0;
	}

	self->lo_mode = self->lo_softspace = self->lo_offset = 0;
	self->lo_fd = self->lo_size = -1;
	self->lo_idx = MAX_BUFFER_SIZE;

	if (self->lo_buf != (char *)NULL);
	{
		PyMem_Free(self->lo_buf);
		self->lo_buf = (char *)NULL;
	}

	Py_XDECREF(self->lo_closed); self->lo_closed = Py_True; Py_INCREF(Py_True);
	Py_XDECREF(self->lo_mname);	 self->lo_mname = Py_None;	Py_INCREF(Py_None);

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char PgLo_read_Doc[] =
	"read(integer) -- Read from a PgLargeObject into a sized string.\n\n"
	"The PgLargeObject must be opened in read mode before calling this "
	"method.";

static PyObject *PgLo_read(PgLargeObject *self, PyObject *args) {
	PyObject	*sObj;
	PGconn		*cnx;
	char		*buf = (char *)NULL;
	int			fd, cur, end, size = 0, remaining = 0;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED|PGRES_LO_READ))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"|i:read", &size)) 
		return (PyObject *)NULL;

	if (lo_flush(self))
		return (PyObject *)NULL;

	/*******************************************************************\
	| If, after flushing the internal buffer, lo_offset is not -1 the	|
	| file position of the next byte to be read will be at lo_offset +	|
	| lo_idx.  Otherwise the file is already positioned at the correct	|
	| location.															|
	\*******************************************************************/

	cnx = PgConnection_Get(self->lo_conn);
	fd = self->lo_fd;

	if (self->lo_offset != -1)
	{
		cur = self->lo_offset + self->lo_idx;
		remaining = self->lo_size - self->lo_idx;
	}
	else
		cur = lo_tell(cnx, fd);

	if (size <= 0)
	{
		if (lo_lseek(cnx, fd, 0, SEEK_END) < 0)
		{
			PyErr_SetString(PyExc_IOError, "error seeking in PgLargeObject");
			return (PyObject *)NULL;
		}

		end = lo_tell(cnx, fd);

		if (lo_lseek(cnx, fd, cur, SEEK_SET) < 0)
		{
			PyErr_SetString(PyExc_IOError, "error seeking in PgLargeObject");
			return (PyObject *)NULL;
		}

		size = end - cur + 1;			  /* Bytes unread in large object */
	}

	/* create a Python String Object of the correct size */

	sObj = PyString_FromStringAndSize((char *)NULL, size);
	if (sObj == (PyObject *)NULL)
	{
		PyErr_SetString(PyExc_MemoryError, "Can't allocate buffer in read().");
		return (PyObject *)NULL;
	}

	/* get a pointer to the strings internal buffer */

	buf = PyString_AS_STRING((PyStringObject *)sObj);

	if (size <= remaining) /* Can we fulfill the reqeust from the buffer? */
	{	/* Yes - copy the bytes into the string */

		(void)memcpy(buf, self->lo_buf + self->lo_idx, size);
		self->lo_idx += size;
	}
	else
	{	/* No - read the bytes from the large object into the string */
		/* seek to where the reading should start */

		if (lo_lseek(cnx, fd, cur, SEEK_SET) < 0)
		{
			Py_XDECREF(sObj);
			PyErr_SetString(PyExc_IOError, "error seeking in PgLargeObject");
			return (PyObject *)NULL;
		}
		
		/* read in size bytes of the data to the string's internal buffer */
		if ((size = lo_read(cnx, fd, buf, size)) < 0)
		{
			Py_XDECREF(sObj);
			PyErr_SetString(PyExc_IOError,
							"error while reading PgLargeObject");
			return (PyObject *)NULL;
		}

		/***************************************************************\
		| We decide to do a read-ahead on the large object based on the |
		| size of the previous read request.  The size of the read re-	|
		| quest was > MAX_BUFFER_SIZE, we don't fill the buffer on the	|
		| assumption that the next request will be the same size and	|
		| will not be fulfilled from the buffer.  If the request was <	|
		| MAX_BUFFER_SIZE, we will fill the buffer on the assumption	|
		| that the next read reqeust will be small enough to be ful-	|
		| filled from the internal buffer.								|
		\***************************************************************/

		if (size < MAX_BUFFER_SIZE)
		{
			self->lo_offset = lo_tell(cnx, fd);

			if ((self->lo_size =
					lo_read(cnx, fd, self->lo_buf, MAX_BUFFER_SIZE)) < 0)
			{
				Py_XDECREF(sObj);
				PyErr_SetString(PyExc_IOError,
								"error while reading PgLargeObject");
				return (PyObject *)NULL;
			}
			self->lo_idx = 0;
		}
		else
		{
			self->lo_idx = self->lo_size = 0;
			self->lo_offset = -1;
		}
	}

	/* re-size the string in case the size changed since we calculated it. */
	_PyString_Resize(&sObj, size);

	return sObj;
}

/*--------------------------------------------------------------------------*/

static char PgLo_write_Doc[] =
	"write(buffer, size) -- Writes size characters from buffer to a PgLarge"
	"Object.\n\nThe PgLargeObject must be opened in write mode before "
	"calling this method.";

static PyObject *PgLo_write(PgLargeObject *self, PyObject *args) {
	PGconn		*cnx;
	int			fd;
	char		*buf;
	int			size;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED|PGRES_LO_WRITE))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"s#:write", &buf, &size)) 
		return (PyObject *)NULL;

	if (lo_flush(self))
		return (PyObject *)NULL;

	cnx = PgConnection_Get(self->lo_conn);
	fd = self->lo_fd;

	/*******************************************************************\
	| If, after flushing the internal buffer, lo_offset is not -1 we	|
	| have to seek to the file position of the next byte to be read.	|
	| This will be at lo_offset + lo_idx.  After seeking, we invalidate |
	| the internal buffer.												|
	\*******************************************************************/

	if (self->lo_offset != -1)
	{
		if (lo_lseek(cnx, fd, (self->lo_offset + self->lo_idx), SEEK_SET) < 0)
		{
			PyErr_SetString(PyExc_IOError, "error seeking in PgLargeObject");
			return (PyObject *)NULL;
		}
		self->lo_offset = -1;
		self->lo_size = self->lo_idx = 0;
	}

	if ((lo_write(cnx, fd, buf, size)) < size)
	{
		PyErr_SetString(PyExc_IOError, "error writing to PgLargeObject");
		return (PyObject *)NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char PgLo_lseek_Doc[] =
	"seek(offset, whence) -- Moves the current location pointer for the\n"
	"\t\t\t		PgLargeObject to a new location specified by offset.\n\n"
	"The valid values for whence are SEEK_SET, SEEK_CUR, and SEEK_END.";

static PyObject *PgLo_lseek(PgLargeObject *self, PyObject *args) {
	PGconn		*cnx;
	int			fd;
	int			offset;
	int			whence;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"ii:seek", &offset, &whence)) 
		return (PyObject *)NULL;

	if (lo_flush(self))
		return (PyObject *)NULL;

	/*******************************************************************\
	| If, after flushing the internal buffer, lo_offset is not -1 we	|
	| check to see if we are seeking a position in the internal buffer. |
	| If so, we adjust lo_idx, otherwise we invalidate the internal		|
	| buffer and execute lo_lseek.										|
	\*******************************************************************/

	if (self->lo_offset != -1 &&
		offset >= self->lo_offset &&
		offset < (self->lo_offset + self->lo_size))
	{
		self->lo_idx = offset - self->lo_offset;
	}
	else
	{
		cnx = PgConnection_Get(self->lo_conn);
		fd = self->lo_fd;

		if (lo_lseek(cnx, fd, offset, whence) < 0)
		{
			PyErr_SetString(PyExc_IOError, "error seeking in PgLargeObject");
			return (PyObject *)NULL;
		}

		self->lo_offset = -1;
		self->lo_size = self->lo_idx = self->lo_dirty = 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char PgLo_tell_Doc[] =
	"tell() -- Returns the current offset into the PgLargeObject.";

static PyObject *PgLo_tell(PgLargeObject *self, PyObject *args) {
	int			result;
	PGconn		*cnx;
	int			fd;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"tell() takes no parameters");
		return (PyObject *)NULL;
	}

	if (self->lo_offset != -1)
		result = self->lo_offset + self->lo_idx;
	else
	{
		cnx = PgConnection_Get(self->lo_conn);
		fd = self->lo_fd;
		result = lo_tell(cnx, fd);
	}

	return Py_BuildValue("i", result);
}

/*--------------------------------------------------------------------------*/

static char PgLo_export_Doc[] =
	"export(filename) -- Export a PgLargeObject to a file.";

static PyObject *PgLo_export(PgLargeObject *self, PyObject *args) {
	int			result;
	PGconn		*cnx;
	Oid			oid;
	char		*filename;

	if (!PgLargeObject_check((PyObject *)self, 0))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"s:export", &filename)) 
		return (PyObject *)NULL;

	cnx = PgConnection_Get(self->lo_conn);
	oid = PgLargeObject_Get(self);

	result = lo_export(cnx, oid, filename);

	return Py_BuildValue("i", result);
}

/***********************************************************************\
| The following methods are included to emulate a Python File object.	|
\***********************************************************************/

static char PgLo_isatty_Doc[] =
	"isatty() -- This is always 0 for a PostgreSQL large object\n\n";

static PyObject *PgLo_isatty(PgLargeObject *self, PyObject *args)
{
	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED|PGRES_LO_WRITE))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"isatty() takes no parameters");
		return (PyObject *)NULL;
	}

	return Py_BuildValue("i", 0);
}

/*--------------------------------------------------------------------------*/

static char PgLo_flush_Doc[] =
	"flush() -- flush the internal buffer of the PgLargeObject.\n\n";

static PyObject *PgLo_flush(PgLargeObject *self, PyObject *args)
{
	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED|PGRES_LO_WRITE))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"flush() takes no parameters");
		return (PyObject *)NULL;
	}

	if (lo_flush(self))
		return (PyObject *)NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static char PgLo_fileno_Doc[] =
	"fileno() -- returns the file discriptor associated with the large object\n\n";

static PyObject *PgLo_fileno(PgLargeObject *self, PyObject *args)
{
	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED|PGRES_LO_WRITE))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"")) 
	{
		PyErr_SetString(PqErr_InterfaceError,
						"fileno() takes no parameters");
		return (PyObject *)NULL;
	}

	return Py_BuildValue("i", self->lo_fd);
}

/*--------------------------------------------------------------------------*/

static char PgLo_readline_Doc[] =
	"readline(size) -- reads a single line from a PgLargeObject.\n\n"
	"The PgLargeObject must be opened in read mode before calling this "
	"method.";

static PyObject *PgLo_readline(PgLargeObject *self, PyObject *args) {
	PyObject	*stringObject;
	int			c, size, idx, arg0 = 0;
	char		*buf = (char *)NULL;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED|PGRES_LO_READ))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"|i:readline", &arg0)) 
		return (PyObject *)NULL;

	if (arg0 > 0)
	{
		size = arg0;
		if ((buf = (char *)PyMem_Realloc(buf, size)) == (char *)NULL)
		{
			PyErr_SetString(PyExc_MemoryError,
							"Can't allocate buffer in readline().");
			return (PyObject *)NULL;
		}

		for (idx = 0; idx < size; idx++)
		{
			if ((c = lo_getch(self)) == -2)
			{
				PyMem_Free(buf);
				return (PyObject *)NULL;
			}

			if (c == -1)
				break;

			buf[idx] = (char)c;

			if (c == '\n')
				break;
		}
	}
	else
	{
		size = MAX_BUFFER_SIZE;
		if ((buf = (char *)PyMem_Realloc(buf, size)) == (char *)NULL)
		{
			PyErr_SetString(PyExc_MemoryError,
							"Can't allocate buffer in readline().");
			return (PyObject *)NULL;
		}

		for (idx = 0, c = lo_getch(self); c > 0; c = lo_getch(self))
		{
			if (idx >= size)
			{
				size += MAX_BUFFER_SIZE;
				if ((buf = (char *)PyMem_Realloc(buf, size)) == (char *)NULL)
				{
					PyErr_SetString(PyExc_MemoryError,
									"Can't allocate buffer in readline().");
					return (PyObject *)NULL;
				}
			}

			buf[idx++] = (char)c;

			if (c == '\n')
				break;
		}

		size = idx;

		if (c == -2)
		{
			PyMem_Free(buf);
			return (PyObject *)NULL;
		}
	}

	stringObject = Py_BuildValue("s#", buf, size);

	PyMem_Free(buf);

	return stringObject;
}

/*--------------------------------------------------------------------------*/

static char PgLo_readlines_Doc[] =
	"readlines(sizehint) -- read until EOF using readline().\n\n"
	"The PgLargeObject must be opened in read mode before calling this "
	"method.\nIf sizehint is present, instead of reading upto EOF, whole "
	"lines totaling approximately sizehint bytes are read.";

static PyObject *PgLo_readlines(PgLargeObject *self, PyObject *args) {
	PyObject	*listObject, *item, *myargs;
	int			size = 0, arg0 = -1, sz;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED|PGRES_LO_READ))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"|i:readline", &arg0)) 
		return (PyObject *)NULL;

	if ((listObject = PyList_New(0)) == (PyObject *)NULL)
		return (PyObject *)NULL;

	if ((myargs = Py_BuildValue("()")) == (PyObject *)NULL)
	{
		Py_DECREF(listObject);
		return (PyObject *)NULL;
	}

	for (;;)
	{
		if (!(item = PgLo_readline(self, myargs)))
		{
			Py_DECREF(listObject);
			Py_DECREF(myargs);
			return (PyObject *)NULL;
		}

		if ((sz = PyString_Size(item)) == 0)
			break;

		size += sz;

		if (PyList_Append(listObject, item))
		{
			Py_XDECREF(item);
			Py_XDECREF(listObject);
			Py_XDECREF(myargs);
			return (PyObject *)NULL;
		}

		if (arg0 > 0 && size > arg0)
			break;
	}

	Py_XDECREF(myargs);

	return listObject;
}

/*--------------------------------------------------------------------------*/

static char PgLo_writelines_Doc[] =
	"writelines(list) -- Write a list of strings to a PgLargeObject.\n\n"
	"The PgLargeObject must be opened in write mode before calling this "
	"method.\nThe name is intended to match readlines(); writelines() does "
	"not add line seperators.";

static PyObject *PgLo_writelines(PgLargeObject *self, PyObject *args) {
	PyObject	*list, *item;
	PyObject	*(*getitem)(PyObject *, Py_ssize_t);
	PGconn		*cnx;
	int			fd;
    Py_ssize_t  size, lsize, i;
	char		*buf;

	if (!PgLargeObject_check((PyObject *)self, PGRES_LO_OPENED|PGRES_LO_WRITE))
		return (PyObject *)NULL;

	if (!PyArg_ParseTuple(args,"O:writelines", &list)) 
		return (PyObject *)NULL;

	if (PyTuple_Check(list)) /*EMPTY*/
	{
		getitem = PyTuple_GetItem;
		lsize = PyTuple_Size(list);
	}
	else if (PyList_Check(list))
	{
		getitem = PyList_GetItem;
		lsize = PyList_Size(list);
	}
	else
	{
		PyErr_SetString(PyExc_TypeError,
					"writelines() requires a list of strings as an argument");
		return (PyObject *)NULL;
	}

	if (lo_flush(self))
		return (PyObject *)NULL;

	cnx = PgConnection_Get(self->lo_conn);
	fd = self->lo_fd;

	/*******************************************************************\
	| If, after flushing the internal buffer, lo_offset is not -1 we	|
	| have to seek to the file position of the next byte to be read.	|
	| This will be at lo_offset + lo_idx.  After seeking, we invalidate |
	| the internal buffer.												|
	\*******************************************************************/

	if (self->lo_offset != -1)
	{
		if (lo_lseek(cnx, fd, (self->lo_offset + self->lo_idx), SEEK_SET) < 0)
		{
			PyErr_SetString(PyExc_IOError, "error seeking in PgLargeObject");
			return (PyObject *)NULL;
		}
		self->lo_offset = -1;
		self->lo_size = self->lo_idx = 0;
	}

	for (i = 0; i < lsize; i++)
	{
		item = getitem(list, i);		/* Borrowed Reference */
		if (!PyString_Check(item))
		{
			PyErr_SetString(PyExc_TypeError,
					"writelines() requires a list of strings as an argument");
			return (PyObject *)NULL;
		}

		buf = PyString_AsString(item);
		size = PyString_Size(item);

		if ((lo_write(cnx, fd, buf, (size_t)size)) < size)
		{
			PyErr_SetString(PyExc_IOError, "error writing to PgLargeObject");
			return (PyObject *)NULL;
		}

	}

	Py_INCREF(Py_None);
	return Py_None;
}

/*--------------------------------------------------------------------------*/

static PyObject *PgLo_pickle(PyObject *self)
{
	PgLargeObject *lo;
	PyObject	  *res;
	int			  offset = 0;


	if (!PgLargeObject_Check(self))
	{
		PyErr_SetString(PyExc_TypeError, "not a PgLargeObject");
		return (PyObject *)NULL;
	}
	
	lo = (PgLargeObject *)self;

	if (lo->lo_closed != Py_True)
	{
		if (lo_flush(lo))
			return (PyObject *)NULL;
		if (lo->lo_offset == -1)
			offset = lo_tell(PgConnection_Get(lo->lo_conn), lo->lo_fd);
		else
			offset = lo->lo_offset + lo->lo_idx;
	}

	if (lo->lo_closed == Py_True)
		res = Py_BuildValue("(Oisii)", (lo->lo_conn)->cinfo, lo->lo_oid, "",
									   lo->lo_softspace, offset);
	else
		res = Py_BuildValue("(OiOii)", (lo->lo_conn)->cinfo, lo->lo_oid,
									   lo->lo_mname, lo->lo_softspace, offset);
	return res;
}

/*--------------------------------------------------------------------------*/

#define LoOFF(x) offsetof(PgLargeObject, x)

/* PgLargeObject object members */
static struct memberlist PgLargeObject_members[] = {
	{ "closed",		 T_OBJECT,	LoOFF(lo_closed),		RO },
	{ "mode",		 T_OBJECT,	LoOFF(lo_mname),		RO },
	{ "name",		 T_OBJECT,	LoOFF(lo_name),			RO },
	{ "softspace",	 T_INT,		LoOFF(lo_softspace)		   },
#if defined(LO_DEBUG)
	{ "conn",		 T_OBJECT,	LoOFF(lo_conn),			RO },
	{ "oid",		 T_INT,		LoOFF(lo_oid),			RO },
	{ "fd",			 T_INT,		LoOFF(lo_fd),			RO },
	{ "imode",		 T_INT,		LoOFF(lo_mode),			RO },
	{ "dirty",		 T_INT,		LoOFF(lo_dirty),		RO },
	{ "offset",		 T_INT,		LoOFF(lo_offset),		RO },
	{ "buffer",		 T_STRING,	LoOFF(lo_buf),			RO },
	{ "size",		 T_INT,		LoOFF(lo_size),			RO },
	{ "index",		 T_INT,		LoOFF(lo_idx),			RO },
	{ "need_commit", T_INT,		LoOFF(need_commit),		RO },
#endif
	{ NULL												   }
};

/*--------------------------------------------------------------------------*/

/* PgLargeObject object methods */
static PyMethodDef PgLargeObject_methods[] = {
	{ "close",			(PyCFunction)PgLo_close,		1, PgLo_close_Doc},
	{ "flush",			(PyCFunction)PgLo_flush,		1, PgLo_flush_Doc},
	{ "open",			(PyCFunction)PgLo_open,			1, PgLo_open_Doc},
	{ "isatty",			(PyCFunction)PgLo_isatty,		1, PgLo_isatty_Doc},
	{ "fileno",			(PyCFunction)PgLo_fileno,		1, PgLo_fileno_Doc},
	{ "read",			(PyCFunction)PgLo_read,			1, PgLo_read_Doc},
	{ "readline",		(PyCFunction)PgLo_readline,		1, PgLo_readline_Doc},
	{ "readlines",		(PyCFunction)PgLo_readlines,	1, PgLo_readlines_Doc},
	{ "seek",			(PyCFunction)PgLo_lseek,		1, PgLo_lseek_Doc},
	{ "tell",			(PyCFunction)PgLo_tell,			1, PgLo_tell_Doc},
	{ "write",			(PyCFunction)PgLo_write,		1, PgLo_write_Doc},
	{ "writelines",		(PyCFunction)PgLo_writelines,	1, PgLo_writelines_Doc},
	{ "export",			(PyCFunction)PgLo_export,		1, PgLo_export_Doc},
	{ "_quote",			(PyCFunction)PgLo_quote,		1},
	{ "_pickle",		(PyCFunction)PgLo_pickle,		1},
	{ NULL, NULL }
};

static PyObject *PgLargeObject_getattr(PgLargeObject *self, char* attr)
{
	PyObject *res;

	res = Py_FindMethod(PgLargeObject_methods, (PyObject *)self, attr);
	if (res != NULL)
		return res;
	PyErr_Clear();

	if (strcmp(attr, "closed") == 0)
		return Py_BuildValue("i", (self->lo_fd == -1));

	if (!strcmp(attr, "__module__"))
		return Py_BuildValue("s", MODULE_NAME);

	if (!strcmp(attr, "__class__"))
		return Py_BuildValue("s", self->ob_type->tp_name);

	return PyMember_Get((char *)self, PgLargeObject_members, attr);
}

static int PgLargeObject_setattr(PgLargeObject *self, char* attr,
								 PyObject *value)
{
	if (value == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, 
						"can't delete PgLargeObject attributes");
		return -1;	 
	}
	return PyMember_Set((char *)self, PgLargeObject_members, attr, value);
}

/*--------------------------------------------------------------------------*/

static char PgLargeObject_Type_Doc[] = "This is the type of PgLargeObjects";

PyTypeObject PgLargeObject_Type = {
	PyObject_HEAD_INIT(NULL)
	(Py_ssize_t)NULL,							/* ob_size */
	MODULE_NAME ".PgLargeObject",		/* tp_name */
	sizeof(PgLargeObject),				/* tp_basicsize */
	(Py_ssize_t)NULL,							/* tp_itemsize */
	(destructor)PgLargeObject_dealloc,	/* tp_dealloc */
	(printfunc)NULL,					/* tp_print */
	(getattrfunc)PgLargeObject_getattr, /* tp_getattr */
	(setattrfunc)PgLargeObject_setattr, /* tp_setattr */
	NULL,								/* tp_compare */
	(reprfunc)PgLargeObject_repr,		/* tp_repr */
	NULL,								/* tp_as_number */			   
	NULL,								/* tp_as_sequence */
	NULL,								/* tp_as_mapping */
	(hashfunc)NULL,						/* tp_hash */
	(ternaryfunc)NULL,					/*tp_call*/
	(reprfunc)PgLargeObject_prnt,		/*tp_str*/
	NULL, NULL, NULL, (Py_ssize_t)NULL,
	PgLargeObject_Type_Doc
};

void initpglargeobject(void)
{
	PgLargeObject_Type.ob_type	= &PyType_Type;
}
