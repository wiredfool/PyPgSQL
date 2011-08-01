#ident "@(#) $Il: PgSQL.py,v 1.47 2005/03/06 20:00:08 ballie01 Exp $"
# vi:set sw=4 ts=8 showmode ai:
#--(H+)-----------------------------------------------------------------+
# Name:		PgSQL.py						|
#									|
# Description:	This file implements a Python DB-API 2.0 interface to	|
#		PostgreSQL.						|
#=======================================================================|
# Copyright 2000 by Billy G. Allie.					|
# All rights reserved.							|
#									|
# Permission to use, copy, modify, and distribute this software and its	|
# documentation for any purpose and without fee is hereby granted, pro-	|
# vided that the above copyright notice appear in all copies and that	|
# both that copyright notice and this permission notice appear in sup-	|
# porting documentation, and that the copyright owner's name not be	|
# used in advertising or publicity pertaining to distribution of the	|
# software without specific, written prior permission.			|
#									|
# THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,	|
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS.  IN	|
# NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR	|
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS	|
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE	|
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE	|
# USE OR PERFORMANCE OF THIS SOFTWARE.					|
#=======================================================================|
# Revision History:							|
#									|
# Date      Ini Description						|
# --------- --- ------------------------------------------------------- |
# 01JUN2006 gh  - Got rid of the custom PgInt2 and PgInt8 types. Normal |
#                 Python int and long are used now, so coercion works   |
#                 again with Python 2.1 through 2.4. This will also get |
#                 rid of the maintenance nightmare that came with C's   |
#                 "long long" type on various platforms.                |
# 20MAY2005 bga - Fixed code so that a DateTimeDelta is returned for a	|
#		  PostgreSQL time datatype.				|
# 15MAY2005 bga - A change to the datetime parsing changed the default	|
#		  datetime object from localtime to UTC.  This update	|
#		  returns the default to localtime and adds a new vari-	|
#		  able, 'useUTCtimeValue' to control the default value	|
#		  of a datetime object.  Setting it to 1 will cause the	|
#		  datetime object to reference the UTC time, 0 will	|
#		  cause the datetime object to reference the client's	|
#		  local time zone.					|
# 27FEB2005 bga - Bug fix release to catch up with the outstanding bug	|
#		  reports.						|
# 27FEB2005 gh  - Converted all checks for Unicode types to isinstance	|
#		  checks.  Fixes bugs #1055180, #1051520, #1016026,	|
#		  #1016010.						|
# 31JUL2004 bga - Fixed mis-spelling of __comm to __conn.		|
#		  [Bug #1001242]					|
# 09MAY2004 bga - Added a 'debug' attribute to the Connection object.	|
# 18JAN2004 bga - Fixed problem with a SELECT ... INTO query.  PgSQL	|
#		  will no longer use a portal for these queires.	|
#		  [Bug #879531]						|
#	    --- - Applied patch by Ben Burton to fix the "FutureWarning |
#		  (negative integers and signed strings)" problem.	|
#		  [Bug #863925]						|
# 16DEC2003 bga - Fixed problem with PgNumeric._quote() method where a	|
#		  PgNumeric with a value of zero was quoted to NULL in-	|
#		  stead of 0.						|
#	    --- - Fixed problem in TypeCache.typecast() when creating	|
#		  large objects from an OID.  The problem also occurred	|
#		  in TypeCache.handleArray().  [Bug #855987].		|
# 22NOV2003 bga - Fix problem with PgNumeric.__rmul__() method.		|
#		  [Bug #841530]						|
# 22OCT2003 bga - Completed the cleanup of parameter handling in the	|
#		  cursor.execute() method.				|
#		- Cleaned up the parameter handling in the		|
#		  cursor.callproc() method.				|
# 20OCT2003 bga - Fixed the quoting of lists/tuples passed in to the	|
#		  cursor.execute method for use with the SQL 'IN' oper-	|
#		  tor.  ALso cleaned up the handling of parameters in	|
#		  cursor.execute().					|
# 05JUL2003 bga - Fixed a problem with PgNumeric where an exception can	|
#		  be thrown if the stated scale and precision of the	|
#		  returned in the first result row does not match later	|
#		  rows.							|
# 26JUN2003 bga - Applied patch from Laurent Pinchart to allow _quote	|
#		  to correctly process objects that are sub-classed	|
#		  from String and Long types.				|
# 05JUN2003 bga - Change the name of the quoting function back to	|
#		  _quote. Variables named like __*__ should be restrict	|
#		  to system names.					|
# 02JUN2003 bga - PgTypes is now hashable.  repr() of a PgType will now	|
#		  return the repr() of the underlying OID.		|
#		- Connection.binary() will now fail if autocommit is	|
#		  enabled.						|
#		- Connection.binary() will no longer commit the trans-	|
#		  action after creating the large object.  The applica-	|
#		  tion developer is now responsible for commiting (or	|
#		  for rolling back) the transaction [Bug #747525].	|
#		- Added PG_TIMETZ to the mix [Patch #708013].		|
#		- Pg_Money will now accept a string as a parameter.	|
#		- PostgreSQL int2, int, int4 will now be cast into	|
#		  Python ints.  Int8 will be cast into a Python long.	|
#		  Float4, float8, and money types will be cast into	|
#		  a Python float.					|
# 07MAR2003 bga - Correct problem with the PgNumeric.__radd__ method.	|
#		  [Bug #694358]						|
#		- Correct problem with conversion of negitive integers	|
#		  (with a given scale and precision) to PgNumerics.	|
#		  [Bug #694358]						|
#		- Work around a problem where the precision and scale	|
#		  of a query result can be different from the first re-	|
#		  sult in the result set.				|
#		  [Bug #697221]						|
# 12JAN2003 bga	- Change the code so that the display length in the	|
#		  cursor.description attribute is always None instead	|
#		  of '-1'.						|
#		- Fixed another problem with interval <-> DateTimeDelta	|
#		  casting.						|
# 23DEC2002 bga - Corrected a problem that caused the close of a portal	|
#		  (ie. PostgreSQL cursor) to fail.			|
# 13DEC2002 bga - Corrected a problem with interval <-> DateTimeDelta	|
#		  casting. [Bug #653044]				|
# 06DEC2002 bga - Corrected problem found by Adam Buraczewski in the	|
#		  __setupTransaction function.				|
#		- Allow both 'e' and 'E' to signify an exponet in the	|
#		  PgNumeric constructor.				|
# 04DEC2002 bga - Correct some problems that were missed in yesterday's	|
#		  fixes (Thanks, Adam, for the help with the problems)	|
# 03DEC2002 bga - Fixed various problems with the constructor and the	|
#		  formatting routine.  These correct the problems re-	|
#		  ported by Adam Buraczewski.				|
# 01DEC2002 bga - Fixed problems with new __setupTransaction function:	|
#		  1. Made it a method of Connection, not Cursor.	|
#		  2. inTransaction was only set if TransactionLevel was	|
#		     set.						|
#		- Fixed instances where method name was incorrect:	|
#		    Connection__gcCursor    -> _Connection__gcCursor	|
#		    Connection__closeCursor -> _Connection__closeCursor	|
#		- Cleaned up code where there was unneeded references	|
#		  to conn in Connection class.				|
# 01DEC2002 gh  - Handle the new '__quote__' method for arrays, too.	|
#		- Still handle '_quote' methods for backwards compati-	|
#		  bility.  This will will avoid complaints by users	|
#		  who have code depending on this.  Like me.		|
# 28NOV2002 bga - Fixed changed PG_TIMESTAMP oid, added PG_TIMESTAMPTZ	|
#		  and PG_REFCURSOR oids.  [Bug #845360]			|
#		- Reference cursors are now type-casted into cursor ob-	|
#		  jects.						|
# 27NOV2002 bga - Completed the emulation of a String object for the	|
#		  PgBytea and PgOther classes.  This corrects several	|
#		  problems with PgBytea concerning comparisons, using	|
#		  PgBytea types as keys in dictionaries, etc.		|
# 10NOV2002 bga - Added the __hash__ function to the PgNumeric class.	|
#		  Cleaned up the code in PgNumeric class and made some	|
#		  small improvments to it.				|
# 02NOV2002 bga - Added the PgArray class.  This is a wrapper around a	|
#		  Python list and is used for all PostgreSQL arrays.	|
#		  This change was made so that lists and tuples no	|
#		  longer have a special meaning in the Cursor.execute()	|
#		  method.						|
#		- Changed the quoting methods defined in the various	|
#		  classes defining PostgreSQL support types to __quote__|
# 27OCT2002 gh  - Merged the Unicode patch. Closes #484468.             |
#		- Convert ROWID to PgInt8 instead of PgInt4 (the origi- |
#		  nal behaviour led to overflow errors.)		|
#		- Always set the displaysize field of			|
#		  cursor.description to -1. PostgreSQL 7.3 doesn't	|
#		  provide this information any more and it's pretty	|
#		  useless nowadays that we've mostly left line printers |
#		  beyond us.						|
# 26OCT2002 bga - Column access by name (attribute and dictionary) now	|
#		  supports mixed-case column name.  Be aware that if	|
#		  you define mixed-case column names in the database,	|
#		  you have to use the mixed-case name to access the co-	|
#		  lumn or it will not be found.  For column names that	|
#		  were not defined with mixed-case in the database, the	|
#		  column access by name is case insensitive.		|
# 02OCT2002 gh  - Only support mxDateTime 2.x and give useful error	|
#		  message if import fails.				|
#		- Cosmetic changes: use cmp builtin where appropriate.	|
#		- Fixed typo where PgTypes.__str__ was spelled		|
#		  incorrectly. Compare to None using "is" operator.	|
#		- Take into account that libpq.PgInt8Type might not be	|
#		  available.						|
#		- Add support for the INTERVAL type.			|
# 08SEP2002 gh  Fixed various problems with the PgResultSet:		|
#		- Column (attribute and dictionary) access is now case- |
#		  insensitive.						|
#		- Added __contains__ method.				|
#		- Added default value parameter to get method.		|
#		- Made setattr actually work.				|
# 11AUG2002 bga Fixed various problems with the PgNumeric type:		|
#		- Added code to allow a float as an argument to the	|
#		  PgNumeric constructor.				|
#		- You can now change the precision/scale of a PgNumeric	|
#		  by: a = PgNumeric(pgnumeric, new prec, new scale).	|
#		  This can be used to 'cast' a PgNumeric to the proper	|
#		  precision and scale before storing it in a field.	|
#		- The arithmatic routines (__add__, __radd__, etc) now	|
#		  ensure that the arguments are properly coerced to the	|
#		  correct types.					|
#		- Added support for the augmented arithmatic operations	|
#		  (__iadd__, etc).					|
#		- The math routines would lose precision becuase the	|
#		  precision/scale were set to be the same as the first	|
#		  operand.  This is no longer the case all precision is	|
#		  retained for the +, -, and * operations.		|
# 03AUG2002 gh  - Fixed problem that occurs when a query on an OID	|
#		  field doesn't return any rows. [Bug #589370].		|
# 29JUL2002 gh  - Applied patch #569203 and also added __pos__ and	|
#		  __abs__ special methods to PgNumeric.			|
# 15MAY2002 gh  - Got rid of redundant building and storing of the	|
#		  mapping of column names to column positions in the	|
#		  PgResultSet class. Now, rows of the same query are	|
#		  instances of a dynamically created class, which has	|
#		  this mapping as a class attribute instead of an at-	|
#		  tribute of the instance. This saves a lot of space,	|
#		  and also slightly increases performance of cursor	|
#		  fetches.						|
# 21APR2002 gh  - Improved the array parsing, so that it now passes all	|
#		  the new mean testcases. Added support for parsing	|
#		  multidimensional arrays. Eventually, the array par-	|
#		  sing code should go as a support function into libpq.	|
#	    	- Replaced all typechecks with "is" operators instead	|
#		  of equals. Mark McEahern had a problem with using	|
#		  pyPgSQL in combination with a FixedPoint class where	|
#		  the reason was that the FixedPoint class was not com-	|
#		  parable to None. The consensus on python-list was	|
#		  that None and all types are singletons, so they	|
#		  should be checked using "is", which is also faster,	|
#		  because it only checks for object identity.		|
# --------- bga Remove prior comments to reduce the size of the flower	|
#		box.  See revision 1.22 for earlier comments.		|
#--(H-)-----------------------------------------------------------------+
"""
    PgSQL - A PyDB-SIG 2.0 compliant module for PostgreSQL.

    Copyright 2000 by Billy G. Allie <Bill.Allie@mug.org>
    See package documentation for further information on copyright.

    Inline documentation is sparse.
    See the Python DB-SIG 2.0 specification for usage information.

    basic usage:

	PgSQL.connect(connect_string) -> connection
	    connect_string = 'host:port:database:user:password:options:tty'
	    All parts are optional. You may also pass the information in as
	    keyword arguments with the following keywords: 'host', 'port',
	    'database', 'user', 'password', 'options', and 'tty'.  The port
	    may also be passed in as part of the host keyword parameter,
	    ie. host='localhost:5432'. Other optional parameters are
	    client_encoding and unicode_results. If unicode_results is true,
	    all strings from the backend are returned as Unicode strings.

	    client_encoding accepts the same parameters as the encode method
	    of Unicode strings. If you also want to set a policy for encoding
	    errors, set client_encoding to a tuple, like ("koi8-r", "replace")

	    Note that you still must make sure that the PostgreSQL client is
	    using the same encoding as set with the client_encoding parameter.
	    This is typically done by issuing a "SET CLIENT_ENCODING TO ..."
	    SQL statement immediately after creating the connection.

	connection.cursor() -> cursor
	    Create a new cursor object.  A connection can support multiple
	    cursors at the same time.

	connection.close()
	    Closes the connection now (instead of when __del__ is called).
	    The connection will be unusable from this point forward.
	    NOTE: Any uncommited transactions will be rolled back and any
	          open cursors for this connection will be closed.

	connection.commit()
	    Commit any pending transactions for this connection.

	    NOTE: This will reset any open cursors for this connection to their
		  inital state.  Any PostgreSQL portals in use by cursors will
		  be closed and any remaining query results will be discarded.

	connection.rollback()
	    Rollback any pending transactions for this connection.

	    NOTE: This will reset any open cursors for this connection to their
		  inital state.  Any PostgreSQL portals in use by cursors will
		  be closed and any remaining query results will be discarded.

	connection.binary(string) -> PgLargeObject
	    Create a new PostgreSQL large object.  If string is present, it is
	    written to the new large object.  The returned large object will
	    not be opened (i.e. it will be closed).
	
	connection.un_link(OID|PgLargeObject)
	    Un-links (removes) the PgLargeObject from the database.

	    NOTE: This is a PostgreSQL extension to the Connection object.
		  It is not safe to un-link PgLargeObjects while in a trans-
		  action (in versions prior to 7.1) since this action can not
		  be rollbacked completely.  Therefore, an attempt to un-link
		  while in a transaction (in versions prior to 7.1) will raise
		  an exception.

	connection.version
	    This instance of the PgVersion class contains information about
	    the version of the PostgreSQL backend to which the connection
	    object is connected to.

	    NOTE: This is a PgSQL extension to the Connection object.

	cursor.execute(query[, param1[, param2, ..., paramN])
	    Execute a query, binding the parameters if they are passed. The
	    binding syntax is the same as the '%' operator except that only %s
	    [or %(name)s] should be used and the %s [or %(name)s] should not be
	    quoted.  Any necessary quoting will be performed by the execute
	    method.

	cursor.execute(query[, sequence])
	    Execute a query, binding the contents of the sequence as parameters.
	    The binding syntax is the same as the '%' operator except that only
	    %s should be used and the %s should not be quoted.  Any necessary
	    quoting will be performed by the execute method.

	cursor.execute(query[, dictionary])
	    Execute a query, binding the contents of the dictionary as para-
	    meters.  The binding syntax is the same as the '%' operator except
	    that only %s [or %(name)s] should be used and the %s [or %(name)s]
	    should not be quoted.  Any necessary quoting will be performed by
	    the execute method.

	cursor.debug
	    Setting this attribute to 'text' will cause the query that will
	    be executed to be displayed to STDOUT.  If it is set to 'pre' or
	    'div', the query will be displayed to STDOUT within a <pre> or
	    <dif> HTML block.  If it is set to None (the default), the query
	    will not be displayed.

	NOTE: In order to use a PostgreSQL portal (i.e. DECLARE ... CURSOR
	      FOR ...), the word SELECT must be the first word in the query,
	      and can only be be proceeded by spaces and tabs.  If this is not
	      the case, a cursor will be simulated by reading the entire result
	      set into memory and handing out the results as needed.

	NOTE: PostgreSQL cursors are read-only.  SELECT ... FOR UPDATE queries
	      will not use PostgreSQL cursors, but will simulate a cursor by
	      reading the entire result set into memory and handing out the
	      results as needed.

	NOTE: Setting the variable, PgSQL.noPostgresCursor, to 1 will cause
	      PgSQL to NOT use PostgreSQL cursor (DECLARE ... CURSOR FOR ...),
	      even if all the conditions for doing so are met.  PgSQL will
	      simulate a cursor by reading the entire result set into memory
	      and handing out the results as needed.

	cursor.executemany(query, sequence_of_params)
	    Execute a query many times, once for each params in the sequence.

	NOTE: The restriction on the use of PostgreSQL cursors described in
	      the cursor.execute() note also applies to cursor.executemany().
	      The params in the sequence of params can be a list, tuple or
	      dictionary.

	cursor.fetchone() -> PgResultSet
	    Fetch the next row of the query result set as a single PgResultSet
	    containing the column data.  Returns None when no more rows are
	    available.  A PgResultSet is a sequence that can be indexed by
	    a column name in addition to an integer.

	cursor.fetchmany([size]) -> [PgResultSet, ...]
	    Fetch the next set of rows from the query result, returning a
	    sequence of PgResultSets.  An empty sequence is returned when
	    no more rows are available.

	    The number of rows returned is determined by the size parameter.
	    If the size parameter is ommited, the value of cursor.arraysize
	    is used.  If size is given, cursor.arraysize is set to that value.

	cursor.fetchall() -> [PgResultSet, ...]
	    Fetch all (remaining) rows from the query result, returning a
	    sequence of PgResultSets.  An empty sequence is returned when
	    no more rows are available.

	cursor.description -> [(column info), ... ]
	    Returns a sequence of 8-item tuples.  Each tuple describes one
	    column of the result: (name, type code, display size, internal
	    size, precision, scale, null_ok, isArray).

	    NOTE: null_ok is not implemented.
		  isArray is a PostgreSQL specific extension.

	cursor.rowcount
	    The number of rows the last execute produced (for DQL statements)
	    or affected (for DML statement).

	cursor.oidValue
	    The object ID of the inserted record, if the last SQL command
	    was an INSERT, otherwise it returns 0 (aka. InvalidOid)

	    NOTE: oidValue is a PostgreSQL specific extension.

	cursor.close()
	    Closes the cursor now (instead of when __del__ is called).  The
	    cursor will be unusable from this point forward.

	cursor.rewind()
	    Moves the cursor back to the beginning of the query result.
	    This is a PgSQL extension to the PyDB 2.0 API.

	PgResultSet.description() -> [(column info), ... ]
	    Returns a sequence of 8-item tuples.  Each tuple describes one
	    column of the result: (name, type code, display size, internal
	    size, precision, scale, null_ok. isArray).

	    NOTE: null_ok is not implemented.
		  isArray is a PostgreSQL specific extension.

	PgResultSet.<column name> -> value
	    Column names are attributes to the PgResultSet.
	
	Note:	Setting the variable, PgSQL.fetchReturnsList = 1 will cause
		the fetch*() methods to return a list instead of a PgResultSet.

	PgSQL.version
	    This string object contains the version number of PgSQL.
"""

