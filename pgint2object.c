#ident "@(#) $Id: pgint2object.c,v 1.12 2002/10/02 04:08:38 ghaering Exp $"
/* vi:set sw=4 ts=8 showmode ai: */
/**(H+)*****************************************************************\
| Name:		pgint2object.c						|
|									|
| Description:	This file implements the PostgreSQL int2 type for	|
|		Pyhon.  This Python object is part of the libpq module.	|
|									|
| Note:		This type object is based on ideas and concepts in the	|
|		Python integer and Python long type object.		|
|=======================================================================|
| Copyright 2001 by Billy G. Allie.					|
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
| 01OCT2002 gh  HAVE_LONG_LONG => HAVE_LONG_LONG_SUPPORT		|
| 01OCT2001 bga Changed all new style comments to original style.	|
| 26SEP2001 bga Change the constructors so that they return PyObject *	|
|		instead of PgInt2Object *.				|
| 30AUG2001 bga Use PyObject_Del() instead of PyMem_Free() in dealloc()	|
|		to delete the object.					|
| 09AUG2001 bga Change code to use Py_BuildValue() for creating Python	|
|		objects from C values (where possible).			|
|	    --- General code clean up.  Removed some definitions that	|
|		are in the new libpqmodule.h header file.		|
| 03AUG2001 bga Changed 'PyMem_DEL()' to 'PyMem_Free()'.		|
| 14JUN2001 bga Wrapped code for PgInt8 inside #ifdef HAVE_LONG_LONG.	|
| 09JUN2001 bga This file did not build under Python 2.0.  It does now.	|
| 06JUN2001 bga To the extent possible, I picked the "lint" from the	|
|		code.							|
| 04JUN2001 bga Changed PyObject_HEAD_INIT(&PyType_Type) to		|
|		PyObject_HEAD_INIT(NULL) to silence error produced by	|
|		some compilers.						|
| 31MAY2001 bga Initial release by Billy G. Allie.			|
\*(H-)******************************************************************/

#include <ctype.h>
#include "Python.h"
#include "libpqmodule.h"

static PyObject *err_ovf(char *msg)
{
    PyErr_SetString(PyExc_OverflowError, msg);
    return NULL;
}

static void int2_dealloc(PgInt2Object *v)
{
    PyObject_Del(v);
}

short PgInt2_GetMax(void)
{
    return INT2_MAX;
}

short PgInt2_AsInt2(register PgInt2Object *op)
{
    if (op && PgInt2_Check(op))
	return PgInt2_AS_INT2((PgInt2Object *)op);
	
    PyErr_SetString(PyExc_TypeError, "a PgInt2 is required");
    return (short)-1;
}

short PgInt2_AsLong(register PgInt2Object *op)
{
    if (op && PgInt2_Check(op))
    {
        return (long)PgInt2_AS_INT2(op);
    }
	
    PyErr_SetString(PyExc_TypeError, "a PgInt2 is required");
    return -1L;
}

PyObject *PgInt2_FromInt2(short ival)
{
    register PyObject *v;

    if ((v = (PyObject *)PyObject_New(PgInt2Object, &PgInt2_Type)) ==
	(PyObject *)NULL)
    {
	return v;
    }
    ((PgInt2Object *)v)->ob_ival = ival;

    return v;
}

PyObject *PgInt2_FromLong(long ival)
{
    register PyObject *v;
    short x = (short)ival;

    if (x != ival)
    {
	PyErr_SetString(PyExc_ValueError,
			"integer to large to convert to PgInt2");
	return (PyObject *)NULL;
    }

    if ((v = (PyObject *)PyObject_NEW(PgInt2Object, &PgInt2_Type)) ==
	(PyObject *)NULL)
    {
	return v;
    }
    ((PgInt2Object *)v)->ob_ival = (short)ival;

    return v;
}

