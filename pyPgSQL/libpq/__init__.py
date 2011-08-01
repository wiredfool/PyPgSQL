"""
    libpq - Access to the PostgreSQL C-API for Python.
    =====================================================================
    Copyright 2000 by Billy G. Allie.
    All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose and without fee is hereby granted, pro-
    vided that the above copyright notice appear in all copies and that
    both that copyright notice and this permission notice appear in sup-
    porting documentation, and that the copyright owner's name not be
    used in advertising or publicity pertaining to distribution of the
    software without specific, written prior permission.

    THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
    INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN
    NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
    CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
    OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
    OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
    USE OR PERFORMANCE OF THIS SOFTWARE.
"""
from libpq import *
from libpq import __version__

HAVE_LONG_LONG_SUPPORT = dir().count('PgInt8') == 1

#-----------------------------------------------------------------------+
# Add support to pickle the following pyPgSQL opbjects:			|
#	PgInt2, PgInt8, PgBoolean					|
#-----------------------------------------------------------------------+

def _B(value):
    return PgBoolean(value)

def _C(conninfo):
    return PQconnectdb(conninfo)

def _I2(value):
    return PgInt2(value)

def _I8(value):
    return PgInt8(value)

def _LO(conninfo, oid, lo_mode, softspace, offset):
    # Rebuild a large object.
    cnx = PQconnectdb(conninfo)
    lo = PgLargeObject(cnx, oid)
    if len(lo_mode) > 0:
	lo.open(lo_mode)
	lo.seek(offset,0)
    lo.softspace = softspace
    return lo

def _V(value):
    return PgVersion(value)

# Module Initialization

class modinit:
    import copy_reg
    def pickle_PgBoolean(value):
	return _B, (str(value),)
    copy_reg.pickle(PgBooleanType, pickle_PgBoolean, _B)

    def pickle_PgConnection(value):
	return _C, (value._conninfo,)
    copy_reg.pickle(PgConnectionType, pickle_PgConnection, _C)

    def pickle_PgInt2(value):
	return _I2, (str(value),)
    copy_reg.pickle(PgInt2Type, pickle_PgInt2, _I2)

    if HAVE_LONG_LONG_SUPPORT:
        def pickle_PgInt8(value):
            return _I8, (str(value),)
        copy_reg.pickle(PgInt8Type, pickle_PgInt8, _I8)

    def pickle_PgLargeObject(value):
	return _LO, value._pickle()
    copy_reg.pickle(PgLargeObjectType, pickle_PgLargeObject, _LO)

    def pickle_PgVersion(value):
	return _V, (str(value),)
    copy_reg.pickle(PgVersionType, pickle_PgVersion, _V)

del modinit