from types import *
from sys import getrefcount, getdefaultencoding
import sys
import copy
import string
import re
import new

try:
    import weakref
    noWeakRef = 0
except:
    noWeakRef = 1

# Define boolean constants for old Python versions
try:
    True
except:
    True, False = 1, 0

try:
    from mx import DateTime
except ImportError:
    raise ImportError, \
	"""You need to install mxDateTime
	(http://www.egenix.com/files/python/eGenix-mx-Extensions.html)"""
from libpq import *

version = '$Revision: 1.50 $'

apilevel = '2.0'
threadsafety = 1
paramstyle = 'pyformat'

# Setting this variable to 1 will cause the fetch*() methods to return
# a list instead of a PgResultSet

fetchReturnsList = 0

# Setting this variable to 1 will prevent the use of a PostgreSQL Cursor
# (via the "DECLARE ... CURSOR FOR ..." syntax).  A cursor will be simulated
# by reading all of the query result into memory and doling out the results
# as needed.

noPostgresCursor = 0

# Setting this variable to 1 will cause the datatime instance returned from 
# the result set for a timestame with timezone to reference the corresponding
# UTC time value (not the value expressed in the clients time zone).

useUTCtimeValue = 0

# Define the regular expresions used to determin the type of SQL statement
# that is being executed.

re_DQL = re.compile('^[\s]*SELECT[\s]', re.I)
re_DRT = re.compile('[\s]*DROP[\s]+TABLE[\s]', re.I)
re_DRI = re.compile('[\s]*DROP[\s]+INDEX[\s]', re.I)
re_4UP = re.compile('[\s]FOR[\s]+UPDATE', re.I)
re_IN2 = re.compile('[\s]INTO[\s]', re.I)

replace = string.replace

#-----------------------------------------------------------------------+
# Make the required Date/Time constructor visable in the PgSQL module.	|
#-----------------------------------------------------------------------+

Date = DateTime.Date
Time = DateTime.Time
Timestamp = DateTime.Timestamp
DateFromTicks = DateTime.DateFromTicks
TimeFromTicks = DateTime.TimeFromTicks
TimestampFromTicks = DateTime.TimestampFromTicks

#--------------------------------------------------+
# The RelativeDateTime type for PgInterval support |
#--------------------------------------------------+

RelativeDateTime = DateTime.RelativeDateTime

#-----------------------------------------------+
# The DateTimeDelta type for PgInterval support |
#-----------------------------------------------+

DateTimeDelta = DateTime.DateTimeDelta

#-------------------------------+
# Also the DateTime types	|
#-------------------------------+

DateTimeType = DateTime.DateTimeType
DateTimeDeltaType = DateTime.DateTimeDeltaType

#-----------------------------------------------------------------------+
# Name:		DBAPITypeObject						|
#									|
# Description:  The DBAPITypeObject class allows implementing the re-	|
#		quired DP-API 2.0 type objects even if their are multi-	|
#		ple database types for the DP-API 2.0 types.		|
#									|
# Note:		This object is taken from the Python DP-API 2.0 imple-	|
#		mentation hints.					|
#-----------------------------------------------------------------------+

class DBAPITypeObject:
    def __init__(self, name, *values):
	self.name = name
	self.values = values

    def __repr__(self):
	return self.name

    def __cmp__(self, other):
	if other in self.values:
	    return 0
	elif other < self.values:
	    return 1
	return -1

# Define the object types required by the DB-API 2.0 specification.

BINARY	 = DBAPITypeObject('BINARY', PG_OID, PG_BLOB, PG_BYTEA)

DATETIME = DBAPITypeObject('DATETIME', PG_DATE, PG_TIME, PG_TIMETZ,
				       PG_TIMESTAMP, PG_TIMESTAMPTZ,
				       PG_ABSTIME, PG_RELTIME,
                                       PG_INTERVAL, PG_TINTERVAL)

NUMBER	 = DBAPITypeObject('NUMBER', PG_INT8, PG_INT2, PG_INT4, PG_FLOAT4,
				     PG_FLOAT8, PG_MONEY, PG_NUMERIC)

ROWID	 = DBAPITypeObject('ROWID', PG_OID, PG_ROWID, PG_CID, PG_TID, PG_XID)

STRING	 = DBAPITypeObject('STRING', PG_CHAR, PG_BPCHAR, PG_TEXT, PG_VARCHAR,
				     PG_NAME)

# BOOLEAN is the PostgreSQL boolean type.

BOOLEAN  = DBAPITypeObject('BOOLEAN', PG_BOOL)

# OTHER is for PostgreSQL types that don't fit in the standard Objects.

OTHER	 = DBAPITypeObject('OTHER', PG_POINT, PG_LSEG, PG_PATH, PG_BOX,
				    PG_POLYGON, PG_LINE, PG_CIDR, PG_CIRCLE,
				    PG_INET, PG_MACADDR, PG_ACLITEM,
                                    PG_REFCURSOR)

#-----------------------------------------------------------------------+
# Name:		PgTypes							|
#									|
# Description:	PgTypes is an object wrapper for the type OID's used by	|
#		PostgreSQL.  It is used to display a meaningful text	|
#		description of the type while still allowing it to be	|
#		compared as a numeric value.				|
#-----------------------------------------------------------------------+

class PgTypes:
    def __init__(self, value):
	self.value = value

    def __coerce__(self, other):
	if type(other) in [IntType, LongType, FloatType]:
	    return (self.value, int(other))
	return None

    def __cmp__(self, other):
	return cmp(self.value, other)

    def __repr__(self):
	return repr(self.value)

    def __str__(self):
	return PQftypeName(self.value)

    def __int__(self):
	return int(self.value)

    def __long__(self):
	return long(self.value)

    def __float__(self):
	return float(self.value)

    def __complex__(self):
	return complex(self.value)

#-----------------------------------------------------------------------+
# Name:		TypeCache						|
#									|
# Description:	TypeCache is an object that defines methods to:		|
#		1.  Cast PostgreSQL result strings into the appropiate	|
#		    Python type or object [typecast()].			|
#		2.  Retrieve addition information about a type from the	|
#		    PostgreSQL system catalogs [getTypeInfo()].  This	|
#		    type information is maintained in a local cache so	|
#		    that subsequent request for the same information do	|
#		    not require a database query to fulfill it.		|
#-----------------------------------------------------------------------+

