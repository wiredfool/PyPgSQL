// #ident "@(#) $Id: pgint8object.c,v 1.15 2003/11/10 05:12:52 ballie01 Exp $"
/**(H+)*****************************************************************\
| Name:			pgint8object.c											|
|																		|
| Description:	This file implements the PostgreSQL int8 type for		|
|				Pyhon.	This Python object is part of the libpq module. |
|																		|
| Note:			This type object is based on ideas and concepts in the	|
|				Python integer and Python long type object.				|
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
| People who have worked on this code.									|
|																		|
| Ini Name																|
| --- ----------------------------------------------------------------- |
|  gh Gerhard Haering <gerhard.haering@gmx.de>							|
| bga Billy G. Allie <bill.allie@mug.org>								|
|=======================================================================|
| Revision History:														|
|																		|
| Date		Ini Description												|
| --------- --- ------------------------------------------------------- |
| 16AUG2011 eds reordered & removed redundant includes to kill warnings |
| 09NOV2003 bga Fixed problem when compiled against Windows++MSVC6.		|
|				Based on code by Mike C. Fletcher.	[BUG #829744].		|
| 02OCT2002 gh	LONG_LONG_MAX => LLONG_MAX								|
| 01OCT2002 gh	HAVE_LONG_LONG => HAVE_LONG_LONG_SUPPORT				|
| 16SEP2002 gh	Reflect the change from windows/ to port/.				|
| 01OCT2001 bga Changed all new style comments to original style.		|
| 26SEP2001 bga Change the constructors so that they return PyObject *	|
|				instead of PgInt8Object *.								|
| 03SEP2001 gh	Modified code for 64bit (long long) support in the MS	|
|				Windows environment with MS Visual C++.					|
|			bga Made changes to avoid use of long long constants.  This |
|				was done to assist in the use of MS Visual C++, which	|
|				uses something other than LL to specify long long con-	|
|				tants. (It's ugly, I know.	Thanks MS.)					|
| 30AUG2001 bga Use PyObject_Del() instead of PyMem_Free() in dealloc() |
|				to delete the object.									|
| 11AUG2001 bga Change code to use Py_BuildValue() for creating Python	|
|				objects from C values (where possible).					|
|			--- General code clean up.	Removed some definitions that	|
|				are in the new libpqmodule.h header file.				|
| 03AUG2001 bga Changed 'PyMem_DEL()' to 'PyMem_Free()'.				|
| 14JUN2001 bga Change #if defined(LONG_LONG) to reference the macro	|
|				HAVE_LONG_LONG.											|
| 09JUN2001 bga This file did not build under Python 2.0.  It does now. |
| 06JUN2001 bga To the extent possible, I picked the "lint" from the	|
|				code.													|
| 04JUN2001 bga Changed PyObject_HEAD_INIT(&PyType_Type) to				|
|				PyObject_HEAD_INIT(NULL) to silence error produced by	|
|				some compilers.											|
| 31MAY2001 bga Initial release by Billy G. Allie.						|
\*(H-)******************************************************************/

#include "Python.h"
#include <ctype.h>
#include "libpqmodule.h"

#if defined(HAVE_LONG_LONG_SUPPORT)

static PyObject *err_ovf(char *msg)
{
	PyErr_SetString(PyExc_OverflowError, msg);
	return NULL;
}

static void int8_dealloc(PgInt8Object *v)
{
	PyObject_Del(v);
}

LONG_LONG PgInt8_GetMax(void)
{
	return LLONG_MAX;
}

LONG_LONG PgInt8_AsLongLong(register PgInt8Object *op)
{
	if (op && PgInt8_Check(op))
		return PgInt8_AS_LONGLONG((PgInt8Object *)op);
		
	PyErr_SetString(PyExc_TypeError, "a PgInt8 is required");
	return (LONG_LONG)-1;
}

long PgInt8_AsLong(register PgInt8Object *op)
{
	LONG_LONG val;
	long ival;
		
	if (op && PgInt8_Check(op))
	{
		val = PgInt8_AS_LONGLONG(op);
		ival = (long)val;
		if ((LONG_LONG)ival != val)
		{
			PyErr_SetString(PyExc_OverflowError,
							"PgInt8 too large to convert");
			return -1L;
		}
		return ival;
	}
		
	PyErr_SetString(PyExc_TypeError, "a PgInt8 is required");
	return -1L;
}

PyObject *PgInt8_FromLongLong(LONG_LONG ival)
{
	register PyObject *v;

	if ((v = (PyObject *)PyObject_New(PgInt8Object, &PgInt8_Type)) ==
		(PyObject *)NULL)
	{
		return v;
	}
	((PgInt8Object *)v)->ob_ival = ival;

	return v;
}