PyObject *PgInt2_FromString(char *s, char **pend, int base)
{
    char *end;
    long x;
    short xs;
    char buffer[256]; /* For errors */

    if ((base != 0 && base < 2) || base > 36)
    {
	PyErr_SetString(PyExc_ValueError,
			"PgInt2() base must be >= 2 and <= 36");
	return (PyObject *)NULL;
    }

    while (*s && isspace(Py_CHARMASK((unsigned int)(*s))))
	s++;

    errno = 0;

    if (base == 0 && s[0] == '0')
	x = strtoul(s, &end, base);
    else
	x = strtol(s, &end, base);

    xs = (short)x;     /* Used later for the overflow check. */

    if (end == s || !isalnum(Py_CHARMASK((unsigned int)(end[-1]))))
	goto bad;

    while (*end && isspace(Py_CHARMASK((unsigned int)(*end))))
	end++;
    
    if (*end != '\0')
    {
bad:
	sprintf(buffer, "invalid literal for PgInt2(): %.200s", s);
	PyErr_SetString(PyExc_ValueError, buffer);
	return (PyObject *)NULL;
    }
    else if ((errno != 0) || (xs != x))
    {
	sprintf(buffer, "PgInt2() literal too large: %.200s", s);
	PyErr_SetString(PyExc_ValueError, buffer);
	return (PyObject *)NULL;
    }

    if (pend)
	*pend = end;

    return PgInt2_FromInt2(xs);
}

PyObject *PgInt2_FromUnicode(Py_UNICODE *s, int length, int base)
{
    char buffer[256];
    
    if (length >= sizeof(buffer))
    {
	PyErr_SetString(PyExc_ValueError,
			"int() literal too large to convert");
	return (PyObject *)NULL;
    }
    
    if (PyUnicode_EncodeDecimal(s, length, buffer, NULL))
	return (PyObject *)NULL;

    return PgInt2_FromString(buffer, NULL, base);
}

static int convert_binop(PyObject *v, PyObject *w, long *a, long *b)
{
    short x, y;

    if (PgInt2_Check(v))
	*a = (long)PgInt2_AS_INT2(v);
    else if (PyLong_Check(v))
    { 
	*a = PyLong_AsLong(v);

	if (*a == -1 && PyErr_Occurred())
	    return 0;
    }
    else if (PyInt_Check(v))
	*a = (long)(PyInt_AS_LONG(v));
    else
    {
	return 0;
    }

    if (w == Py_None)
	return 1;
    else if (PgInt2_Check(w))
	*b = (long)PgInt2_AS_INT2(w);
    else if (PyLong_Check(w))
    {
	*b = PyLong_AsLong(w);

	if (*b == -1 && PyErr_Occurred())
	    return 0;
    }
    else if (PyInt_Check(w))
	*b = (long)(PyInt_AS_LONG(w));

    x = *a;
    y = *b;
    if ((x != *a) || (y != *b))
    {
	PyErr_SetString(PyExc_ValueError,
			"numeric literal too large to convert to PgInt2");
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
			    "operation not implemented for PgInt2 type"); \
	    return NULL; \
	}

#define PY_NOT_IMPLEMENTED \
	PyErr_SetString(PyExc_NotImplementedError, \
			"operation not implemented for PgInt2 type"); \
	return NULL; \

#define Py_TPFLAGS_CHECKTYPES 0
#endif

#define CONVERT_TO_INT2(v, a) \
		CONVERT_BINOP(v, Py_None, a, (long *)NULL)

/* Methods */

/* ARGSUSED */
static int int2_print(PgInt2Object *v, FILE *fp, int flags)
     /* flags -- not used but required by interface */
{
    fprintf(fp, "%d", (int)PgInt2_AS_INT2(v));
    return 0;
}

static PyObject *int2_repr(PgInt2Object *v)
{
    char buf[32];
    sprintf(buf, "%d", (int)PgInt2_AS_INT2(v));
    return Py_BuildValue("s", buf);
}