class TypeCache:
    """Type cache -- used to cache postgreSQL data type information."""

    _time_units = (
	( 'days', 'day'),
	( 'months', 'month', 'mon', 'mons' ),
	( 'years', 'year'),
    )

    def __init__(self, conn):
	if noWeakRef:
	    self.__conn = conn
	else:
	    self.__conn = weakref.proxy(conn, self.__callback)
	self.__type_cache = {}
	self.__lo_cache = {}
    
    def __callback(self, o):
	self.__conn = None

    def interval2DateTimeDelta(self, s):
	"""Parses PostgreSQL INTERVALs.
	The expected format is [[[-]YY years] [-]DD days] [-]HH:MM:SS.ss"""
	parser = DateTime.Parser.DateTimeDeltaFromString

	ydh = s.split()
	ago = 1

	result = DateTimeDelta(0) 

	# Convert any years using 365.2425 days per year, which is PostgreSQL's
	# assumption about the number of days in a year.
        if len(ydh) > 1:
            if ydh[1].lower().startswith('year'):
                result += parser('%s days' % ((int(ydh[0]) * 365.2425),))
                ydh = ydh[2:]
	
	# Convert any months using 30 days per month, which is PostgreSQL's
	# assumption about the number of days in a months IF it doesn't
	# know the end or start date of the interval. If PG DOES know either
	# date it will use the correct length of the month, eg 28-31 days.
	# However, at this stage we have no way of telling which one it
	# was. If you want to work with accurate intervals (eg. post-processing
	# them) you need to use date1-date2 syntax rather than age(date1, date2)
	# in your queries.
	#
	# This is per discussion on pgsql-general:
	#     http://www.spinics.net/lists/pgsql/msg09668.html
	# Google for: 
        #     >>>"interval output format" available that removes ambiguity<<<
	#
	# Note: Should a notice be provided to the user that post-processing
	#       year/month intervals is unsafe practice ?
	if len(ydh) > 1:
		if ydh[1].lower().startswith('mon'):
			result += parser('%s days' % ((int(ydh[0]) * 30),))
			ydh = ydh[2:]

	# Converts any days and adds it to the years (as an interval)
        if len(ydh) > 1:
            if ydh[1].lower().startswith('day'):
                result += parser('%s days' % (ydh[0],))
                ydh = ydh[2:]

	# Adds in the hours, minutes, seconds (as an interval)
        if len(ydh) > 0:
            result += parser(ydh[0])

	return result

    def interval2RelativeDateTime(self, s):
	"""Parses PostgreSQL INTERVALs.
 	The expected format is [[[-]YY years] [-]DD days] [-]HH:MM:SS.ss"""

        tokens = s.split()
        quantity = None
        result = RelativeDateTime()
        for token in tokens: 
            # looking for quantity
            if token.isdigit() or token[0] == '-' and token[1:].isdigit():
                quantity = int(token)
                continue
            # looking for unit
            elif token.isalpha() and not quantity is None:
                unitAttr = None
                for unit in self._time_units:
                    if token in unit:
                        unitAttr = unit[0]
                if unitAttr and not quantity is None:
                    setattr(result, unitAttr, quantity)
                    quantity = None
                    continue
            # looking for time
            elif token.find(':') != -1:
                hms = [ int(value) for value in token.split(':') ]
                result.hour = hms[0]
                result.minute= hms[1]
                if len(hms) == 3:
                    result.second = hms[2]
        return result

    def parseArray(self, s):
	"""Parse a PostgreSQL array strings representation.
	This parses a PostgreSQL array and return a list of the array
	elements as strings.
	"""
	class LeaveLoopException(Exception): pass

	# Get rid of the escaping in the array string
	def convertEscapes(s):
	    # If we're called with a list in a multi-dimensional
	    # array, simply return the list. We only convert the
	    # elements of the multi-dimensional array.
	    if type(s) is ListType:
		return s

	    schars = []
	    escape = 0
	    octdigits = []

	    for char in s:
		if char == '\\':
		    escape += 1
		    if escape == 2:
			schars.append(char)
			escape = 0
		else:
		    if escape:
			if char in string.digits:
			    octdigits.append(char)
			else:
			    if octdigits != []:
				curchar = chr(int(octdigits[0]) * 64) + \
					  chr(int(octdigits[1]) * 8) + \
					  chr(int(octdigits[2]))
				schars.append(curchar)
				octdigits = []
			    schars.append(char)
		    else:
			schars.append(char)
		    escape = 0
	    return "".join(schars)

	lst = PgArray()
	s = s[1:-1] # drop '{' and '}' at start/end

	# If the array is empty, return immediately
	if len(s) == 0:
	    return lst

	pos = 0
	try:
	    while 1:
		if s[pos] == '"':
		    # A quoted element, find the end-quote, which is the next
		    # quote char that is not escaped.
		    end_quote_pos = pos + 1
		    escape = 0
		    while 1:
			if s[end_quote_pos] == '\\':
			    escape = not escape
			elif s[end_quote_pos] == '"':
			    if not escape:
				break
			    escape = 0
			else:
			    escape = 0
			end_quote_pos += 1
		    lst.append(convertEscapes(s[pos + 1:end_quote_pos]))
		    
		    # Skip quote char and next comma
		    pos = end_quote_pos + 2

		    # If end-of-string. leave loop.
		    if pos >= len(s):
			break
		else:
		    # This array element is not quoted, so it ends either at
		    # the next comma that isn't escaped, or at the end of the
		    # string, or, if it contains a subarray, at the position
		    # of the corresponding curly brace.
		    if s[pos] != '{':
			next_comma_pos = pos + 1
			escape = 0
			while 1:
			    if next_comma_pos >= len(s):
				# This is the last array element.
				lst.append(convertEscapes(s[pos:]))
				raise LeaveLoopException

			    if s[next_comma_pos] == '\\':
				escape = not escape
			    elif s[next_comma_pos] == ',':
				if not escape:
				    break
				escape = 0
			    else:
				escape = 0
			    next_comma_pos += 1

			curelem = s[pos:next_comma_pos]
			if curelem.startswith("{"):
			    lst.append(self.parseArray(curelem[1:-1]))
			else:
			    lst.append(convertEscapes(curelem))
			pos = next_comma_pos + 1
			if s[pos] == ',':
			    pos += 1
		    else:
			# The current character is '{', which means we've
			# found a sub-array:
			# We find the end of the sub-array, then feed this
			# string into parseArray again.
			escape = 0
			open_braces = 1
			closing_brace_pos = pos + 1
			in_quotes = 0
			while 1:
			    if s[closing_brace_pos] == '\\':
				escape = not escape
			    elif s[closing_brace_pos] == '{':
				if (not escape) and (not in_quotes):
				    open_braces += 1
				escape = 0
			    elif s[closing_brace_pos] == '}':
				if (not escape) and (not in_quotes):
				    open_braces -= 1
				    if open_braces == 0:
					break
				escape = 0
			    elif s[closing_brace_pos] == '"':
				if not escape:
				    in_quotes = not in_quotes
				escape = 0
			    else:
				escape = 0
			    closing_brace_pos += 1

			curelem = s[pos:closing_brace_pos + 1]
			lst.append(self.parseArray(curelem))
			pos = closing_brace_pos + 1
			if pos >= len(s):
			    break
			if s[pos] == ',':
			    pos += 1

	except LeaveLoopException:
	    pass

	#lst = map(convertEscapes, lst)
	return lst

    def typecast(self, colinfo, value):
	"""
	typecast(rowinfo, value)
	    Convert certain postgreSQL data types into the appropiate python
	    object."""

	if value is None:
	    return value

	_fn, _ft, _ds, _is, _p, _s, _nu, _ia = colinfo

	_ftv = _ft.value
	
	if _ia:
	    # Convert string representation of the array into PgArray object.
	    _list = self.parseArray(value)
	    return self.handleArray(colinfo, _list)

	if _ftv == PG_INT2:
	    if type(value) is PgInt2Type:
		return value
	    else:
		return int(value)
	elif _ftv == PG_INT4:
	    if type(value) is IntType:
		return value
	    else:
		return int(value)
	elif _ftv == PG_INT8 or _ftv == ROWID:
	    if type(PgInt8) is ClassType:
		if isinstance(value, PgInt8):
		    return value
	    else:
		if type(value) is PgInt8Type:
		    return value
	    return long(value)
	elif _ftv == PG_NUMERIC:
	    if isinstance(value, PgNumeric):
		return value
	    else:
		try:
		    return PgNumeric(value, _p, _s)
		except OverflowError:
		    # If we reached this point, then the precision and scale
		    # of the current field does not match the precision and
		    # scale of the first record in the result set (there are
		    # a few reasons why this can happen).  Let PgNumeric
		    # figure out a precision and scale from the value.
		    return PgNumeric(value)
	elif _ftv == PG_MONEY:
	    if isinstance(value, PgMoney):
		return value
	    else:
		return PgMoney(value).value
	elif _ftv == DATETIME:
	    if type(value) is DateTimeType:
                if value in ('infinity', '+infinity', '-infinity'):
                    fake_infinity = '9999-12-13 23:59:59' # fake infinity
                if value[0] == '-':
                    value = '-' + fake_infinity
                else:
                    value = fake_infinity
		return value
	    else:
		if _ftv == PG_INTERVAL:
                    return self.interval2RelativeDateTime(value)
		elif _ftv == PG_TIME:
		    return DateTime.Parser.TimeFromString(value)
		else:
		    if useUTCtimeValue or _ftv == PG_DATE:
		        return DateTime.Parser.DateTimeFromString(value)
		    else:
			return DateTime.Parser.DateTimeFromString(value).localtime()
	elif _ftv == BINARY:
	    if isinstance(value, PgBytea) or type(value) is PgLargeObjectType:
		return value
	    elif type(value) is IntType:
		return PgLargeObject(self.__conn.conn, value)
	    else:
		return PgBytea(value)
        elif _ftv == PG_REFCURSOR:
            return self.__conn.cursor(value, isRefCursor=PG_True)
	elif _ftv == OTHER:
	    if isinstance(value, PgOther):
		return value
	    else:
		return value
	elif self.__conn.unicode_results \
	  and _ftv in (PG_CHAR, PG_BPCHAR, PG_TEXT, PG_VARCHAR, PG_NAME):
	    return unicode(value, *self.__conn.client_encoding)
	# Other typecasting is not needed.  It will be once support for
	# the other built-in types (ie. box, line, inet, cidr, etc) are added.
	
	return value

    def handleArray(self, colinfo, lst):
	# If the list is empty, just return the empty list.

	if len(lst) == 0:
	    return lst

	_fn, _ft, _ds, _is, _p, _s, _nu, _ia = colinfo

	_ftv = _ft.value

	for _i in range(len(lst)):
	    if isinstance(lst[_i], PgArray):
		lst[_i] = self.handleArray(colinfo, lst[_i])
	    elif _ftv == PG_INT4 or _ftv == ROWID:
		lst[_i] = int(lst[_i])
	    elif _ftv == PG_INT8:
		lst[_i] = PgInt8(lst[_i])
	    elif _ftv == PG_NUMERIC:
		try:
                    lst[_i] = PgNumeric(lst[_i], _p, _s)
		except OverflowError:
		    # If we reached this point, then the precision and scale
		    # of the current field does not match the precision and
		    # scale of the first record in the result set (there are
		    # a few reasons why this can happen).  Let PgNumeric
		    # figure out a precision and scale from the value.
                    lst[_i] = PgNumeric(lst[_i])
	    elif _ftv == PG_INT2:
		lst[_i] = PgInt2(lst[_i])
	    elif _ftv == DATETIME:
		if _fvt != PG_INTERVAL:
		    if lst[_i] in ('infinity', '+infinity', '-infinity'):
			fake_infinity = '9999-12-13 23:59:59' # fake infinity
			if lst[_i][0] == '-':
			    lst[_i] = '-' + fake_infinity
			else:
			    lst[_i] = fake_infinity
		    else:
			lst[_i] = DateTime.Parser.DateTimeFromString(lst[_i])
			#lst[_i] = DateTime.ISO.ParseAny(lst[_i])
		else:
                    lst[_i] = self.interval2RelativeDateTime(lst[_i])
	    elif _ftv == PG_MONEY:
		if lst[_i][0] == '(':
		    lst[_i] = PgMoney(-float(replace(lst[_i][2:-1], ',', '')))
		elif lst[_i][0] == '-':
		    lst[_i] = PgMoney(-float(replace(lst[_i][2:], ',', '')))
		else:
		    lst[_i] = PgMoney(float(replace(lst[_i][1:], ',', '')))
	    elif _ftv == BINARY:
		if _ftv == PG_BYTEA:
		    # There is no need to un-escape lst[_i], it's already been
		    # done when the PostgreSQL array was converted to a list
		    # via parseArray().
		    lst[_i] = PgBytea(lst[_i])
		else:
		    lst[_i] = PgLargeObject(self.__conn.conn, int(lst[_i]))
		    
	return lst

    def getTypeInfo(self, pgtype):
	try:
	    return self.__type_cache[pgtype.value]
	except KeyError:
	    _nl = len(self.__conn.notices)
	    _res = self.__conn.conn.query("SELECT typname, -1 , typelem "
					  "FROM pg_type "
					  "WHERE oid = %s" % pgtype.value)

	    if len(self.__conn.notices) != _nl:
		raise Warning, self.__conn.notices.pop()

	    _n = _res.getvalue(0, 0)
	    _p = _res.getvalue(0, 1)
	    _b = _res.getvalue(0, 2)
	    if _n[0] == '_':
		_n = _n[1:]
		_i = 1
	    else:
		_i = 0

	    self.__type_cache[pgtype.value] = (_n, _p, _i, PgTypes(_b))
	
	return self.__type_cache[pgtype.value]

#-----------------------------------------------------------------------+
# Name:		PgOther							|
#									|
# Description:	A Python wrapper class for the PostgreSQL types that do	|
#		not (yet) have an implementation in python.  The number	|
#		of types in this category will shrink as more wrappers	|
#		are implemented.					|
#									|
# Note:		A Python String is used to store the PostgreSQL type in	|
#		class.							|
#-----------------------------------------------------------------------+