PyObject *PgInt8_FromLong(long ival)
{
	register PyObject *v;

	if ((v = (PyObject *)PyObject_New(PgInt8Object, &PgInt8_Type)) ==
		(PyObject *)NULL)
	{
		return v;
	}
	((PgInt8Object *)v)->ob_ival = (LONG_LONG)ival;

	return v;
}

PyObject *PgInt8_FromString(char *s, char **pend, int base)
{
	char *end;
	LONG_LONG x;
	char buffer[256]; /* For errors */

	if ((base != 0 && base < 2) || base > 36)
	{
		PyErr_SetString(PyExc_ValueError,
						"PgInt8() base must be >= 2 and <= 36");
		return NULL;
	}

	while (*s && isspace(Py_CHARMASK((unsigned int)(*s))))
		s++;
	errno = 0;
	if (base == 0 && s[0] == '0')
		x = pg_strtoull(s, &end, base);
	else
		x = pg_strtoll(s, &end, base);
	if (end == s || !isalnum(Py_CHARMASK((unsigned int)(end[-1]))))
		goto bad;
	while (*end && isspace(Py_CHARMASK((unsigned int)(*end))))
		end++;
	if (*end != '\0') {
bad:
		sprintf(buffer, "invalid literal for PgInt8(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return (PyObject *)NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "PgInt8() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return (PyObject *)NULL;
	}
	if (pend)
		*pend = end;
	return PgInt8_FromLongLong(x);
}

PyObject *PgInt8_FromUnicode(Py_UNICODE *s, int length, int base)
{
	char buffer[256];
	
	if (length >= sizeof(buffer)) {
		PyErr_SetString(PyExc_ValueError,
						"int() literal too large to convert");
		return (PyObject *)NULL;
	}
	if (PyUnicode_EncodeDecimal(s, length, buffer, NULL))
		return (PyObject *)NULL;

	return PgInt8_FromString(buffer, NULL, base);
}

static int convert_binop(PyObject *v, PyObject *w, LONG_LONG *a, LONG_LONG *b)
 {
	 if (PgInt8_Check(v))
	 { 
		 *a = PgInt8_AS_LONGLONG(v);
	 }
	 else if (PyLong_Check(v))
	 { 
		 *a = PyLong_AsLongLong(v);

		 if (*a == -1 && PyErr_Occurred())
			 return 0;
	 }
	 else if (PyInt_Check(v)) {
		 *a = (LONG_LONG)(PyInt_AS_LONG(v));
	 }
	 else
	 {
		 return 0;
	 }

	 if (w == Py_None)
		 return 1;
	 else if (PgInt8_Check(w))
	 { 
		 *b = PgInt8_AS_LONGLONG(w);
	 }
	 else if (PyLong_Check(w))
	 {
		 *b = PyLong_AsLongLong(w);

		 if (*b == -1 && PyErr_Occurred())
			 return 0;
	 }
	 else if (PyInt_Check(w))
	 {
		 *b = (LONG_LONG)(PyInt_AS_LONG(w));
	 }
	 else
	 {
		 return 0;
	 }

	 return 1;
}

#if (PY_VERSION_HEX >= 0x02010000)
#define CONVERT_BINOP(v, w, a, b) \
		if (!convert_binop(v, w, a, b)) { \
				Py_INCREF(Py_NotImplemented); \
				return Py_NotImplemented; \
		}

#define PY_NOT_IMPLEMENTED \
		Py_INCREF(Py_NotImplemented); \
		return Py_NotImplemented;
#else
#define CONVERT_BINOP(v, w, a, b) \
		if (!convert_binop(v, w, a, b)) { \
			PyErr_SetString(PyExc_NotImplementedError, \
							"operation not implemented for PgInt8 type"); \
			return NULL; \
		}

#define PY_NOT_IMPLEMENTED \
		PyErr_SetString(PyExc_NotImplementedError, \
						"operation not implemented for PgInt8 type"); \
		return NULL; \

#define Py_TPFLAGS_CHECKTYPES 0
#endif

#define CONVERT_TO_LONGLONG(v, a) \
				CONVERT_BINOP(v, Py_None, a, (LONG_LONG *)NULL)

/* Methods */

/* ARGSUSED */
static int int8_print(PgInt8Object *v, FILE *fp, int flags)
	 /* flags -- not used but required by interface */
{
#if defined(_MSC_VER)
	fprintf(fp, "%I64d", PgInt8_AS_LONGLONG(v));
#else
	fprintf(fp, "%lld", PgInt8_AS_LONGLONG(v));
#endif
	return 0;
}

static PyObject *int8_repr(PgInt8Object *v)
{
	char buf[32];
#if defined(_MSC_VER)
	sprintf(buf, "%I64d", PgInt8_AS_LONGLONG(v));
#else
	sprintf(buf, "%lld", PgInt8_AS_LONGLONG(v));
#endif
	return Py_BuildValue("s", buf);
}

static int int8_compare(PgInt8Object *v, PgInt8Object *w)
{
	register LONG_LONG i = PgInt8_AS_LONGLONG(v);
	register LONG_LONG j = PgInt8_AS_LONGLONG(w);
	
	return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static long int8_hash(PgInt8Object *v)
{
	LONG_LONG x = PgInt8_AS_LONGLONG(v);

	/* Have PgInt8s has to the same value as Ints for the same value */
	if (!(((LONG_LONG)LONG_MIN < x) && (x <= (LONG_LONG)LONG_MAX)))
	{
		x = (((unsigned LONG_LONG)x >> 32U) + (unsigned LONG_LONG)x) &
			 0x7fffffffU;
	}

	if (x == (LONG_LONG)-1)
		x = (LONG_LONG)-2;

	return (long)x;
}

static int int8_coerce(PyObject **pv, PyObject **pw)
{
	if (PgInt8_Check(*pv))
	{
		if (PyInt_Check(*pw))
		{
			*pw = (PyObject *)PgInt8_FromLong(PyInt_AS_LONG(*pw));
			Py_INCREF(*pv);
		}
		else if (PyLong_Check(*pw))
		{
			*pv = (PyObject *)PyLong_FromLongLong(PgInt8_AS_LONGLONG(*pv));
			Py_INCREF(*pw);
		}
		else if (PyFloat_Check(*pw))
		{
		  *pv = (PyObject *)PyFloat_FromDouble((double)PgInt8_AS_LONGLONG(*pv));
			Py_INCREF(*pw);
		}
		else if (PyComplex_Check(*pw))
		{
			*pv = (PyObject *)PyComplex_FromDoubles(
									(double)PgInt8_AS_LONGLONG(*pv), (double)0);
			Py_INCREF(*pw);
		}
		else
			return 1;

		return 0;
	}
	else if (PgInt8_Check(*pw)) {
		if (PyInt_Check(*pv))
		{
			*pv = (PyObject *)PgInt8_FromLong(PyInt_AS_LONG(*pv));
			Py_INCREF(*pv);
		}
		else if (PyLong_Check(*pv))
		{
			*pw = (PyObject *)PyLong_FromLongLong(PgInt8_AS_LONGLONG(*pw));
			Py_INCREF(*pw);
		}
		else if (PyFloat_Check(*pv))
		{
		  *pw = Py_BuildValue("d", (double)PgInt8_AS_LONGLONG(*pw));
			Py_INCREF(*pw);
		}
		else if (PyComplex_Check(*pv))
		{
			*pw = (PyObject *)PyComplex_FromDoubles(
									(double)PgInt8_AS_LONGLONG(*pw), (double)0);
			Py_INCREF(*pv);
		}
		else
			return 1;

		return 0;
	}

	return 1; /* Can't do it */
}

static PyObject *int8_add(PyObject *v, PyObject *w)
{
	LONG_LONG a, b, x;
	PyObject *v1 = v, *w1 =w;

	if (!(PgInt8_Check(v) && PgInt8_Check(w)))
	{
		/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
		/* the numbers to the same type.								*/

		if (int8_coerce(&v1, &w1))
		{
			PY_NOT_IMPLEMENTED
		}

		/* Now that the numbers are the same type, we check to see if	*/
		/* they are PgInt8s.  If not, we call the add method of the		*/
		/* type that they were coerced into.							*/

		if (!PgInt8_Check(v1))
		{
			PyNumberMethods *nb;

			nb = v1->ob_type->tp_as_number;
			if (nb && nb->nb_add)
				return (PyObject *)(*nb->nb_add)(v1, w1);
			else
			{
				PY_NOT_IMPLEMENTED
			}
		}
	}

	/* We are adding two PgInt8s. */

	CONVERT_BINOP(v1, w1, &a, &b);

	x = a + b;

	if ((((unsigned LONG_LONG)x ^ (unsigned LONG_LONG)a) < 0) &&
		(((unsigned LONG_LONG)x ^ (unsigned LONG_LONG)b) < 0))
		return err_ovf("PgInt8 addition");

	return (PyObject *)PgInt8_FromLongLong(x);
}

static PyObject *int8_sub(PyObject *v, PyObject *w)
{
	LONG_LONG a, b, x;
	PyObject *v1 = v, *w1 = w;

	if (!(PgInt8_Check(v) && PgInt8_Check(w)))
	{
		/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
		/* the numbers to the same type.								*/

		if (int8_coerce(&v1, &w1))
		{
			PY_NOT_IMPLEMENTED
		}

		/* Now that the numbers are the same type, we check to see if	*/
		/* they are PgInt8s.  If not, we call the subtract method of	*/
		/* the type that they were coerced into.						*/

		if (!PgInt8_Check(v1))
		{
			PyNumberMethods *nb;

			nb = v1->ob_type->tp_as_number;
			if (nb && nb->nb_subtract)
				return (PyObject *)(*nb->nb_subtract)(v1, w1);
			else
			{
				PY_NOT_IMPLEMENTED
			}
		}
	}

	/* We are subtracting two PgInt8s. */
	
	CONVERT_BINOP(v1, w1, &a, &b);
	
	x = a - b;

	if ((((unsigned LONG_LONG)x ^ (unsigned LONG_LONG)a) < 0) &&
		(((unsigned LONG_LONG)x ^ (unsigned LONG_LONG)b) < 0))
		return err_ovf("PgInt8 subtraction");

	return (PyObject *)PgInt8_FromLongLong(x);
}

static PyObject *PgInt8_repeat(PyObject *v, PyObject *w)
{
	/* sequence * long */
	long n = PgInt8_AsLong((PgInt8Object *)w);

	if (n == -1 && PyErr_Occurred())
		return NULL;
	else
		return (PyObject *)(*v->ob_type->tp_as_sequence->sq_repeat)(v, n);
}

/*
Integer overflow checking used to be done using a double, but on 64
bit machines (where both long and double are 64 bit) this fails
because the double doesn't have enough precision.  John Tromp suggests
the following algorithm:

Suppose again we normalize a and b to be nonnegative.
Let ah and al (bh and bl) be the high and low 32 bits of a (b, resp.).
Now we test ah and bh against zero and get essentially 3 possible outcomes.

1) both ah and bh > 0 : then report overflow

2) both ah and bh = 0 : then compute a*b and report overflow if it comes out
						negative

3) ah > 0 and bh = 0  : compute ah*bl and report overflow if it's >= 2^31
						compute al*bl and report overflow if it's negative
						add (ah*bl) << 32 to al*bl and report overflow if
						it's negative

In case of no overflow the result is then negated if necessary.

The majority of cases will be 2), in which case this method is the same as
what I suggested before. If multiplication is expensive enough, then the
other method is faster on case 3), but also more work to program, so I
guess the above is the preferred solution.

*/

static PyObject *int8_mul(PyObject *v, PyObject *w)
{
	LONG_LONG a, b, ah, bh, x, y;
	int s = 1;
	PyObject *v1 = v, *w1 = w;

	if (v->ob_type->tp_as_sequence &&
		v->ob_type->tp_as_sequence->sq_repeat) {
		return PgInt8_repeat(v, w);
	}
	else if (w->ob_type->tp_as_sequence &&
			 w->ob_type->tp_as_sequence->sq_repeat) {
		return PgInt8_repeat(w, v);
	}

	if (!(PgInt8_Check(v) && PgInt8_Check(w)))
	{
		/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
		/* the numbers to the same type.								*/

		if (int8_coerce(&v1, &w1))
		{
			PY_NOT_IMPLEMENTED
		}

		/* Now that the numbers are the same type, we check to see if	*/
		/* they are PgInt8s.  If not, we call the multiply method of	*/
		/* the type that they were coerced into.						*/

		if (!PgInt8_Check(v1))
		{
			PyNumberMethods *nb;

			nb = v1->ob_type->tp_as_number;
			if (nb && nb->nb_subtract)
				return (PyObject *)(*nb->nb_multiply)(v1, w1);
			else
			{
				PY_NOT_IMPLEMENTED
			}
		}
	}

	CONVERT_BINOP(v1, w1, &a, &b);

	ah = (unsigned LONG_LONG)a >> (unsigned int)(LONG_LONG_BIT / 2);
	bh = (unsigned LONG_LONG)b >> (unsigned int)(LONG_LONG_BIT / 2);

	/* Quick test for common case: two small positive ints */

	if (ah == 0 && bh == 0) {
		x = a * b;
		if (x < 0)
			goto bad;
		return (PyObject *)PgInt8_FromLongLong(x);
	}

	/* Arrange that a >= b >= 0 */

	if (a < 0) {
		a = -a;
		if (a < 0) {
			/* Largest negative */
			if (b == 0 || b == 1) {
				x = a * b;
				goto ok;
			}
			else
				goto bad;
		}
		s = -s;
		ah = (unsigned LONG_LONG)a >> (unsigned int)(LONG_LONG_BIT / 2);
	}
	if (b < 0) {
		b = -b;
		if (b < 0) {
			/* Largest negative */
			if (a == 0 || (a == 1 && s == 1)) {
				x = a * b;
				goto ok;
			}
			else
				goto bad;
		}
		s = -s;
		bh = (unsigned LONG_LONG)b >> (unsigned int)(LONG_LONG_BIT / 2);
	}

	/* 1) both ah and bh > 0 : then report overflow */

	if (ah != 0 && bh != 0)
		goto bad;

	/* 2) both ah and bh = 0 : then compute a*b and report
							   overflow if it comes out negative */

	if (ah == 0 && bh == 0) {
		x = a * b;
		if (x < 0)
			goto bad;
		return (PyObject *)PgInt8_FromLongLong(x * s);
	}

	/* Note: At this point one of ah or bh is equal to zero (but not both)
			 From step 1: !(ah != 0 && bh != 0) => (ah == 0 || bh == 0)
			 From step 2: !(ah == 0 && bh == 0) => (ah != 0 || bh != 0)
			 Taken together (ah == 0 || bh == 0) && (ah != 0 || bh != 0),
			 which can only be satisfied by only one of ah or bh being 0. */

	if (a < b) {
		/* Swap */
		x = a;
		a = b;
		b = x;
		ah = bh;
		/* bh is not needed anymore */
	}

	/* 3) ah > 0 and bh = 0: compute ah*bl and report overflow if it's >= 2^31
							 compute al*bl and report overflow if it's negative
							 add (ah*bl)<<32 to al*bl and report overflow if
							 it's negative (NB b == bl in this case, and we
							 make a = al) */

	y = ah * b; /* since bh = 0, b := bl */

	if (y >= ((unsigned LONG_LONG)1 << (unsigned)((LONG_LONG_BIT / 2) - 1)))
		goto bad;

	a = (unsigned LONG_LONG)a &
		(((LONG_LONG)1 << (unsigned)(LONG_LONG_BIT / 2)) - 1);
	x = a * b;
	if (x < 0)
		goto bad;
	x += (unsigned LONG_LONG)y << (unsigned int)(LONG_LONG_BIT / 2);
	if (x < 0)
		goto bad;
ok:
	return (PyObject *)PgInt8_FromLongLong(x * s);

bad:
	return err_ovf("PgInt8 multiplication");
}

static int i_divmod(register LONG_LONG xi, register LONG_LONG yi,
					LONG_LONG *p_xdivy, LONG_LONG *p_xmody)
{
	LONG_LONG xdivy, xmody;
	
	if (yi == 0) {
		PyErr_SetString(PyExc_ZeroDivisionError,
						"PgInt8 division or modulo by zero");
		return -1;
	}
	if (yi < 0) {
		if (xi < 0) {
			if (yi == -1 && -xi < 0) {
				/* most negative / -1 */
				err_ovf("PgInt8 division");
				return -1;
			}
			xdivy = -xi / -yi;
		}
		else
			xdivy = - (xi / -yi);
	}
	else {
		if (xi < 0)
			xdivy = - (-xi / yi);
		else
			xdivy = xi / yi;
	}
	xmody = xi - xdivy * yi;
	if ((xmody < 0 && yi > 0) || (xmody > 0 && yi < 0)) {
		xmody += yi;
		xdivy -= 1;
	}
	*p_xdivy = xdivy;
	*p_xmody = xmody;
	return 0;
}

static PyObject *int8_div(PyObject *x, PyObject *y)
{
	LONG_LONG xi, yi;
	LONG_LONG d, m;
	PyObject *v1 = x, *w1 = y;

	if (!(PgInt8_Check(x) && PgInt8_Check(y)))
	{
		/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
		/* the numbers to the same type.								*/

		if (int8_coerce(&v1, &w1))
		{
			PY_NOT_IMPLEMENTED
		}

		/* Now that the numbers are the same type, we check to see if	*/
		/* they are PgInt8s.  If not, we call the divide method of the	*/
		/* type that they were coerced into.							*/

		if (!PgInt8_Check(v1))
		{
			PyNumberMethods *nb;

			nb = v1->ob_type->tp_as_number;
			if (nb && nb->nb_subtract)
				return (PyObject *)(*nb->nb_divide)(v1, w1);
			else
			{
				PY_NOT_IMPLEMENTED
			}
		}
	}

	CONVERT_BINOP(v1, w1, &xi, &yi);

	if (i_divmod(xi, yi, &d, &m) < 0)
		return NULL;

	return (PyObject *)PgInt8_FromLongLong(d);
}

static PyObject *int8_mod(PyObject *x, PyObject *y)
{
	LONG_LONG xi, yi;
	LONG_LONG d, m;
	PyObject *v1 = x, *w1 = y;

	if (!(PgInt8_Check(x) && PgInt8_Check(y)))
	{
		/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
		/* the numbers to the same type.								*/

		if (int8_coerce(&v1, &w1))
		{
			PY_NOT_IMPLEMENTED
		}

		/* Now that the numbers are the same type, we check to see if	*/
		/* they are PgInt8s.  If not, we call the remainder method of	*/
		/* the type that they were coerced into.						*/

		if (!PgInt8_Check(v1))
		{
			PyNumberMethods *nb;

			nb = v1->ob_type->tp_as_number;
			if (nb && nb->nb_subtract)
				return (PyObject *)(*nb->nb_remainder)(v1, w1);
			else
			{
				PY_NOT_IMPLEMENTED
			}
		}
	}

	CONVERT_BINOP(v1, w1, &xi, &yi);

	if (i_divmod(xi, yi, &d, &m) < 0)
		return NULL;

	return (PyObject *)PgInt8_FromLongLong(m);
}

static PyObject *int8_divmod(PyObject *x, PyObject *y)
{
	LONG_LONG xi, yi;
	LONG_LONG d, m;
	PyObject *v1 = x, *w1 = y;

	if (!(PgInt8_Check(x) && PgInt8_Check(y)))
	{
		/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
		/* the numbers to the same type.								*/

		if (int8_coerce(&v1, &w1))
		{
			PY_NOT_IMPLEMENTED
		}

		/* Now that the numbers are the same type, we check to see if	*/
		/* they are PgInt8s.  If not, we call the divmod method of the	*/
		/* type that they were coerced into.							*/

		if (!PgInt8_Check(v1))
		{
			PyNumberMethods *nb;

			nb = v1->ob_type->tp_as_number;
			if (nb && nb->nb_subtract)
				return (PyObject *)(*nb->nb_divmod)(v1, w1);
			else
			{
				PY_NOT_IMPLEMENTED
			}
		}
	}

	CONVERT_BINOP(v1, w1, &xi, &yi);

	if (i_divmod(xi, yi, &d, &m) < 0)
		return NULL;

	return Py_BuildValue("(oo)", PgInt8_FromLongLong(d),
								 PgInt8_FromLongLong(m));
}

static PyObject *int8_pow(PyObject *v, PyObject *w, PyObject *z)
{
#if 1
	LONG_LONG iv, iw, iz=0, ix, temp, prev;
	PyObject *v1 = v, *w1 =w;

	if (!(PgInt8_Check(v) && PgInt8_Check(w)))
	{
		/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
		/* the numbers to the same type.								*/

		if (int8_coerce(&v1, &w1))
		{
			PY_NOT_IMPLEMENTED
		}

		/* Now that the numbers are the same type, we check to see if	*/
		/* they are PgInt8s.  If not, we call the power method of the	*/
		/* type that they were coerced into.							*/

		if (!PgInt8_Check(v1))
		{
			PyNumberMethods *nb;

			nb = v1->ob_type->tp_as_number;
			if (nb && nb->nb_add)
				return (PyObject *)(*nb->nb_power)(v1, w1, z);
			else
			{
				PY_NOT_IMPLEMENTED
			}
		}
	}

	CONVERT_BINOP(v, w, &iv, &iw);

	if (iw < 0)
	{
		if (iv)
			PyErr_SetString(PyExc_ValueError,
							"cannot raise PgInt8 to a negative power");
		else
			PyErr_SetString(PyExc_ZeroDivisionError,
							"cannot raise 0 to a negative power");
		return (PyObject *)NULL;
	}

	if ((PyObject *)z != Py_None)
	{
		CONVERT_TO_LONGLONG(z, &iz);
		if (iz == 0)
		{
			PyErr_SetString(PyExc_ValueError,
							"pow() arg 3 cannot be 0");
			return (PyObject *)NULL;
		}
	}
	/*
	 * XXX: The original exponentiation code stopped looping
	 * when temp hit zero; this code will continue onwards
	 * unnecessarily, but at least it won't cause any errors.
	 * Hopefully the speed improvement from the fast exponentiation
	 * will compensate for the slight inefficiency.
	 * XXX: Better handling of overflows is desperately needed.
	 */
	temp = iv;
	ix = 1;
	while (iw > 0)
	{
		prev = ix;		/* Save value for overflow check */
		if ((unsigned LONG_LONG)iw & 1U)
		{
			ix = ix*temp;
			if (temp == 0)
				break; /* Avoid ix / 0 */
			if (ix / temp != prev)
				return err_ovf("PgInt8 exponentiation");
		}
		iw = (unsigned LONG_LONG)iw >> 1U; /* Shift exponent down by 1 bit */
		if (iw == 0) break;
		prev = temp;
		temp *= temp;	/* Square the value of temp */
		if (prev != 0 && (temp / prev) != prev)
			return err_ovf("PgInt8 exponentiation");
		if (iz)
		{
			/* If we did a multiplication, perform a modulo */
			ix = ix % iz;
			temp = temp % iz;
		}
	}
	if (iz)
	{
		LONG_LONG div, mod;
		if (i_divmod(ix, iz, &div, &mod) < 0)
			return (PyObject *)NULL;
		ix=mod;
	}
	return (PyObject *)PgInt8_FromLongLong(ix);
#else
	LONG_LONG iv, iw, ix;

	CONVERT_BINOP(v, w, &iv, &iw);

	if (iw < 0) {
		PyErr_SetString(PyExc_ValueError,
						"PgInt8 to the negative power");
		return (PyObject *)NULL;
	}
	if ((PyObject *)z != Py_None) {
		PyErr_SetString(PyExc_TypeError,
						"pow(PgInt8, PgInt8, PgInt8) not yet supported");
		return (PyObject *)NULL;
	}
	ix = 1;
	while (--iw >= 0) {
		LONG_LONG prev = ix;
		ix = ix * iv;
		if (iv == 0)
			break; /* 0 to some power -- avoid ix / 0 */
		if (ix / iv != prev)
			return err_ovf("PgInt8 exponentiation");
	}
	return (PyObject *)PgInt8_FromLongLong(ix);
#endif
}								

static PyObject *int8_neg(PgInt8Object *v)
{
	LONG_LONG a, x;

	a = PgInt8_AS_LONGLONG(v);
	x = -a;

	if (a < 0 && x < 0)
		return err_ovf("PgInt8 negation");

	return (PyObject *)PgInt8_FromLongLong(x);
}

static PyObject *int8_pos(PgInt8Object *v)
{
	Py_INCREF(v);
	return (PyObject *)v;
}

static PyObject *int8_abs(PyObject *v)
{
	LONG_LONG a, x;

	CONVERT_TO_LONGLONG(v, &a);

	if (a >= 0)
		return (PyObject *)PgInt8_FromLongLong(a);
	else
	{
		x = -a;

		if (a < 0 && x < 0)
			return err_ovf("PgInt8 negation");

		return (PyObject *)PgInt8_FromLongLong(x);
	}
}

static int int8_nonzero(PgInt8Object *v)
{
	return (PgInt8_AS_LONGLONG(v) != 0);
}

static PyObject *int8_invert(PgInt8Object *v)
{
	unsigned LONG_LONG a = (unsigned LONG_LONG)PgInt8_AS_LONGLONG(v);

	return (PyObject *)PgInt8_FromLongLong(~a);
}

static PyObject *int8_lshift(PyObject *v, PyObject *w)
{
	LONG_LONG a, b;

	CONVERT_BINOP(v, w, &a, &b);

	if (b < 0) {
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		return (PyObject *)NULL;
	}
	if (a == 0 || b == 0) {
		Py_INCREF(v);
		return v;
	}
	if (b >= LONG_LONG_BIT) {
		return (PyObject *)PgInt8_FromLong(0L);
	}
	a = (unsigned LONG_LONG)a << b;
	return (PyObject *)PgInt8_FromLongLong(a);
}

static PyObject *int8_rshift(PyObject *v, PyObject *w)
{
	LONG_LONG a, b;

	CONVERT_BINOP(v, w, &a, &b);

	if (b < 0) {
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		return (PyObject *)NULL;
	}
	if (a == 0 || b == 0) {
		Py_INCREF(v);
		return (PyObject *)v;
	}
	if (b >= LONG_LONG_BIT) {
		if (a < 0)
			a = (LONG_LONG)-1;
		else
			a = (LONG_LONG)0;
	}
	else {
		a = Py_ARITHMETIC_RIGHT_SHIFT(LONG_LONG, a, b);
	}
	return (PyObject *)PgInt8_FromLongLong(a);
}

static PyObject *int8_and(PyObject *v, PyObject *w)
{
	LONG_LONG a, b;

	CONVERT_BINOP(v, w, &a, &b);

	return (PyObject *)PgInt8_FromLongLong((unsigned LONG_LONG)a &
										   (unsigned LONG_LONG)b);
}

static PyObject *int8_xor(PyObject *v, PyObject *w)
{
	LONG_LONG a, b;

	CONVERT_BINOP(v, w, &a, &b);

	return (PyObject *)PgInt8_FromLongLong((unsigned LONG_LONG)a ^
										   (unsigned LONG_LONG)b);
}

static PyObject *int8_or(PyObject *v, PyObject *w)
{
	LONG_LONG a, b;

	CONVERT_BINOP(v, w, &a, &b);

	return (PyObject *)PgInt8_FromLongLong((unsigned LONG_LONG)a |
										   (unsigned LONG_LONG)b);
}

static PyObject *int8_int(PgInt8Object *v)
{
	long x;

	x = PgInt8_AsLong(v);

	if (PyErr_Occurred())
		return NULL;

	return Py_BuildValue("l", x);
}

static PyObject *int8_long(PgInt8Object *v)
{
	return PyLong_FromLongLong(PgInt8_AS_LONGLONG(v));
}

static PyObject *int8_float(PgInt8Object *v)
{
	return Py_BuildValue("d", (double)(PgInt8_AS_LONGLONG(v)));
}

static PyObject *int8_oct(PgInt8Object *v)
{
	char buf[100];
	LONG_LONG x = PgInt8_AS_LONGLONG(v);
	if (x == 0)
		strcpy(buf, "0");
	else
		sprintf(buf, "0%llo", x);
	return Py_BuildValue("s", buf);
}

static PyObject *int8_hex(PgInt8Object *v)
{
	char buf[100];
	LONG_LONG x = PgInt8_AS_LONGLONG(v);
	sprintf(buf, "0x%llx", x);
	return Py_BuildValue("s", buf);
}

static PyNumberMethods int8_as_number = {
		(binaryfunc)	int8_add,		/*nb_add*/
		(binaryfunc)	int8_sub,		/*nb_subtract*/
		(binaryfunc)	int8_mul,		/*nb_multiply*/
		(binaryfunc)	int8_div,		/*nb_divide*/
		(binaryfunc)	int8_mod,		/*nb_remainder*/
		(binaryfunc)	int8_divmod,	/*nb_divmod*/
		(ternaryfunc)	int8_pow,		/*nb_power*/
		(unaryfunc)		int8_neg,		/*nb_negative*/
		(unaryfunc)		int8_pos,		/*nb_positive*/
		(unaryfunc)		int8_abs,		/*nb_absolute*/
		(inquiry)		int8_nonzero,	/*nb_nonzero*/
		(unaryfunc)		int8_invert,	/*nb_invert*/
		(binaryfunc)	int8_lshift,	/*nb_lshift*/
		(binaryfunc)	int8_rshift,	/*nb_rshift*/
		(binaryfunc)	int8_and,		/*nb_and*/
		(binaryfunc)	int8_xor,		/*nb_xor*/
		(binaryfunc)	int8_or,		/*nb_or*/
		(coercion)		int8_coerce,	/*nb_coerce*/
		(unaryfunc)		int8_int,		/*nb_int*/
		(unaryfunc)		int8_long,		/*nb_long*/
		(unaryfunc)		int8_float,		/*nb_float*/
		(unaryfunc)		int8_oct,		/*nb_oct*/
		(unaryfunc)		int8_hex,		/*nb_hex*/
		0,								/*nb_inplace_add*/
		0,								/*nb_inplace_subtract*/
		0,								/*nb_inplace_multiply*/
		0,								/*nb_inplace_divide*/
		0,								/*nb_inplace_remainder*/
		0,								/*nb_inplace_power*/
		0,								/*nb_inplace_lshift*/
		0,								/*nb_inplace_rshift*/
		0,								/*nb_inplace_and*/
		0,								/*nb_inplace_xor*/
		0,								/*nb_inplace_or*/
};

PyTypeObject PgInt8_Type = {
		PyObject_HEAD_INIT(NULL)
		0,
		"PgInt8",
		sizeof(PgInt8Object),
		0,
		(destructor)int8_dealloc,		/*tp_dealloc*/
		(printfunc)int8_print,			/*tp_print*/
		0,								/*tp_getattr*/
		0,								/*tp_setattr*/
		(cmpfunc)int8_compare,			/*tp_compare*/
		(reprfunc)int8_repr,			/*tp_repr*/
		&int8_as_number,				/*tp_as_number*/
		0,								/*tp_as_sequence*/
		0,								/*tp_as_mapping*/
		(hashfunc)int8_hash,			/*tp_hash*/
		0,								/*tp_call*/
		0,								/*tp_str*/
		0,								/*tp_getattro*/
		0,								/*tp_setattro*/
		0,								/*tp_as_buffer*/
		Py_TPFLAGS_CHECKTYPES			/*tp_flags*/
};

void initpgint8(void)
{
	PgInt8_Type.ob_type = &PyType_Type;
}
#endif