static int int2_compare(PgInt2Object *v, PgInt2Object *w)
{
    register short i = PgInt2_AS_INT2(v);
    register short j = PgInt2_AS_INT2(w);
    
    return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static long int2_hash(PgInt2Object *v)
{
    long x = (long)PgInt2_AS_INT2(v);

    if (x == -1)
	x = -2;

    return x;
}

static int int2_coerce(PyObject **pv, PyObject **pw)
{
    if (PgInt2_Check(*pv))
    {
	if (PyInt_Check(*pw))
	{
	    *pv = Py_BuildValue("h", PgInt2_AS_INT2(*pv));
	    Py_INCREF(*pw);
	}
#if defined(HAVE_LONG_LONG_SUPPORT)
	else if (PgInt8_Check(*pw))
	{
	    *pv = (PyObject *)PgInt8_FromLong((long)PgInt2_AS_INT2(*pv));
	    Py_INCREF(*pw);
	}
#endif
	else if (PyLong_Check(*pw))
	{
	    *pv = (PyObject *)PyLong_FromLong((long)PgInt2_AS_INT2(*pv));
	    Py_INCREF(*pw);
	}
	else if (PyFloat_Check(*pw))
	{
	  *pv = Py_BuildValue("d", (double)PgInt2_AS_INT2(*pv));
	    Py_INCREF(*pw);
	}
	else if (PyComplex_Check(*pw))
	{
	    *pv = (PyObject *)PyComplex_FromDoubles(
				    (double)PgInt2_AS_INT2(*pv), (double)0);
	    Py_INCREF(*pw);
	}
	else
	    return 1;

	return 0;
    }
    else if (PgInt2_Check(*pw)) {
	if (PyInt_Check(*pv))
	{
	    *pw = Py_BuildValue("h", PgInt2_AS_INT2(*pw));
	    Py_INCREF(*pv);
	}
#if defined(HAVE_LONG_LONG_SUPPORT)
	else if (PgInt8_Check(*pv))
	{
	    *pw = (PyObject *)PgInt8_FromLong((long)PgInt2_AS_INT2(*pw));
	    Py_INCREF(*pv);
	}
#endif
	else if (PyLong_Check(*pv))
	{
	    *pw = (PyObject *)PyLong_FromLong((long)PgInt2_AS_INT2(*pw));
	    Py_INCREF(*pw);
	}
	else if (PyFloat_Check(*pv))
	{
	  *pw = Py_BuildValue("d", (double)PgInt2_AS_INT2(*pw));
	    Py_INCREF(*pw);
	}
	else if (PyComplex_Check(*pv))
	{
	    *pw = (PyObject *)PyComplex_FromDoubles(
				    (double)PgInt2_AS_INT2(*pw), (double)0);
	    Py_INCREF(*pv);
	}
	else
	    return 1;

	return 0;
    }

    return 1; /* Can't do it */
}

static PyObject *int2_add(PyObject *v, PyObject *w)
{
    long a, b, x;
    short xs;
    PyObject *v1 = v, *w1 =w;

    if (!(PgInt2_Check(v) && PgInt2_Check(w)))
    {
	/* Ok, we are adding mixed mode numbers, first we coerce the	*/
	/* numbers to the same type.					*/

	if (int2_coerce(&v1, &w1))
	{
	    PY_NOT_IMPLEMENTED
	}

	/* Now that the numbers are the same type, we check to see if	*/
	/* they are PgInt2s.  If not, we call the add method of the	*/
	/* type that they were coerced into.				*/

	if (!PgInt2_Check(v1))
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

    /* We are adding two PgInt2s. */

    CONVERT_BINOP(v1, w1, &a, &b);

    xs = (short)(x = a + b);

    if (x != (long)xs)
        return err_ovf("PgInt2 addition");

    return (PyObject *)PgInt2_FromInt2(xs);
}

static PyObject *int2_sub(PyObject *v, PyObject *w)
{
    long a, b, x;
    short xs;
    PyObject *v1 = v, *w1 = w;

    if (!(PgInt2_Check(v) && PgInt2_Check(w)))
    {
	/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
	/* the numbers to the same type.				*/

	if (int2_coerce(&v1, &w1))
	{
	    PY_NOT_IMPLEMENTED
	}

	/* Now that the numbers are the same type, we check to see if	*/
	/* they are PgInt2s.  If not, we call the subtract method of	*/
	/* the type that they were coerced into.			*/

	if (!PgInt2_Check(v1))
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

    /* We are subtracting two PgInt2s. */
    
    CONVERT_BINOP(v1, w1, &a, &b);
    
    xs = (short)(x = a - b);

    if (x != (long)xs)
	return err_ovf("PgInt2 subtraction");

    return (PyObject *)PgInt2_FromLong(x);
}

static PyObject *PgInt2_repeat(PyObject *v, PyObject *w)
{
    /* sequence * long */
    long n = PgInt2_AsLong((PgInt2Object *)w);

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
                        add (ah*bl)<<32 to al*bl and report overflow if
                        it's negative

In case of no overflow the result is then negated if necessary.

The majority of cases will be 2), in which case this method is the same as
what I suggested before. If multiplication is expensive enough, then the
other method is faster on case 3), but also more work to program, so I
guess the above is the preferred solution.

*/

static PyObject *int2_mul(PyObject *v, PyObject *w)
{
    long a, b, x;
    short xs;
    PyObject *v1 = v, *w1 = w;

    if (v->ob_type->tp_as_sequence &&
        v->ob_type->tp_as_sequence->sq_repeat) {
        return PgInt2_repeat(v, w);
    }
    else if (w->ob_type->tp_as_sequence &&
             w->ob_type->tp_as_sequence->sq_repeat) {
        return PgInt2_repeat(w, v);
    }

    if (!(PgInt2_Check(v) && PgInt2_Check(w)))
    {
	/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
	/* the numbers to the same type.				*/

	if (int2_coerce(&v1, &w1))
	{
	    PY_NOT_IMPLEMENTED
	}

	/* Now that the numbers are the same type, we check to see if	*/
	/* they are PgInt2s.  If not, we call the multiply method of	*/
	/* the type that they were coerced into.			*/

	if (!PgInt2_Check(v1))
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

    xs = (short)(x = a * b);

    if (x != (long)xs)
        return err_ovf("PgInt2 multiplication");
    else
        return (PyObject *)PgInt2_FromInt2(x);
}

static int i_divmod(register long xi, register long yi,
		    long *p_xdivy, long *p_xmody)
{
    long xdivy, xmody;
    
    if (yi == 0) {
	PyErr_SetString(PyExc_ZeroDivisionError,
			"PgInt2 division or modulo by zero");
	return -1;
    }
    if (yi < 0) {
	if (xi < 0) {
	    if (yi == -1 && -xi < 0) {
		/* most negative / -1 */
		err_ovf("PgInt2 division");
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

static PyObject *int2_div(PyObject *x, PyObject *y)
{
    long xi, yi;
    long d, m;
    PyObject *v1 = x, *w1 = y;

    if (!(PgInt2_Check(x) && PgInt2_Check(y)))
    {
	/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
	/* the numbers to the same type.				*/

	if (int2_coerce(&v1, &w1))
	{
	    PY_NOT_IMPLEMENTED
	}

	/* Now that the numbers are the same type, we check to see if	*/
	/* they are PgInt2s.  If not, we call the divide method of the	*/
	/* type that they were coerced into.				*/

	if (!PgInt2_Check(v1))
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

    return (PyObject *)PgInt2_FromLong(d);
}

static PyObject *int2_mod(PyObject *x, PyObject *y)
{
    long xi, yi;
    long d, m;
    PyObject *v1 = x, *w1 = y;

    if (!(PgInt2_Check(x) && PgInt2_Check(y)))
    {
	/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
	/* the numbers to the same type.				*/

	if (int2_coerce(&v1, &w1))
	{
	    PY_NOT_IMPLEMENTED
	}

	/* Now that the numbers are the same type, we check to see if	*/
	/* they are PgInt2s.  If not, we call the remainder method of	*/
	/* the type that they were coerced into.			*/

	if (!PgInt2_Check(v1))
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

    return (PyObject *)PgInt2_FromLong(m);
}

static PyObject *int2_divmod(PyObject *x, PyObject *y)
{
    long xi, yi;
    long d, m;
    PyObject *v1 = x, *w1 = y;

    if (!(PgInt2_Check(x) && PgInt2_Check(y)))
    {
	/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
	/* the numbers to the same type.				*/

	if (int2_coerce(&v1, &w1))
	{
	    PY_NOT_IMPLEMENTED
	}

	/* Now that the numbers are the same type, we check to see if	*/
	/* they are PgInt2s.  If not, we call the divmod method of the	*/
	/* type that they were coerced into.				*/

	if (!PgInt2_Check(v1))
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

    return Py_BuildValue("(oo)", PgInt2_FromLong(d),
                         	 PgInt2_FromLong(m));
}

static PyObject *int2_pow(PyObject *v, PyObject *w, PyObject *z)
{
    long iv, iw, iz=0, ix, temp, prev;
    short ixs;
    PyObject *v1 = v, *w1 = w;

    if (!(PgInt2_Check(v) && PgInt2_Check(w)))
    {
	/* Ok, we are subtracting mixed mode numbers, first we coerce	*/
	/* the numbers to the same type.				*/

	if (int2_coerce(&v1, &w1))
	{
	    PY_NOT_IMPLEMENTED
	}

	/* Now that the numbers are the same type, we check to see if	*/
	/* they are PgInt2s.  If not, we call the power method of the	*/
	/* type that they were coerced into.				*/

	if (!PgInt2_Check(v1))
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

    if (iw < 0) {
	if (iv)
	    PyErr_SetString(PyExc_ValueError,
			    "cannot raise PgInt2 to a negative power");
	else
	    PyErr_SetString(PyExc_ZeroDivisionError,
			    "cannot raise 0 to a negative power");
	return (PyObject *)NULL;
    }

    if (z != Py_None) {
	CONVERT_TO_INT2(z, &iz);
	if (iz == 0) {
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
    while (iw > 0) {
	prev = ix;	/* Save value for overflow check */
	if ((unsigned long)iw & 1U) {	
	    ix = ix*temp;
	    if (temp == 0)
		break; /* Avoid ix / 0 */
	    if (ix / temp != prev)
		return err_ovf("PgInt2 exponentiation");
	}
	iw = (unsigned long)iw >> 1U;	/* Shift exponent down by 1 bit */
	if (iw==0) break;
	prev = temp;
	temp *= temp;	/* Square the value of temp */
	if (prev!=0 && (temp / prev) != prev)
	    return err_ovf("PgInt2 exponentiation");
	if (iz) {
	    /* If we did a multiplication, perform a modulo */
	    ix = ix % iz;
	    temp = temp % iz;
	}
    }
    if (iz) {
	long div, mod;
	if (i_divmod(ix, iz, &div, &mod) < 0)
	    return (PyObject *)NULL;
	ix=mod;
    }

    ixs = (short)ix;
    if (ix != (long)ixs)
        return err_ovf("PgInt2 exponentiation");

    return (PyObject *)PgInt2_FromLong(ix);
}

static PyObject *int2_neg(PgInt2Object *v)
{
    long a, x;

    a = (long)PgInt2_AS_INT2(v);
    x = -a;

    if (a < 0 && x < 0)
	return err_ovf("PgInt2 negation");

    return (PyObject *)PgInt2_FromLong(x);
}

static PyObject *int2_pos(PgInt2Object *v)
{
    Py_INCREF(v);
    return (PyObject *)v;
}

static PyObject *int2_abs(PyObject *v)
{
    long a, x;

    CONVERT_TO_INT2(v, &a);

    if (a >= 0)
	return (PyObject *)PgInt2_FromLong(a);
    else
    {
        x = -a;

        if (a < 0 && x < 0)
            return err_ovf("PgInt2 negation");

	return (PyObject *)PgInt2_FromLong(x);
    }
}

static int int2_nonzero(PgInt2Object *v)
{
    return ((long)PgInt2_AS_INT2(v) != 0);
}

static PyObject *int2_invert(PgInt2Object *v)
{
    return (PyObject *)PgInt2_FromLong(~(unsigned long)PgInt2_AS_INT2(v));
}

static PyObject *int2_lshift(PyObject *v, PyObject *w)
{
    long a, b;

    CONVERT_BINOP(v, w, &a, &b);

    if (b < 0) {
	PyErr_SetString(PyExc_ValueError, "negative shift count");
	return (PyObject *)NULL;
    }

    if (a == 0 || b == 0) {
	Py_INCREF(v);
	return v;
    }

    if (b >= INT2_BIT)
	return (PyObject *)PgInt2_FromLong(0L);

    a = a << b;

    return (PyObject *)PgInt2_FromLong((unsigned long)a & 0xFFFF);
}

static PyObject *int2_rshift(PyObject *v, PyObject *w)
{
    long a, b;

    CONVERT_BINOP(v, w, &a, &b);

    if (b < 0) {
	PyErr_SetString(PyExc_ValueError, "negative shift count");
	return (PyObject *)NULL;
    }
    if (a == 0 || b == 0) {
	Py_INCREF(v);
	return (PyObject *)v;
    }
    if (b >= INT2_BIT) {
	if (a < 0)
	    a = -1;
	else
	    a = 0;
    }
    else {
	a = Py_ARITHMETIC_RIGHT_SHIFT(long, a, b);
    }
    return (PyObject *)PgInt2_FromLong(a);
}

static PyObject *int2_and(PyObject *v, PyObject *w)
{
    long a, b;

    CONVERT_BINOP(v, w, &a, &b);

    return (PyObject *)PgInt2_FromLong((unsigned long)a & (unsigned long)b);
}

static PyObject *int2_xor(PyObject *v, PyObject *w)
{
    long a, b;

    CONVERT_BINOP(v, w, &a, &b);

    return (PyObject *)PgInt2_FromLong((unsigned long)a ^ (unsigned long)b);
}

static PyObject *int2_or(PyObject *v, PyObject *w)
{
    long a, b;

    CONVERT_BINOP(v, w, &a, &b);

    return (PyObject *)PgInt2_FromLong((unsigned long)a | (unsigned long)b);
}

static PyObject *int2_int(PgInt2Object *v)
{
    return Py_BuildValue("h", PgInt2_AS_INT2(v));
}

static PyObject *int2_long(PgInt2Object *v)
{
    return PyLong_FromLong((long)PgInt2_AS_INT2(v));
}

static PyObject *int2_float(PgInt2Object *v)
{
    return Py_BuildValue("d", (double)(PgInt2_AS_INT2(v)));
}

static PyObject *int2_oct(PgInt2Object *v)
{
    char buf[100];
    int x = (int)PgInt2_AS_INT2(v);
    if (x == 0)
	strcpy(buf, "0");
    else
	sprintf(buf, "0%o", (unsigned)x);
    return Py_BuildValue("s", buf);
}

static PyObject *int2_hex(PgInt2Object *v)
{
    char buf[100];
    int x = (int)PgInt2_AS_INT2(v);
    sprintf(buf, "0x%x", (unsigned)x);
    return Py_BuildValue("s", buf);
}

static PyNumberMethods int2_as_number = {
	(binaryfunc)    int2_add,	/*nb_add*/
	(binaryfunc)    int2_sub,	/*nb_subtract*/
	(binaryfunc)    int2_mul,	/*nb_multiply*/
	(binaryfunc)    int2_div,	/*nb_divide*/
	(binaryfunc)    int2_mod,	/*nb_remainder*/
	(binaryfunc)    int2_divmod,	/*nb_divmod*/
	(ternaryfunc)   int2_pow,	/*nb_power*/
	(unaryfunc)     int2_neg,	/*nb_negative*/
	(unaryfunc)     int2_pos,	/*nb_positive*/
	(unaryfunc)     int2_abs,	/*nb_absolute*/
	(inquiry)       int2_nonzero,	/*nb_nonzero*/
	(unaryfunc)     int2_invert,	/*nb_invert*/
	(binaryfunc)    int2_lshift,	/*nb_lshift*/
	(binaryfunc)    int2_rshift,	/*nb_rshift*/
	(binaryfunc)    int2_and,	/*nb_and*/
	(binaryfunc)    int2_xor,	/*nb_xor*/
	(binaryfunc)    int2_or,	/*nb_or*/
        (coercion)      int2_coerce,	/*nb_coerce*/
	(unaryfunc)     int2_int,	/*nb_int*/
	(unaryfunc)     int2_long,	/*nb_long*/
	(unaryfunc)     int2_float,	/*nb_float*/
	(unaryfunc)     int2_oct,	/*nb_oct*/
	(unaryfunc)     int2_hex, 	/*nb_hex*/
	0,				/*nb_inplace_add*/
	0,				/*nb_inplace_subtract*/
	0,				/*nb_inplace_multiply*/
	0,				/*nb_inplace_divide*/
	0,				/*nb_inplace_remainder*/
	0,				/*nb_inplace_power*/
	0,				/*nb_inplace_lshift*/
	0,				/*nb_inplace_rshift*/
	0,				/*nb_inplace_and*/
	0,				/*nb_inplace_xor*/
	0,				/*nb_inplace_or*/
};

PyTypeObject PgInt2_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"PgInt2",
	sizeof(PgInt2Object),
	0,
	(destructor)int2_dealloc, 	/*tp_dealloc*/
	(printfunc)int2_print,		/*tp_print*/
	0,				/*tp_getattr*/
	0,				/*tp_setattr*/
	(cmpfunc)int2_compare,		/*tp_compare*/
	(reprfunc)int2_repr,		/*tp_repr*/
	&int2_as_number,		/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	(hashfunc)int2_hash,		/*tp_hash*/
        0,				/*tp_call*/
        0,				/*tp_str*/
	0,				/*tp_getattro*/
	0,				/*tp_setattro*/
	0,				/*tp_as_buffer*/
	Py_TPFLAGS_CHECKTYPES		/*tp_flags*/
};

void initpgint2(void)
{
    PgInt2_Type.ob_type = &PyType_Type;
}