class PgOther:
    def __init__(self, value):
	if type(value) is not StringType:
	    raise TypeError, "argument must be a string."

	self.value = value

	if hasattr(value, '__methods__'):
	    for i in self.value.__methods__:
		exec 'self.%s = self.value.%s' % (i, i)

    # This definition of __coerce__ will cause Python to always call the
    # (existing) arithmatic operators for this class.  We can the perform the
    # appropiate operation on the base type, letting it decide what to do.
    def __coerce__(self, other):
	    return (self, other)

    def __getitem__(self, index):
	if type(index) is SliceType:
	    if index.step is None:
		return PgOther(self.value[index.start:index.stop])
	    else:
		return PgOther(self.value[index.start:index.stop:index.step])

	return self.value[index];
    
    def __setitem__(self, index, item):
        raise TypeError, "object doesn't support slice assignment"

    def __delitem__(self, index):
        raise TypeError, "object doesn't support slice deletion"
    
    def __contains__(self, item):
	return (item in self.value)

    if sys.version_info < (2, 0):
	# They won't be defined if version is at least 2.0 final
	def __getslice__(self, i, j):
	    return PgOther(self.value[max(0, i):max(0, j)])

	def __setslice__(self, i, j, seq):
            raise TypeError, "object doesn't support slice assignment"
        
	def __delslice__(self, i, j):
            raise TypeError, "object doesn't support slice deletion"

    # NOTE: A string is being concatenated to a PgOther, so the result type
    #       is a PgOther
    def __add__(self, other):
	return PgOther((self.value + other))

    # NOTE: A PgOther is being concatenated to a string, so the result type
    #       is a string.
    def __radd__(self, other):
	return (other + self.value)

    def __mul__(self, other):
	return PgOther((self.value * other))

    def __rmul__(self, other):
	return PgOther((self.value * other))

    def __repr__(self):
	return repr(self.value)

    def __str__(self):
	return str(self.value)

    def __hash__(self):
        return hash(self.value)

    def __cmp__(self, other):
        return cmp(self.value, other)

    def __rcmp__(self, other):
        return cmp(other, self.value)

    def __lt__(self, other):
        return self.value < other

    def __le__(self, other):
        return self.value <= other

    def __eq__(self, other):
        return self.value == other

    def __ne__(self, other):
        return self.value != other

    def __gt__(self, other):
        return self.value > other

    def __ge__(self, other):
        return self.value >= other

    # NOTE: A PgOther object will use the PgQuoteString() function in libpq.
    def _quote(self, forArray=0):
	if self.value:
	    return PgQuoteString(self.value, forArray)
	return 'NULL'

#-----------------------------------------------------------------------+
# Name:		PgArray							|
#									|
# Description:	A Python wrapper class for PostgreSQL arrays.		|
#		It is used so that the list type can be used as an arg-	|
#		ument to Connection.execute() without being treated as	|
#		a PostgreSQL array.					|
#-----------------------------------------------------------------------+

class PgArray:
    def __init__(self, value=None):
	if value is None:
	    self.value = []
	    return

	if type(value) is not ListType and not isinstance(value, PgArray):
	    raise TypeError, "argument must be a list or a PgArray."

	# We have to insure that nested mutable sequences (list and PgArray)
	# get copied.
	for i in range(len(value)):
	    if type(value[i]) is ListType or isinstance(value[i], PgArray):
		value[i] = PgArray(value[i][:])

	self.value = value

    # Define the methods used
    def append(self, item):
	if type(item) is ListType:
	    item = PgArray(item)
	self.value.append(item)

    def count(self, item):
	return self.value.count(item)

    def extend(self, item):
	if type(item) is ListType:
	    item = PgArray(item)
	self.value.extend(item)

    def index(self, item):
	return self.value.index(item)

    def insert(self, key, item):
	if type(item) is ListType:
	    item = PgArray(item)
	self.value.insert(key, item)

    def pop(self, key=-1):
	return self.value.pop(key)

    def remove(self, item):
	self.value.remove(item)

    def reverse(self):
	self.value.reverse()

    def sort(self, compfunc=None):
	if compfunc is None:
	    self.value.sort()
	else:
	    self.value.sort(compfunc)

    def __cmp__(self, other):
	if not isinstance(other, PgArray):
	    return cmp(id(self), id(other))

	# PgArray objects are considered equal if:
	#   1.  The lengh of the PgArray objects are equal and
	#   2.  Each item[k] in the PgArray objects are equal.

	res = cmp(len(self), len(other))
	if res != 0:
	    return res

	for i in range(len(self.value)):
	    res = cmp(self.value[i], other.value[i])
	    if res != 0:
		return res

	return 0

    def __len__(self):
	return len(self.value)

    def __getitem__(self, index):
	if type(index) is SliceType:
	    if index.step is None:
		return PgArray(self.value[index.start:index.stop])
	    else:
		return PgArray(self.value[index.start:index.stop:index.step])

	return self.value[index];
    
    def __setitem__(self, index, item):
	if type(item) is ListType:
	    item = PgArray(item)

	if type(index) is SliceType:
	    if index.step is None:
		self.value[index.start:index.stop] = item
	    else:
		self.value[index.start:index.stop:index.step] = item
	else:
	    self.value[index] = item

    def __delitem__(self, index):
	if type(index) is SliceType:
	    if index.step is None:
		del self.value[index.start:index.stop]
	    else:
		del self.value[index.start:index.stop:index.step]
	else:
	    del self.value[index];
    
    def __contains__(self, item):
	return (item in self.value)

    if sys.version_info < (2, 0):
	# They won't be defined if version is at least 2.0 final
	def __getslice__(self, i, j):
	    return PgArray(self.value[max(0, i):max(0, j)])

	def __setslice__(self, i, j, seq):
	    if type(seq) is ListType:
		seq = PgArray(seq)
	    self.value[max(0, i):max(0, j)] = seq.value

	def __delslice__(self, i, j):
	    del self.value[max(0, i):max(0, j)]

    def __add__(self, other):
	return PgArray((self.value + other))

    def __radd__(self, other):
	return PgArray(other + self.value)

    def __iadd__(self, other):
	value = value + other
	return self

    def __mul__(self, other):
	return PgArray((self.value * other))

    def __rmul__(self, other):
	return PgArray((self.value * other))

    def __imul__(self, other):
	value = value * other
	return self

    def __repr__(self):
	return repr(self.value)
    
    def __str__(self):
	return str(self.value)

    # NOTE: A PgArray object will use the _handleArray() function to quote
    #	    itself.
    def _quote(self, forArray=0):
	if self.value:
	    return _handleArray(self.value)
	return 'NULL'

#-----------------------------------------------------------------------+
# Name:		PgBytea							|
#									|
# Description:	A Python wrapper class for the PostgreSQL BYTEA type.	|
#									|
# Note:		A Python String is used to store the PostgreSQL type in	|
#		class.							|
#-----------------------------------------------------------------------+

class PgBytea:
    def __init__(self, value):
	if type(value) is not StringType:
	    raise TypeError, "argument must be a string."

	self.value = value

	if hasattr(value, '__methods__'):
	    for i in self.value.__methods__:
		exec 'self.%s = self.value.%s' % (i, i)

    # This definition of __coerce__ will cause Python to always call the
    # (existing) arithmatic operators for this class.  We can the perform the
    # appropiate operation on the base type, letting it decide what to do.
    def __coerce__(self, other):
	    return (self, other)

    def __getitem__(self, index):
	if type(index) is SliceType:
	    if index.step is None:
		return PgBytea(self.value[index.start:index.stop])
	    else:
		return PgBytea(self.value[index.start:index.stop:index.step])

	return self.value[index];
    
    def __setitem__(self, index, item):
        raise TypeError, "object doesn't support slice assignment"

    def __delitem__(self, index):
        raise TypeError, "object doesn't support slice deletion"
    
    def __contains__(self, item):
	return (item in self.value)

    if sys.version_info < (2, 0):
	# They won't be defined if version is at least 2.0 final
	def __getslice__(self, i, j):
	    return PgBytea(self.value[max(0, i):max(0, j)])

	def __setslice__(self, i, j, seq):
            raise TypeError, "object doesn't support slice assignment"
        
	def __delslice__(self, i, j):
            raise TypeError, "object doesn't support slice deletion"

    def __add__(self, other):
	return PgBytea((self.value + other))

    def __radd__(self, other):
	return PgBytea(other + self.value)

    def __mul__(self, other):
	return PgBytea((self.value * other))

    def __rmul__(self, other):
	return PgBytea((self.value * other))

    def __repr__(self):
	return repr(self.value)
    
    def __str__(self):
	return str(self.value)

    def __hash__(self):
        return hash(self.value)

    def __cmp__(self, other):
        return cmp(self.value, other)

    def __rcmp__(self, other):
        return cmp(other, self.value)

    def __lt__(self, other):
        return self.value < other

    def __le__(self, other):
        return self.value <= other

    def __eq__(self, other):
        return self.value == other

    def __ne__(self, other):
        return self.value != other

    def __gt__(self, other):
        return self.value > other

    def __ge__(self, other):
        return self.value >= other
    
    # NOTE: A PgBytea object will use the PgQuoteBytea() function in libpq
    def _quote(self, forArray=0):
	if self.value:
	    return PgQuoteBytea(self.value, forArray)
	return 'NULL'

#-----------------------------------------------------------------------+
# Name:		PgNumeric						|
#									|
# Description:	A Python wrapper class for the PostgreSQL numeric type.	|
#		It implements addition, subtraction, mulitplcation, and	|
#		division of scaled, fixed precision numbers.		|
#									|
# Note:		The PgNumeric class uses a Python Long type to store	|
#		the PostgreSQL numeric type.				|
#-----------------------------------------------------------------------+
    
class PgNumeric:
    def __init__(self, value, prec=None, scale=None):
	if type(value) in [IntType, LongType] or value is None:
	    if prec is None or scale is None:
		raise TypeError, \
		      "you must supply precision and scale when value is a " \
		      "integer, long, or None"
	    if value is None:
		self.__v = value
	    else:
		self.__v = long(value)
		# Check to see if the value is too large for the given
		# precision/scale
		_v = str(abs(value))
		if _v[-1:] == 'L':
		    _v = _v[:-1]
		if len(_v) > prec:
		    raise OverflowError, "value too large for PgNumeric"

	    self.__p = prec
	    self.__s = scale
        elif type(value) in (FloatType, StringType):
	    # Get the value to convert as a string.
	    # The expected input is in the form of [+|-][d]*[.[d]*][e[d]+]

	    # First get the value as a (trimmed) string
	    if type(value) is FloatType:
		_v = str(value)
	    else:
		_v = value.split()
		if len(_v) == 0 or len(_v) > 1:
		    raise ValueError, \
			  "invalid literal for PgNumeric: %s" % value
		_v = _v[0]
	    
	    # Save the sign character (if any) and remove from the string
	    _sign = '+'
	    if _v[0] in ('-', '+'):
		_sign = _v[0]
		_v = _v[1:]

	    # Split the remaining string into int part, frac part and exponet
            _d = _v.rfind('.')
            _e = _v.rfind('e')

	    # Ensure that _e and _v contains a sane value
	    if _e < 0:
		_e = _v.rfind('E')
		if _e < 0:
		    _e = len(_v)

	    if _d < 0:
	        _d = _e

	    _ip = _v[:_d]
	    _fp = _v[_d+1:_e]
	    _xp = _v[_e+1:]

	    # Check the validity of the input
	    if len(_ip) == 0 and len(_fp) == 0:
		raise ValueError, \
		      "invalid literal for PgNumeric: %s" % value

            if len(_xp) > 0:
		try:
		    _exp = int(_xp)
		except:
		    raise ValueError, \
			  "invalid literal for PgNumeric: %s" % value
	    else:
		_exp = 0

            if _exp > 999:
                raise OverflowError, "value too large for PgNumeric"

            if _exp < -999:
                raise OverflowError, "value too small for PgNumeric"

	    # Create the string that will become the base (long) object
	    _v = _ip + _fp

	    _sc = len(_fp)
            if _exp > 0:
		if _exp > _sc:
		    _v = _v + ("0" * (_exp - _sc))
		    _sc = 0
		else:
		    _sc = _sc - _exp
            else:
                _sc = _sc - _exp

            try:
                self.__v = long(_sign + _v)
            except:
                raise ValueError, \
                      "invalid literal for PgNumeric: %s" % value

            self.__p = len(_v)
            if self.__p < _sc:
                self.__p = _sc
            self.__s = _sc

	    # Now adjust for the inputted scale (if any)
	    if scale is None:
		pass
	    else:
		_adj = scale - self.__s
		if _adj > 0:
		    self.__v = self.__v * (10L ** (scale - self.__s))
		elif _adj < 0:
		    self.__v = self._round(self.__v, -_adj)
		
		self.__p = self.__p + _adj
		self.__s = scale
	    
	    # Apply the inputted precision (if any)
	    if prec is None:
		pass
	    else:
		if prec > 1000:
		    raise ValueError, "precision too large for PgNumeric"
		elif self.__p > prec:
		    raise OverflowError, "value too large for PgNumeric"
		else:
		    self.__p = prec
        elif isinstance(value, PgNumeric):
            # This is used to "cast" a PgNumeric to the specified precision
            # and scale.  It can also make a copy of a PgNumeric.
            self.__v = value.__v
            if scale is not None:
                self.__s = scale
                _ds = scale - value.__s
            else:
                self.__s = value.__s
                _ds = 0
            if prec:
                self.__p = prec
            else:
                self.__p = value.__p
            # Now we adjust the value to reflect the new scaling factor.
            if _ds > 0:
                if _ds == 1:
                    self.__v = self.__v * 10
                else:
                    self.__v = self.__v * (10L ** _ds)
            elif _ds < 0:
                self.__v = self._round(self.__v, -_ds)
            if self.__v > (10L ** self.__p):
                raise OverflowError, "result exceeds precision of %d" % self.__p
        else:
	    raise TypeError, "value can not be converted to a PgNumeric."

	if self.__s > self.__p:
	    raise ValueError, \
		  "scale of %d exceeds precision of %d" % (self.__s, self.__p)

	# The value (10L ** self.__s) is used a lot.  Save it as a constant
	# to save a (small) bit of time.

	self.__sf = 10L ** self.__s

    def __fmtNumeric(self, value=None):
	if value is None:
	    _v = self.__v
	else:
	    _v = value

	# Check for a negative value and adjust if necessary
	if _v < 0:
	    _sign = '-'
	    _v = -_v
	else:
	    _sign = ''
	_v = str(_v)

	# Check to see if the string representation of the python long has
	# a trailing 'L', if so, remove it.  Python 1.5 has the trailing 'L',
	# Python 1.6 does not.
	if _v[-1:] == 'L':
	    _v = _v[:-1]

	# Check to see if the numeric is less than one and fix string if so.
	if len(_v) <= self.__s:
	    _v = ("0" * (self.__s - len(_v) + 1)) + _v

	if self.__s:
	    _s = "%s%s.%s" % (_sign, _v[:-(self.__s)], _v[-(self.__s):])
	else:
	    _s = "%s%s" % (_sign, _v)
	return _s

    def __repr__(self):
	return "<PgNumeric instance - precision: %d scale: %d value: %s>" % \
	       (self.__p, self.__s, self.__fmtNumeric())

    def __str__(self):
	return self.__fmtNumeric()

    def getScale(self):
	return self.__s

    def getPrecision(self):
	return self.__p

    def __coerce__(self, other):
        if isinstance(other, PgNumeric):
            return self, other
        elif type(other) in [IntType, LongType]:
            _s = str(other)
	    if _s[-1:] == 'L':
		_s = _s[:-1] # Work around v1.5/1.6 differences
            return (self, PgNumeric(_s))
        elif type(other) == FloatType:
            return (self, PgNumeric(other))
        return None

    def _round(self, value, drop):
	if drop == 1:
	    return ((value + 5L) / 10L)
        elif drop > 1:
	    return (((value / (10L ** (drop - 1))) + 5L) / 10L)

	return value

    def __add__(self, other):
        _c = self.__coerce__(other)
        if _c is None:
            return None
        self, other = _c
	if self.__s < other.__s:
            _s = self.__v * (other.__sf / self.__sf)
            _o = other.__v
	elif self.__s > other.__s:
            _s = self.__v
            _o = other.__v * (self.__sf / other.__sf)
        else:
            _s = self.__v
            _o = other.__v

	mp = max(self.__p - self.__s, other.__p - other.__s)
	ms = max(self.__s, other.__s)
	v = _s + _o
	# Check to see if the addition caused an increase in the precision
	# due to a carry. If so, compensate for it.
	if (v / (10L ** (mp + ms))) > 0:
	    mp = mp + 1

	return PgNumeric((_s + _o), (mp + ms), ms)

    def __radd__(self, other):
	return self.__add__(other)

    def __iadd__(self, other):
        _r = self.__add__(other)
        if _r is None:
            return None
        self.__v  = _r.__v
        self.__p  = _r.__p
        self.__s  = _r.__s
        self.__sf = _r.__sf
	return self
        
    def __sub__(self, other):
        _c = self.__coerce__(other)
        if _c is None:
            return None
        self, other = _c
	if self.__s < other.__s:
            _s = self.__v * (other.__sf / self.__sf)
            _o = other.__v
	elif self.__s > other.__s:
            _s = self.__v
            _o = other.__v * (self.__sf / other.__sf)
        else:
            _s = self.__v
            _o = other.__v

	mp = max(self.__p - self.__s, other.__p - other.__s)
	ms = max(self.__s, other.__s)
	return PgNumeric((_s - _o), (mp + ms), ms)

    def __rsub__(self, other):
	return other.__sub__(self)
        
    def __isub__(self, other):
        _r = self.__sub__(other)
        if _r is None:
            return None
        self.__v  = _r.__v
        self.__p  = _r.__p
        self.__s  = _r.__s
        self.__sf = _r.__sf
	return self

    def __mul__(self, other):
        _c = self.__coerce__(other)
        if _c is None:
            return None
        self, other = _c
	_p = self.__v * other.__v
	return PgNumeric(_p, self.__p + other.__p, self.__s + other.__s)

    def __rmul__(self, other):
	return self.__mul__(other)

    def __imul__(self, other):
        _r = self.__mul__(other)
        if _r is None:
            return None
        self.__v  = _r.__v
        self.__p  = _r.__p
        self.__s  = _r.__s
        self.__sf = _r.__sf
	return self

    def __div__(self, other):
        _c = self.__coerce__(other)
        if _c is None:
            return None
        self, other = _c
	_n = self.__v * other.__sf * self.__sf
	_d = other.__v
	_q = self._round((_n / _d), self.__s)
	return PgNumeric(_q, self.__p, self.__s)

    def __rdiv__(self, other):
	return other.__div__(self)

    def __idiv__(self, other):
        _r = self.__div__(other)
        if _r is None:
            return None
        self.__v  = _r.__v
        self.__p  = _r.__p
        self.__s  = _r.__s
        self.__sf = _r.__sf
	return self

    def __cmp__(self, other):
        if other is None:
            return 1
        _c = self.__coerce__(other)
        if _c is None:
            return 1
        self, other = _c
	if self.__s < other.__s:
	    _s = self.__v * (other.__sf / self.__sf)
	    _o = other.__v
	elif self.__s > other.__s:
	    _s = self.__v
	    _o = other.__v * (self.__sf / other.__sf)
	else:
	    _s = self.__v
	    _o = other.__v
	return cmp(_s, _o)

    def __neg__(self):
	return PgNumeric(-self.__v, self.__p, self.__s)

    def __nonzero__(self):
	return self.__v not in (None, 0)

    def __pos__(self):
	return PgNumeric(self.__v, self.__p, self.__s)

    def __abs__(self):
	if self.__v >= 0:
	    return PgNumeric(self.__v, self.__p, self.__s)
	else:
	    return PgNumeric(-self.__v, self.__p, self.__s)

    def _quote(self, forArray=0):
	if self.__v is not None:
	    if forArray:
		return '"%s"' % self.__fmtNumeric()
	    else:
		return self.__fmtNumeric()
	return 'NULL'

    def __int__(self):
	return int(self.__v / self.__sf)

    def __long__(self):
	return self.__v / self.__sf

    def __float__(self):
	v = self.__v
	s = self.__sf
	return (float(v / s) + (float(v % s) / float(s)))

    def __complex__(self):
	return complex(self.__float__())

    def __hash__(self):
	if self.__s == 0:
	    return hash(self.__v)
	v = self.__v / self.__sf
	if (v * self.__sf) == self.__v:
	    return hash(v)
	return hash(float(self))

#-----------------------------------------------------------------------+
# Name:		PgMoney							|
#									|
# Description:	A Python wrapper class for the PostgreSQL Money type.	|
#		It's primary purpose it to check for overflow during	|
#		calulations and to provide formatted output.		|
#									|
# Note:		The PgMoney class uses a Python Floating point number	|
#		represent a PostgreSQL money type.			|
#-----------------------------------------------------------------------+

class PgMoney:
    def __init__(self, value):
	if value is None:
	    self.value = value
	    return

	if type(value) is StringType:
	    if value[0] == '(':
		self.value = PgMoney(-float(replace(value[2:-1], ',', '')))
	    elif value[0] == '-':
		self.value = PgMoney(-float(replace(value[2:], ',', '')))
	    else:
		self.value = PgMoney(float(replace(value[1:], ',', '')))
	else:
	    self.value = float(value)

	if self.value < -21474836.48 or self.value > 21474836.47:
	    raise OverflowError, 'money initialization'
	

    def __checkresult(self, value, op):
	if value < -21474836.48 or value > 21474836.47:
	    raise OverflowError, 'money %s' % op
	return PgMoney(value)

    def __coerce__(self, other):
	if other is None:
	    return None
	res = coerce(self.value, other)
	if res is None:
	    return None
	_s, _o = res
	return (self, _o)

    def __hash__(self):
	return hash(self.value)

    def __cmp__(self, other):
        return cmp(self.value, other)

    def __add__(self, other):
	return self.__checkresult(self.value + other, "addition")

    def __sub__(self, other):
 	return self.__checkresult(self.value - other, "subtraction")

    def __mul__(self, other):
	return self.__checkresult(self.value * other, "mulitplication")

    def __div__(self, other):
	return self.__checkresult(self.value / other, "division")

    def __divmod__(self, other):
	_a, _b = divmod(self.value, other)
	return (self.__checkresult(_a, "divmod"), _b)

    def __pow__(self, other, modulus=None):
	return self.__checkresult(pow(self.value, other, modulus), "pow")

    def __radd__(self, other):
	return self.__checkresult(other + self.value, "addition")

    def __rsub__(self, other):
	return self.__checkresult(other - self.value, "subtraction")

    def __rmul__(self, other):
	return self.__checkresult(other * self.value, "multiplication")

    def __rdiv__(self, other):
	return self.__checkresult(other / self.value, "division")

    def __rdivmod__(self, other):
	_a, _b = divmod(other, self.value)
	return (self.__checkresult(_a, "divmod"), _b)

    def __rpow__(self, other, modulus=None):
	return self.__checkresult(pow(other, self.value, modulus), "pow")

    def __neg__(self):
	return self.__checkresult(self.value * -1, "negation")

    def __pos__(self):
	return self.value

    def __abs__(self):
	return self.__checkresult(abs(self.value), "abs")

    def __complex__(self):
	return complex(self.value)

    def __int__(self):
	return int(self.value)

    def __long__(self):
	return long(self.value)

    def __float__(self):
	return self.value	# PgMoney is already a float :-)

    def __repr__(self):
	return '%.2f' % self.value

    def __str__(self):
	_s = '%.2f' % abs(self.value)
	_i = string.rfind(_s, '.')
	_c = (_i - 1) / 3
	for _j in range(_c):
	    _i = _i - 3
	    _s = '%s,%s' % (_s[:_i], _s[_i:])
	if self.value < 0.0:
	    return '($%s)' % _s
	else:
	    return '$%s' % _s

    def _quote(self, forArray=0):
	if self.value is not None:
	    if forArray:
		return '"%s"' % str(self.value)
	    else:
		return str(self.value)
	return 'NULL'

# PgInt2 and PgInt8 used to be custom number types that checked for overflows
# in their specific ranges. In order to make them work across current Python
# versions we got rid of them and replaced them with int and long.

PgInt2 = int
PgInt2Type = IntType

PgInt8 = long
PgInt8Type = LongType

#-----------------------------------------------------------------------+
# Name:		PgResultSet						|
#									|
# Description:	This class defines the DB-API query result set for a	|
#		single row.  It emulates a sequence  with the added	|
#	        feature of being able to reference an attribute by	|
#		column name in addition to a zero-based numeric index.	|
#									|
#		This class isn't used directly, instead it's used as a	|
#		base class for the actual result set class created with	|
#		make_PgResultSetClass.					|
#									|
#-----------------------------------------------------------------------+

class PgResultSet:

    # It may not be obvious what self.__class__ does:
    # Apart from the __init__ method, all methods are called on instances of a
    # class dynamically created make_PgResultSetClass, which means that when
    # you call a method, self.__class__ is *not* PgResultSet, but a subclass of
    # it created with make_PgResultSetClass (using the new module). The
    # subclass will have a class attribute called _xlatkey, which is a mapping
    # of column names to column positions.
    
    def __init__(self, value):
	self.__dict__['baseObj'] = value

    def __getattr__(self, key):
        # When retrieving column data by name as an attribute, we must be
        # aware that a column name can be defiend with mixed-case within the
        # database.  Because of this we must first check for an exact match
        # with the given key.  If that fails, then we match with the key that
        # has been changed to lower case.  Note: we are relying on the fact
        # that PostgreSQL sends column names that are not defined with mixed-
        # case to the client as lower-case names.
        keyl = key.lower()
	if self._xlatkey.has_key(key):
	    return self.baseObj[self._xlatkey[key]]
	if self._xlatkey.has_key(keyl):
	    return self.baseObj[self._xlatkey[keyl]]
	raise AttributeError, key

    # We define a __setattr__ routine that will only allow the attributes that
    # are the column names to be updated.  All other attributes are read-only.
    def __setattr__(self, key, value):
	if key in ('baseObj', '_xlatkey', '_desc_'):
	    raise AttributeError, "%s is read-only." % key

        # Try an exact match first, then the case-insensitive match.
        # See comment in __getattr__ for details.
	keyl = key.lower()
	if self._xlatkey.has_key(key):
	    self.__dict__['baseObj'][self._xlatkey[key]] = value
	elif self._xlatkey.has_key(keyl):
	    self.__dict__['baseObj'][self._xlatkey[keyl]] = value
	else:
	    raise AttributeError, key

    def __len__(self):
	return len(self.baseObj)

    def __getitem__(self, key):
	if isinstance(key, StringType):
            # Try an exact match first, then the case-insensitive match.
            # See comment in __getattr__ for details.
            try:
                key = self._xlatkey[key]
            except:
                key = self._xlatkey[key.lower()]
	return self.baseObj[key]

    def __setitem__(self, key, value):
	if isinstance(key, StringType):
            # Try an exact match first, then the case-insensitive match.
            # See comment in __getattr__ for details.
            try:
                key = self._xlatkey[key]
            except:
                key = self._xlatkey[key.lower()]
	self.baseObj[key] = value

    def __contains__(self, key):
	return self.has_key(key)

    def __getslice__(self, i, j):
	klass = make_PgResultSetClass(self._desc_[i:j])
	obj = klass(self.baseObj[i:j])
	return obj

    def __repr__(self):
	return repr(self.baseObj)

    def __str__(self):
	return str(self.baseObj)

    def __cmp__(self, other):
	return cmp(self.baseObj, other)

    def description(self):
	return self._desc_

    def keys(self):
	_k = []
	for _i in self._desc_:
	    _k.append(_i[0])
	return _k

    def values(self):
	return self.baseObj[:]

    def items(self):
	_items = []
	for i in range(len(self.baseObj)):
	    _items.append((self._desc_[i][0], self.baseObj[i]))
	return _items

    def has_key(self, key):
        # Try an exact match first, then the case-insensitive match.
        # See comment in __getattr__ for details.
        if not self._xlatkey.has_key(key):
            key = key.lower()
	return self._xlatkey.has_key(key)

    def get(self, key, defaultval=None):
	try:
            if isinstance(key, StringType):
                # Try an exact match first, then the case-insensitive match.
                # See comment in __getattr__ for details.
                try:
                    key = self._xlatkey[key]
                except:
                    key = self._xlatkey[key.lower()]
            return self[key]
	except:
	    return defaultval

def make_PgResultSetClass(description, mapname=None):
    """Dynamically create a new subclass of PgResultSet."""
    klass = new.classobj("PgResultSetConcreteClass", (PgResultSet,), {})
    klass.__dict__['_desc_'] = description

    klass.__dict__['_xlatkey'] = {}
    if mapname is None:
	for _i in range(len(description)):
	    klass.__dict__['_xlatkey'][description[_i][0]] = _i
    else:
	for k, v in mapname.items():
	    klass.__dict__['_xlatkey'][k] = v
    return klass 


#-----------------------------------------------------------------------+
# Define the PgSQL function calls:					|
#									|
#	connect()      -- connect to a PostgreSQL database.		|
#	_handleArray() -- Transform a PgArray class into a string rep-	|
#			  resenting a PostgreSQL array.			|
#	_quote()       -- Transform a Python object representing a	|
#			  PostgreSQL type into a appropiately quoted	|
#			  string that can be sent to the database in a	|
#			  UPDATE/INSERT statement.  _quote() calls the	|
#			  _handleArray() function to quote arrays.	|
#       _quoteall()    -- transforms all elements of a list or diction-	|
#			  ary using _quote.				|
#       dateTimeDelta2Interval() -- converts a DateTimeDelta type into	|
#			  a string PostgreSQL accepts as an interval.	|
#	relativeDateTime2Interval() -- converts a RelativeDateTime into |
#			  a string PostgreSQL accepts as an interval.	|
#-----------------------------------------------------------------------+

def connect(dsn=None, user=None, password=None, host=None, database=None,
		port=None, options=None, tty=None, client_encoding=None,
		unicode_results=None):
    """
connection =  PgSQL.connect(dsn[, user, password, host, database, port,
				  options, tty] [, client_encoding] 
				  [, unicode_results])
    Opens a connection to a PostgreSQL database."""
	
    _d = {}

    # Try getting values from the DSN first.
    if dsn is not None:
	try:
	    params = string.split(dsn, ":")
	    if params[0] != '':	_d["host"]     = params[0]
	    if params[1] != '':	_d["port"]     = params[1]
	    if params[2] != '':	_d["dbname"]   = params[2]
	    if params[3] != '':	_d["user"]     = params[3]
	    if params[4] != '':	_d["password"] = params[4]
	    if params[5] != '':	_d["options"]  = params[5]
	    if params[6] != '':	_d["tty"]      = params[6]
	except:
	    pass

    # Override from the keyword arguments, if needed.
    if (user is not None):	_d["user"]	= user
    if (password is not None):	_d["password"]	= password
    if (host is not None):
	_d["host"] = host
	try:
	    params = string.split(host, ":")
	    _d["host"] = params[0]
	    _d["port"] = params[1]
	except:
	    pass
    if (database is not None):	_d["dbname"]	= database
    if (port is not None):	_d["port"]	= port
    if (options is not None):	_d["options"]	= options
    if (tty is not None):	_d["tty"]	= tty

    # Build up the connection info string passed to PQconnectdb
    # via the constructor to Connection.

    connInfo = ""
    for i in _d.keys():
	connInfo = "%s%s=%s " % (connInfo, i, _d[i])
	
    return Connection(connInfo, client_encoding, unicode_results)

def _handleArray(value):
    """
_handleArray(list) -> string
    This function handle the transformation of a Python list into a string that
    can be used to update a PostgreSQL array attribute."""

    #Check for, and handle an empty list.
    if len(value) == 0:
	return '{}'

    _j = "'{"
    for _i in value:
	if _i is None:
	    _j += ","
	elif isinstance(_i, PgArray):
	    _j = _j + _handleArray(_i)[1:-1] + ','
	elif hasattr(_i, '_quote'):
	    _j = '%s%s,' % (_j, _i._quote(1))
	elif type(_i) is DateTimeType:
	    _j = '%s"%s",' % (_j, _i)
	elif type(_i) is DateTime.DateTimeDeltaType:
	    _j = '%s"%s",' % (_j, dateTimeDelta2Interval(_i))
        elif isinstance(value, RelativeDateTime):
	    _j = '%s"%s",' % (_j, relativeDateTime2Interval(_i))
	elif type(_i) is PgInt2Type or isinstance(_i, PgInt8Type):
	    _j = '%s%s,' % (_j, str(_i))
	else:
	    _j = '%s%s,' % (_j, PgQuoteString(str(_i), 1))

    return _j[:-1] + "}'"

def _quote(value, forProc=False):
    """
_quote(value) -> string
    This function transforms the Python value into a string suitable to send
    to the PostgreSQL database in a insert or update statement.  This function
    is automatically applied to all parameter sent vis an execute() call.
    Because of this an update/insert statement string in an execute() call
    should only use '%s' [or '%(name)s'] for variable subsitution without any
    quoting."""

    if value is None:
        return 'NULL'

    try:
        return value._quote()
    except AttributeError:
        pass

    if type(value) is DateTimeType:
	return "'%s'" % value
    elif type(value) is DateTimeDeltaType:
	return "'%s'" % dateTimeDelta2Interval(value)
    elif isinstance(value, RelativeDateTime):
	return "'%s'" % relativeDateTime2Interval(value)
    elif isinstance(value, StringType):
	return PgQuoteString(value)
    elif type(value) is DictType or isinstance(value, PgResultSet):
        raise TypeError, "dictionary or PgResultSet not allowed here"
    elif type(value) in [ListType, TupleType]:
        if forProc:
            raise TypeError, "lists or tuples not allowed here"
        return "(" + \
               reduce(lambda x, y: x + ", " + y, tuple(map(_quote, value))) + \
               ")"
    elif isinstance(value, LongType):
        return str(value)
    else:
	return repr(value)

def _procQuote(value):
    return _quote(value, True)
    
def _quoteall(vdict):
    """
_quoteall(vdict)->dict
    Quotes all elements in a list or dictionary to make them suitable for
    insertion."""

    if type(vdict) is DictType or isinstance(vdict, PgResultSet):
	t = {}
	for k, v in vdict.items():
	    t[k]=_quote(v)
    elif isinstance(vdict, StringType) or isinstance(vdict, UnicodeType):
	# Note: a string is a SequenceType, but is treated as a single
	#	entity, not a sequence of characters.
	t = (_quote(vdict), )
    elif type(vdict) in [ListType, TupleType]:
        t = tuple(map(_quote, vdict))
    else:
	raise TypeError, \
	      "argument to _quoteall must be a sequence or dictionary!"

    return t

def dateTimeDelta2Interval(interval):
    """
DateTimeDelta2Interval - Converts a DateTimeDelta to an interval string\n
    The input format is [+-]DD:HH:MM:SS.ss\n
    The output format is DD days HH:MM:SS.ss [ago]\n
    """

    if type(interval) is DateTimeDeltaType:
	s = str(interval)
	ago = ''
	if s[0] == '-':
	    ago = ' ago'
	    s = s[1:]
	else:
	    ago = ''
	s = s.split(':')
	if len(s) < 4:
	    return '%s:%s:%s %s' % (s[0], s[1], s[2], ago)

	return '%s days %s:%s:%s %s' % (s[0], s[1], s[2], s[3], ago)
    else:
	raise TypeException, "DateTimeDelta2Interval requires a DataTimeDelta."


def relativeDateTime2Interval(interval):
    """
relativeDateTime2Interval - Converts a RelativeDateTime to an interval string\n
    The output format is YYYY years M mons DD days HH:MM:SS\n
    """

    if type(interval) is RelativeDateTime:
	return "%s years %s mons %s days %02i:%02i:%02i" % (
		    interval.years, interval.months, interval.days,
		    interval.hours, interval.minutes, interval.seconds
		)
    else:
	raise TypeException, \
	      "relativeDateTime2Interval requires a RelativeDateTime"

#-----------------------------------------------------------------------+
# Name:		Connection						|
#									|
# Description:	Connection defines the Python DB-API 2.0 connection	|
#		object.  See the DB-API 2.0 specifiaction for details.	|
#-----------------------------------------------------------------------+

class Connection:
    """Python DB-API 2.0 Connection Object."""

    def __init__(self, connInfo, client_encoding=None, unicode_results=None):
	try:
	    self.__dict__["conn"] = PQconnectdb(connInfo)
	except Exception, m:
	    # The connection to the datadata failed.
	    # Clean up the Connection object that was created.
	    # Note: _isOpen must be defined for __del__ to work.
	    self.__dict__["_isOpen"] = None
	    del(self)
	    raise DatabaseError, m
	    
	self.__dict__["autocommit"] = 0
	self.__dict__["TransactionLevel"] = ""
	self.__dict__["notices"] = self.conn.notices
	self.__dict__["inTransaction"] = 0
	self.__dict__["version"] = self.conn.version
	self.__dict__["_isOpen"] = 1
	self.__dict__["_cache"] = TypeCache(self)
	self.__dict__["debug"] = self.conn.debug
	if noWeakRef:
	    self.__dict__["cursors"] = []
	else:
	    self.__dict__["cursors"] = weakref.WeakValueDictionary()

	self.unicode_results = unicode_results
	if type(client_encoding) in (TupleType, ListType):
	    self.client_encoding = client_encoding
	else:
	    self.client_encoding = (client_encoding or getdefaultencoding(),)

    def __del__(self):
	if self._isOpen:
	    self.close()	# Ensure that the connection is closed.

    def __setattr__(self, name, value):
	if name == "autocommit":
	    if value is None:
		raise InterfaceError, \
		    "Can't delete the autocommit attribute."
	    # Don't allow autocommit to change if there are any opened cursor
	    # associated with this connection.
	    if self.__anyLeft():
		if noWeakRef:
		    # If the are cursors left, but weak references are not
		    # available, garbage collect any cursors that are only
		    # referenced in self.cursors.

		    self.__gcCursors()

		    if len(self.cursors) > 0:
			raise AttributeError, \
			    "Can't change autocommit when a cursor is active."
		else:
		    raise AttributeError, \
			"Can't change autocommit when a cursor is active."

	    # It's possible that the connection can still have an open
	    # transaction, even though there are no active cursors.

	    if self.inTransaction:
		self.rollback()

	    if value:
		self.__dict__[name] = 1
	    else:
		self.__dict__[name] = 0
	elif name == "TransactionLevel":
	    if value is None:
		raise InterfaceError, \
		    "Can't delete the TransactinLevel attribute."
	    # Don't allow TransactionLevel to change if there are any opened
	    # cursors associated with this connection.
	    if self.__anyLeft():
		if noWeakRef:
		    # If the are cursors left, but weak references are not
		    # available, garbage collect any cursors that are only
		    # referenced in self.cursors.

		    self.__gcCursors()

		    if len(self.cursors) > 0:
			raise AttributeError, \
			"Can't change TransactionLevel when a cursor is active."
		else:
		    raise AttributeError, \
			"Can't change TransactionLevel when a cursor is active."

	    # It's possible that the connection can still have an open
	    # transaction, even though there are no active cursors.

	    if self.inTransaction:
		self.rollback()

	    if type(value) is not StringType:
		raise ValueError, "TransactionLevel must be a string."

	    if value.upper() in [ "", "READ COMMITTED", "SERIALIZABLE" ]:
		self.__dict__[name] = value.upper()
	    else:
	        raise ValueError, \
	    'TransactionLevel must be: "", "READ COMMITTED", or "SERIALIZABLE"'
	elif name in ('unicode_results', 'client_encoding'):
	    self.__dict__[name] = value
	elif name == 'debug':
	    self.conn.debug = value
	    self.__dict__["debug"] = self.conn.debug
	elif self.__dict__.has_key(name):
	    raise AttributeError, "%s is read-only." % name
	else:
            raise AttributeError, name

    def __closeCursors(self, flag=0):
	"""
	__closeCursors() - closes all cursors associated with this connection"""
	if self.__anyLeft():
	    if noWeakRef:
		curs = self.cursors[:]
	    else:
		curs = map(lambda x: x(), self.cursors.data.values())

	    for i in curs:
		if flag:
		    i.close()
		else:
		    i._Cursor__reset()

	return self.inTransaction

    def __anyLeft(self):
	if noWeakRef:
	    return len(self.cursors) > 0

	return len(self.cursors.data.keys()) > 0

    def __gcCursors(self):
	# This routine, which will be called only if weak references are not
	# available, will check the reference counts of the cursors in the
	# connection.cursors list and close any that are only referenced
	# from that list.  This will clean up deleted cursors.

	for i in self.cursors[:]:
	    # Check the reference count.  It will be 4 if it only exists in
	    # self.cursors.  The magic number for is derived from the fact
	    # that there will be 1 reference count for each of the follwoing:
	    #     self.cursors, self.cursors[:], i, and as the argument to
	    #     getrefcount(),

	    if getrefcount(i) < 5:
		i.close()

    def __setupTransaction(self):
        """
    __setupTransaction()
        Internal routine that will set up a transaction for this connection.\n"""
        self.conn.query("BEGIN WORK")
        if self.TransactionLevel != "":
	    _nl = len(self.notices)
            self.conn.query('SET TRANSACTION ISOLATION LEVEL %s' %
                                 self.TransactionLevel)
            if len(self.notices) != _nl:
                raise Warning, self.notices.pop()
        self.__dict__["inTransaction"] = 1

        
    def close(self):
	"""
    close()
	Close the connection now (rather than whenever __del__ is called).
	Any active cursors for this connection will be closed and the connection
	will be unusable from this point forward.\n"""

	if not self._isOpen:
	    raise InterfaceError, "Connection is already closed."

	if self.__closeCursors(1):
	    try:
		_nl = len(self.conn.notices)
		self.conn.query("ROLLBACK WORK")
		if len(self.notices) != _nl:
		    raise Warning, self.notices.pop()
	    except:
		pass

	self.__dict__["_cache"] = None
	self.__dict__["_isOpen"] = 0
	self.__dict__["autocommit"] = None
	self.__dict__["conn"] = None
	self.__dict__["cursors"] = None
	self.__dict__["inTransaction"] = 0
	self.__dict__["TransactionLevel"] = None
	self.__dict__["version"] = None
	self.__dict__["notices"] = None

    def commit(self):
	"""
    commit()
	Commit any pending transactions to the database.\n"""

	if not self._isOpen:
	    raise InterfaceError, "Commit failed - Connection is not open."

	if self.autocommit:
	    raise InterfaceError, "Commit failed - autocommit is on."

	if self.__closeCursors():
	    self.__dict__["inTransaction"] = 0
	    _nl = len(self.conn.notices)
	    res = self.conn.query("COMMIT WORK")
	    if len(self.notices) != _nl:
		raise Warning, self.notices.pop()
	    if res.resultStatus != COMMAND_OK:
		raise InternalError, "Commit failed - reason unknown."

    def rollback(self):
	"""
    rollback()
	Rollback to the start of any pending transactions.\n"""

	if not self._isOpen:
	    raise InterfaceError, "Rollback failed - Connection is not open."

	if self.autocommit:
	    raise InterfaceError, "Rollback failed - autocommit is on."

	if self.__closeCursors():
	    self.__dict__["inTransaction"] = 0
	    _nl = len(self.conn.notices)
	    res = self.conn.query("ROLLBACK WORK")
	    if len(self.notices) != _nl:
		raise Warning, self.notices.pop()
	    if res.resultStatus != COMMAND_OK:
		raise InternalError, \
		      	"Rollback failed - %s" % res.resultErrorMessage

    def cursor(self, name=None, isRefCursor=PG_False):
	"""
    cursor([name])
	Returns a new 'Cursor Object' (optionally named 'name')."""

	if not self._isOpen:
	    raise InterfaceError, \
		  "Create cursor failed - Connection is not open."

	return Cursor(self, name, isRefCursor)

    def binary(self, string=None):
	"""
    binary([string])
	Returns a new 'Large Object'.  If sting is present, it is used to
	initialize the large object."""

	if not self._isOpen:
	    raise InterfaceError, \
		  "Creation of large object failed - Connection is not open."

        if self.autocommit:
            raise InterfaceError, \
                  "Creation of large object failed - autocommit is on."
        
	_nl = len(self.notices)
        _ct = 0
	# Ensure that we are in a transaction for working with large objects
	if not self.inTransaction:
            self.__setupTransaction()
            _ct = 1

	_lo = self.conn.lo_creat(INV_READ | INV_WRITE)

	if len(self.notices) != _nl:
	    raise Warning, self.notices.pop()

	if string:
	    _lo.open("w")
	    _lo.write(string)
	    _lo.close()
	
	    if len(self.notices) != _nl:
		if self.inTransaction:
		    self.conn.query("ROLLBACK WORK")
		raise Warning, self.conn.notices.pop()

	return _lo

    def unlink(self, lobj):
	"""
    unlink(OID|PgLargeObject)
	Remove a large object from the database inversion file syste."""

	if not self._isOpen:
	    raise InterfaceError, \
		  "Unlink of large object failed - Connection is not open."

	if not self.version.post70 and self.inTransaction:
	    raise NotSupportedError, \
		  "unlink of a PostgreSQL Large Object in a transaction"

	if type(lobj) is IntType:
	    oid = lobj
	elif type(lobj) is PgLargeObjectType:
	    oid = lobj.oid

	_nl = len(self.conn.notices)
	res = self.conn.lo_unlink(oid)
	if len(self.notices) != _nl:
	    raise Warning, self.notices.pop()

	return res

#-----------------------------------------------------------------------+
# Name:		Cursor							|
#									|
# Description:	Cursor defines the Python DB-API 2.0 cursor object.	|
#		See the DB-API 2.0 specification for details.		|
#-----------------------------------------------------------------------+

class Cursor:
    """Python DB-API 2.0 Cursor Object."""

    def __init__(self, conn, name, isRefCursor=PG_False):
	if not isinstance(conn, Connection):
	    raise TypeError, "Cursor requires a connection."

	# Generate a unique name for the cursor is one is not given.
	if name is None:
            if isRefCursor:
                raise TypeError, "Reference cursor requires a name."
	    name = "PgSQL_%08X" % long(id(self))
	elif type(name) is not StringType:
	    raise TypeError, "Cursor name must be a string."

	# Define the public variables for this cursor.
	self.__dict__["arraysize"] = 1

	# Define the private variables for this cursor.
	if noWeakRef:
	    self.__dict__["conn"]    = conn
	else:
	    self.__dict__["conn"]    = weakref.proxy(conn)
	self.__dict__["name"]        = name

	# This needs to be defined here sot that the initial call to __reset()
	# will work.
        self.__dict__["closed"] = None
        self.__reset()

	# _varhdrsz is the length (in bytes) of the header for variable
	# sized postgreSQL data types.

	self.__dict__["_varhdrsz"] = 4

	# Add ourselves to the list of cursors for our owning connection.
	if noWeakRef:
	    self.conn.cursors.append(self)
	    if len(self.conn.cursors) > 1:
		# We have additional cursors, garbage collect them.
		self.conn._Connection__gcCursors()
	else:
	    self.conn.cursors[id(self)] = self

	if not self.conn.autocommit:
	    # Only the first created cursor begins the transaction.
	    if not self.conn.inTransaction:
                self.conn._Connection__setupTransaction()
	self.__dict__["PgResultSetClass"] = None
        
        if isRefCursor:
	    # Ok -- we've created a cursor, we will pre-fetch the first row in
	    # order to make the description array.  Note: the first call to
	    # fetchXXX will return the pre-fetched record.
            self.__dict__["closed"] = 0
	    self.res = self.conn.conn.query('FETCH 1 FROM "%s"' % self.name)
	    self._rows_ = self.res.ntuples
	    self._idx_ = 0
	    self.__makedesc__()

    def __del__(self):
	# Ensure that the cursor is closed when it is deleted.  This takes
	# care of some actions that needs to be completed when a cursor is
	# deleted, such as disassociating the cursor from the connection
	# and closing an open transaction if this is the last cursor for
	# the connection.
	if not self.closed:
	    self.close()

    def __reset(self):
	try:
	    if (self.closed == 0) and self.conn.inTransaction:
		try:
		    self.conn.conn.query('CLOSE "%s"' % self.name)
		except:
		    pass
	except:
	    pass

	self.__dict__["res"]	     = None
	# closed is a trinary variable:
	#     == None => Cursor has not been opened.
	#     ==    0 => Cursor is open.
	#     ==    1 => Curosr is closed.
	self.__dict__["closed"]    = None
	self.__dict__["description"] = None
	self.__dict__["oidValue"]    = None
	self.__dict__["_mapname"]    = None
	self.__dict__["_rows_"]      = 0
	self.__dict__["_idx_"]       = 1
	self.__dict__["rowcount"]    = -1

    def __setattr__(self, name, value):
	if self.closed:
	    raise InterfaceError, "Operation failed - the cursor is closed."

	if name in ["rowcount", "oidValue", "description"]:
	    raise AttributeError, "%s is read-only." % name
	elif self.__dict__.has_key(name):
	    self.__dict__[name] = value
	else:
	    raise AttributeError, name
	
    def __unicodeConvert(self, obj):
	if type(obj) is StringType:
	    return obj
	elif isinstance(obj, UnicodeType):
	    return obj.encode(*self.conn.client_encoding)
	elif type(obj) in (ListType, TupleType):
	    converted_obj = []
	    for item in obj:
		if isinstance(item, UnicodeType):
		    converted_obj.append(item.encode(*self.conn.client_encoding))
		else:
		    converted_obj.append(item)
	    return converted_obj
	elif type(obj) is DictType:
	    converted_obj = {}
	    for k, v in obj.items():
		if isinstance(v, UnicodeType):
		    converted_obj[k] = v.encode(*self.conn.client_encoding)
		else:
		    converted_obj[k] = v
	    return converted_obj
	elif isinstance(obj, PgResultSet):
	    obj = copy.copy(obj)
	    for k, v in obj.items():
		if isinstance(v, UnicodeType):
		    obj[k] = v.encode(*self.conn.client_encoding)
	    return obj
	else:
	    return obj
 
    def __fetchOneRow(self):
	if self._idx_ >= self._rows_:
	    self.__dict__['rowcount'] = 0
	    return None

	_j = []
	_r = self.res
	_c = self.conn._cache
	for _i in range(self.res.nfields):
	    _j.append(_c.typecast(self.description[_i],
				  _r.getvalue(self._idx_, _i)))
	
	self._idx_ = self._idx_ + 1

	self.__dict__['rowcount'] = 1

	if fetchReturnsList:
	    # Return a list (This is the minimum required by DB-API 2.0
	    # compliance).
	    return _j
	else:
            return self.PgResultSetClass(_j)

    def __fetchManyRows(self, count, iList=[]):
	_many = iList
	if count < 0:
	    while 1:
		_j = self.__fetchOneRow()
		if _j is not None:
		    _many.append(_j)
		else:
		    break
	elif count > 0:
	    for _i in xrange(count):
		_j = self.__fetchOneRow()
		if _j is not None:
		    _many.append(_j)
		else:
		    break

	self.__dict__['rowcount'] = len(_many)

	return _many

    def __makedesc__(self):
	# Since __makedesc__ will only be called after a successful query or
	# fetch, self.res will contain the information needed to build the
	# description attribute even if no rows were returned.  So, we always
	# build up the description.
	self.__dict__['description'] = []
	self._mapname = {}
	_res = self.res
	_cache = self.conn._cache
	for _i in range(_res.nfields):
	    _j = []

	    _j.append(_res.fname(_i))

	    _typ = PgTypes(_res.ftype(_i))
	    _mod = _res.fmod(_i)
	    _tn, _pl, _ia, _bt = _cache.getTypeInfo(_typ)
	    if _ia:
		_s, _pl, _s, _s = _cache.getTypeInfo(_bt)
		if _bt == PG_OID:
		    _bt = PgTypes(PG_BLOB)
		_typ = _bt
	    elif _typ.value == PG_OID:
		try:
		    _p = _res.getvalue(0, _i)
		except (ValueError, TypeError), m:
		    # We can only guess here ...
		    _typ = PgTypes(PG_BLOB)
		else:
		    if type(_p) in [PgLargeObjectType, NoneType]:
			_typ = PgTypes(PG_BLOB)
		    else:
			_typ = PgTypes(PG_ROWID)

	    _j.append(_typ)

	    # Calculate Display size, Internal size, Precision and Scale.
	    # Note: Precision and Scale only have meaning for PG_NUMERIC
	    # columns.
	    if _typ.value == PG_NUMERIC:
		if _mod == -1:
		    # We have a numeric with no scale/precision.
		    # Get them from by converting the string to a PgNumeric
		    # and pulling them form the PgNumeric object.  If that
		    # fails default to a precision of 30 with a scale of 6.
		    try:
			nv = PgNumeric(_res.getvalue(0, _i))
			_p = nv.getPrecision()
			_s = nv.getScale()
		    except (ValueError, TypeError), m:
			_p = 30
			_s = 6
		else:
		    # We hava a valid scale/precision value.  Use them.
		    _s = _mod - self._varhdrsz
		    _p = (_s >> 16) & 0xffff
		    _s = _s & 0xffff
		_j.append(None)		# Display size (always None since PG7.3)
		_j.append(_p)		# Internal (storage) size
		_j.append(_p)		# Precision
		_j.append(_s)		# Scale
	    else:
		if _pl == -1:
		    _pl = _res.fsize(_i)
		    if _pl == -1:
			_pl = _mod - self._varhdrsz
		_j.append(None)		# Display size (always None since PG7.3)
		_s = _res.fsize(_i)
		if _s == -1:
		    _s = _mod
		_j.append(_s)		# Internal (storage) size
		if _typ.value == PG_MONEY:
		    _j.append(9)	# Presicion and Scale (from
		    _j.append(2)		# the PostgreSQL doco.)
		else:
		    _j.append(None)		# Preision
		    _j.append(None)		# Scale

	    _j.append(None)	# nullOK is not implemented (yet)
	    _j.append(_ia)	# Array indicator (PostgreSQL specific)

	    self.__dict__["description"].append(_j)

	    # Add the fieldname:fieldindex to the _mapname dictionary
	    self._mapname[_j[0]] = _i

	# Create a subclass of PgResultSet. Note that we pass a copy of the
	# description to this class.
	self.PgResultSetClass = make_PgResultSetClass(self.description[:], self._mapname)

    def callproc(self, proc, *args):
	if self.closed:
	    raise InterfaceError, "callproc failed - the cursor is closed."

	if self.conn is None:
	    raise Error, "connection is closed."

	if self.closed == 0:
	    raise InterfaceError, "callproc() failed - cursor is active."
		    
	if self.conn.autocommit:
	    pass
	else:
	    if not self.conn.inTransaction:
                self.conn._Connection__setupTransaction()

	proc = self.__unicodeConvert(proc)
        if len(args) == 0:
            args = "()"
        else:
            args = "(" + \
                   reduce(lambda x, y: x + ", " + y,
                          tuple(map(_procQuote,
                                    self.__unicodeConvert(args)))) + ")"

	_qstr = "select %s%s" % (proc, args)

	_nl = len(self.conn.notices)

	try:
	    self.res = self.conn.conn.query(_qstr)
	    self._rows_ = self.res.ntuples
	    self._idx_  = 0
	    if type(self.res) is not PgResultType:
		self.__dict__['rowcount'] = -1
	    else:
		self.__dict__['oidValue'] =  self.res.oidValue
		if self.res.resultType == RESULT_DQL:
		    pass
		elif self.res.resultType == RESULT_DML:
		    self.__dict__['rowcount'] = self.res.cmdTuples
		else:
		    self.__dict__['rowcount'] = -1
	except OperationalError, msg:
	    # Uh-oh.  A fatal error occurred.  This means the current trans-
	    # action has been aborted.  Try to recover to a sane state.
	    if self.conn.inTransaction:
		self.conn.conn.query('END WORK')
		self.conn.__dict__["inTransaction"] = 0
		self.conn._Connection__closeCursors()
	    raise OperationalError, msg
	except InternalError, msg:
	    # An internal error occured.  Try to get to a sane state.
	    self.conn.__dict__["inTransaction"] = 0
	    self.conn._Connection__closeCursors_()
	    self.conn.close()
	    raise InternalError, msg

	if len(self.conn.notices) != _nl:
	    _drop = self.conn.notices[-1]
	    if _drop.find('transaction is aborted') > 0:
		raise Warning, self.conn.notices.pop()

	self._rows_ = self.res.ntuples
	self._idx_ = 0
	self.__dict__['rowcount'] = -1    # New query - no fetch occured yet.
	self.__makedesc__()

	return None

    def close(self):
	if self.closed:
	    raise InterfaceError, "The cursor is already closed."

	# Dis-associate ourselves from our cursor.
	self.__reset()
	try:
	    _cc = self.conn.cursors
	    if noWeakRef:
		_cc.remove(self)
		if (len(_cc) > 0):
		    # We have additional cursors, garbage collect them.
		    _cc._Connection__gcCursors()
	    else:
		del _cc.data[id(self)]
	except:
	    pass
	self.conn = None
	self.closed = 1

    def execute(self, query, *parms):
	if self.closed:
	    raise InterfaceError, "execute failed - the cursor is closed."

	if self.conn is None:
	    raise Error, "connection is closed."

	if self.closed == 0:
	    if re_DQL.search(query):
		# A SELECT has already been executed with this cursor object,
		# which means a PostgreSQL portal (may) have been opened.
		# Trying to open another portal will cause an error to occur,
		# so we asusme that the developer is done with the previous
		# SELECT and reset the cursor object to it's inital state.
		self.__reset()
		    
	_qstr = query
	if self.conn.autocommit:
	    pass
	else:
	    _badQuery = (self.conn.version < 70100) and \
			(re_DRT.search(query) or re_DRI.search(query))
	    if not self.conn.inTransaction:
		if _badQuery:
		    pass # PostgreSQL version < 7.1 and not in transaction,
			 # so DROP TABLE/INDEX is ok.
		else:
                    self.conn._Connection__setupTransaction()

	    if re_DQL.search(query) and \
	       not (noPostgresCursor or
	            re_IN2.search(query) or
		    re_4UP.search(query)):
		_qstr = 'DECLARE "%s" CURSOR FOR %s' % (self.name, query)
		self.closed = 0
	    elif _badQuery and self.conn.inTransaction:
		raise NotSupportedError, \
		      "DROP [TABLE|INDEX] within a transaction"
	    if not self.conn.inTransaction:
		if _badQuery:
		    pass # not in transaction so DROP TABLE/INDEX is ok.
		else:
                    self.conn._Connection__setupTransaction()

	_nl = len(self.conn.notices)

	try:
	    _qstr = self.__unicodeConvert(_qstr)
	    if len(parms) == 0:
		# If there are no paramters, just execute the query.
		self.res = self.conn.conn.query(_qstr)
            else:
                if len(parms) == 1 and \
                   (type(parms[0]) in [DictType, ListType, TupleType] or \
                    isinstance(parms[0], PgResultSet)):
                    parms = _quoteall(self.__unicodeConvert(parms[0]))
                else:
                    parms = self.__unicodeConvert(parms)
                    parms = tuple(map(_quote, self.__unicodeConvert(parms)));
		self.res = self.conn.conn.query(_qstr % parms)
	    self._rows_ = self.res.ntuples
	    self._idx_  = 0
	    self.__dict__['rowcount'] = -1 # New query - no fetch occured yet.
	    if type(self.res) is not PgResultType:
		self.__dict__['rowcount'] = -1
	    else:
		self.__dict__['oidValue'] =  self.res.oidValue
		if self.res.resultType == RESULT_DQL:
		    pass
		elif self.res.resultType == RESULT_DML:
		    self.__dict__['rowcount'] = self.res.cmdTuples
		else:
		    self.__dict__['rowcount'] = -1
	except OperationalError, msg:
	    # Uh-oh.  A fatal error occurred.  This means the current trans-
	    # action has been aborted.  Try to recover to a sane state.
	    if self.conn.inTransaction:
		_n = len(self.conn.notices)
		self.conn.conn.query('ROLLBACK WORK')
		if len(self.conn.notices) != _n:
		    raise Warning, self.conn.notices.pop()
		self.conn.__dict__["inTransaction"] = 0
		self.conn._Connection__closeCursors()
	    raise OperationalError, msg
	except InternalError, msg:
	    # An internal error occured.  Try to get to a sane state.
	    self.conn.__dict__["inTransaction"] = 0
	    self.conn._Connection__closeCursors_()
	    self.conn.close()
	    raise InternalError, msg

	if len(self.conn.notices) != _nl:
	    _drop = self.conn.notices[-1]
	    if _drop.find('transaction is aborted') > 0:
		raise Warning, self.conn.notices.pop()

	if self.res.resultType == RESULT_DQL:
	    self.__makedesc__()
	elif _qstr[:8] == 'DECLARE ':
	    # Ok -- we've created a cursor, we will pre-fetch the first row in
	    # order to make the description array.  Note: the first call to
	    # fetchXXX will return the pre-fetched record.
	    self.res = self.conn.conn.query('FETCH 1 FROM "%s"' % self.name)
	    self._rows_ = self.res.ntuples
	    self._idx_ = 0
	    self.__makedesc__()

	if len(self.conn.notices) != _nl:
	    _drop = self.conn.notices[-1]
	    if _drop.find('transaction is aborted') > 0:
		raise Warning, self.conn.notices.pop()
	    
    def executemany(self, query, parm_sequence):
	if self.closed:
	    raise InterfaceError, "executemany failed - the cursor is closed."

	if self.conn is None:
	    raise Error, "connection is closed."

	for _i in parm_sequence:
	    self.execute(query, _i)

    def fetchone(self):
	if self.closed:
	    raise InterfaceError, "fetchone failed - the cursor is closed."

	if self.conn is None:
	    raise Error, "connection is closed."

	if self.res is None:
	    raise Error, \
		  "fetchone() failed - cursor does not contain a result."
	elif self.res.resultType != RESULT_DQL:
	    if self.closed is None:
		raise Error, \
		      "fetchone() Failed - cursor does not contain any rows."

	if self._idx_ < self._rows_:
	    pass	# Still data in result buffer, use it.
	elif self.closed == 0:
	    _nl = len(self.conn.notices)
	    self.res = self.conn.conn.query('FETCH 1 FROM "%s"' % self.name)
	    self._rows_ = self.res.ntuples
	    self._idx_ = 0

	    if len(self.conn.notices) != _nl:
		_drop = self.conn.notices[-1]
		if _drop.find('transaction is aborted') > 0:
		    raise Warning, self.conn.notices.pop()

	return self.__fetchOneRow()

    def fetchmany(self, sz=None):
	if self.closed:
	    raise InterfaceError, "fetchmany failed - the cursor is closed."

	if self.conn is None:
	    raise Error, "connection is closed."

	if self.res is None:
	    raise Error, \
		  "fetchmany() failed - cursor does not contain a result."
	elif self.res.resultType != RESULT_DQL:
	    if self.close is None:
		raise Error, \
		      "fetchmany() Failed - cursor does not contain any rows."
	
	if sz is None:
	    sz = self.arraysize
	else:
	    self.__dict__["arraysize"] = abs(sz)

	if sz < 0:
	    return self.fetchall()
	    
	_list = []

	# While there are still results in the PgResult object, append them
	# to the list of results.
	while self._idx_ < self._rows_ and sz > 0:
	    _list.append(self.__fetchOneRow())
	    sz = sz - 1

	# If still need more results to fullfill the request, fetch them from
	# the PostgreSQL portal.
	if self.closed == 0 and sz > 0:
	    _nl = len(self.conn.notices)
	    self.res = self.conn.conn.query('FETCH %d FROM "%s"' %
					    (sz, self.name))
	    self._rows_ = self.res.ntuples
	    self._idx_ = 0

	    if len(self.conn.notices) != _nl:
		_drop = self.conn.notices[-1]
		if _drop.find('transaction is aborted') > 0:
		    raise Warning, self.conn.notices.pop()

	return self.__fetchManyRows(sz, _list)

    def fetchall(self):
	if self.closed:
	    raise InterfaceError, "fetchall failed - the cursor is closed."

	if self.conn is None:
	    raise Error, "connection is closed."

	if self.res is None:
	    raise Error, \
                  "fetchall() failed - cursor does not contain a result."
	elif self.res.resultType != RESULT_DQL:
	    if self.closed is None:
		raise Error, \
		      "fetchall() Failed - cursor does not contain any rows."
	    
	_list = []

	# While there are still results in the PgResult object, append them
	# to the list of results.
	while self._idx_ < self._rows_:
	    _list.append(self.__fetchOneRow())

	# Fetch the remaining results from the PostgreSQL portal.
	if self.closed == 0:
	    _nl = len(self.conn.notices)
	    self.res = self.conn.conn.query('FETCH ALL FROM "%s"' % self.name)
	    self._rows_ = self.res.ntuples
	    self._idx_ = 0

	    if len(self.conn.notices) != _nl:
		_drop = self.conn.notices[-1]
		if _drop.find('transaction is aborted') > 0:
		    raise Warning, self.conn.notices.pop()

	return self.__fetchManyRows(self._rows_, _list)

    def rewind(self):
	if self.closed:
	    raise InterfaceError, "rewind failed - the cursor is closed."

	if self.conn is None:
	    raise Error, "connection is closed."

	if self.res is None:
	    raise Error, "rewind() failed - cursor does not contain a result."
	elif self.res.resultType != RESULT_DQL:
	    if self.closed is None:
		raise Error, \
		          "rewind() Failed - cursor does not contain any rows."
	    
	if self.closed == 0:
	    _nl = len(self.conn.notices)
	    self.res = self.conn.conn.query('MOVE BACKWARD ALL IN "%s"' %
					    self.name)
	    self._rows_ = 0
	    if len(self.conn.notices) != _nl:
		_drop = self.conn.notices[-1]
		if _drop.find('transaction is aborted') > 0:
		    raise Warning, self.conn.notices.pop()

	self.__dict__["rowcount"] = -1
	self._idx_ = 0

    def setinputsizes(self, sizes):
	if self.closed:
	    raise InterfaceError, "setinputsize failed - the cursor is closed."

    def setoutputsize(self, size, column=None):
	if self.closed:
	    raise InterfaceError, "setoutputsize failed - the cursor is closed."
