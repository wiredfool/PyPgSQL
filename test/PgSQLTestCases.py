#!/usr/bin/env python
#ident "@(#) $Id: PgSQLTestCases.py,v 1.29 2006/06/02 14:27:03 ghaering Exp $"
#-----------------------------------------------------------------------+
# Name:			PgSQLTestCases.py										|
#																		|
# Synopsys:																|
#																		|
# Description:	PgSQLTestCases contains the functional test cases for	|
#				the PgSQL Python -> PostgreSQL DB-API 2.0 compliant in- |
#				terface module.											|
#=======================================================================|
# Copyright 2000 by Billy G. Allie.										|
# All rights reserved.													|
#																		|
# Permission to use, copy, modify, and distribute this software and its |
# documentation for any purpose and without fee is hereby granted, pro- |
# vided that the above copyright notice appear in all copies and that	|
# both that copyright notice and this permission notice appear in sup-	|
# porting documentation, and that the copyright owner's name not be		|
# used in advertising or publicity pertaining to distribution of the	|
# software without specific, written prior permission.					|
#																		|
# THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,	|
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.	IN	|
# NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR	|
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS	|
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE |
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE	|
# USE OR PERFORMANCE OF THIS SOFTWARE.									|
#=======================================================================|
# Revision History:														|
#																		|
# Date		Ini Description												|
# --------- --- ------------------------------------------------------- |
# 15AUG2011 eds Added tests for Connection based quoting				|
# 02JUN2006 gh	Adjusted test suite to patch #882032.					|
# 01JUN2006 gh	Slimmed down tests to make them work with recent Python |
#				and PostgreSQL releases.								|
# 06MAR2005 bga Updated tests for PostgresQL 8.0						|
# 06OCT2004 bga [Bug #816729] Updated tests for PostgreSQL 7.4(beta).	|
#				Thanks to Jeff Putnam for the fixes.					|
# 30NOV2002 bga Updated tests for PostgreSQL 7.3.						|
# 10NOV2002 bga Added an additional PgNumeric class check.				|
# 27OCT2002 gh	Don't check for the obsolete displaysize field in		|
#				cursor.description. Also don't check the backend enco-	|
#				ding.													|
# 08SEP2002 gh	Added tests for PgResultSet.							|
# 11AUG2002 bga Added additional tests for the PgNumeric class.			|
# 03AUG2002 gh	Added test for bug #589370 (wether it is possible to	|
#				use OID in a query that doesn't return any rows).		|
# 29JUL2002 gh	Added a few tests for PgNumeric (which currently fail). |
#				Simplified the construction of testcases by a lot and	|
#				fixed a bug where there were two methods named			|
#				CheckConnectionObject (thus hiding the first one).		|
# 16APR2002 gh	Updated the PostgreSQL version specific tests to cope	|
#				with PostgreSQL 7.2.x.									|
# 09SEP2001 bga Modified tests to reflect changes to PgVersion object.	|
#				In particulare, a PgVersion object no longer has a		|
#				__dict__ attribute, but now acts as a mapping object.	|
#				The coerce method of the PgVersion object will always	|
#				succeed, but will return a 'special' PgVersion Object	|
#				that contains any error information if the coersion		|
#				failed.													|
# 08SEP2001 gh	Added more tests for PgVersion object.					|		
# 03SEP2001 gh	Added more checks for PgMoney methods.					|
# 29JUL2001 bga Added test to check for correct handling of non-print-	|
#				able characters in the CHAR types.						|
# 12JUN2001 bga Added more tests relating to closed Connection and		|
#				Cursor objects.	 Also added test for corrected circular |
#				references with weak references (Python 2.1+ only).		|
# 10JUN2001 bga Modified tests to reflect changes to PgSQL.py.			|
# 09JUN2001 bga Fixed test cases to reflect corrected implementation of |
#				the PgInt2 and PgInt8 pow() function.					|
# 04JUN2001 bga Fixed a reversed assertion in the CheckPgVer test.		|
# 03JUN2001 bga Modified test to account for the new C implementations	|
#				of PgInt2 and PgInt8.									|
# 27MAY2001 bga Expanded the number of tests.  Primarily, added tests	|
#				to check out the emulated numerical types, PgInt8,		|
#				PgInt2, and PgMoney.									|
# 21MAY2001 bga Changed assert's to use the unittest assert_() method.	|
#			--- Discovered and fixed some really nasty bugs.			|
# 18APR2001 bga Changed version check so that 7.1<anything> is treated	|
#				as version 7.1.											|
# 09MAR2001 bga Added version check for PostgreSQL 7.1beta.				|
#				Added tests to check the 'pyformat' paramstyle.			|
# 13DEC2000 bga Added version check for PostgreSQL 7.0.3.				|
# 09OCT2000 bga Fixed another section for v6.5.x compatability.			|
# 08OCT2000 bga Added code to check for the PostgreSQL version and mod- |
#				ify the test(s) accordingly.							|
# 01OCT2000 bga Initial release by Billy G. Allie.						|
#-----------------------------------------------------------------------+
import sys
import unittest
import types
import string
from pyPgSQL import PgSQL

from TestConnection import Defaults

version = sys.version_info
version = ((((version[0] * 100) + version[1]) * 100) + version[2])

class DBAPICompliance(unittest.TestCase):
	def CheckAPILevel(self):
		self.assertEqual(PgSQL.apilevel, '2.0',
						 'apilevel is %s, should be 2.0' % PgSQL.apilevel)

	def CheckThreadSafety(self):
		self.assertEqual(PgSQL.threadsafety, 1,
						 'threadsafety is %d, should be 1' % PgSQL.threadsafety)

	def CheckParamStyle(self):
		self.assertEqual(PgSQL.paramstyle, 'pyformat',
						 'paramstyle is "%s", should be "pyformat"' %
						 PgSQL.paramstyle)

	def CheckWarning(self):
		self.assert_(issubclass(PgSQL.Warning, StandardError),
					 'Warning is not a subclass of StandardError')

	def CheckError(self):
		self.failUnless(issubclass(PgSQL.Error, StandardError),
						'Error is not a subclass of StandardError')

	def CheckInterfaceError(self):
		self.failUnless(issubclass(PgSQL.InterfaceError, PgSQL.Error),
						'InterfaceError is not a subclass of Error')

	def CheckDatabaseError(self):
		self.failUnless(issubclass(PgSQL.DatabaseError, PgSQL.Error),
						'DatabaseError is not a subclass of Error')

	def CheckDataError(self):
		self.failUnless(issubclass(PgSQL.DataError, PgSQL.DatabaseError),
						'DataError is not a subclass of DatabaseError')

	def CheckOperationalError(self):
		self.failUnless(issubclass(PgSQL.OperationalError, PgSQL.DatabaseError),
						'OperationalError is not a subclass of DatabaseError')

	def CheckIntegrityError(self):
		self.failUnless(issubclass(PgSQL.IntegrityError, PgSQL.DatabaseError),
						'IntegrityError is not a subclass of DatabaseError')

	def CheckInternalError(self):
		self.failUnless(issubclass(PgSQL.InternalError, PgSQL.DatabaseError),
						'InternalError is not a subclass of DatabaseError')

	def CheckProgrammingError(self):
		self.failUnless(issubclass(PgSQL.ProgrammingError, PgSQL.DatabaseError),
						'ProgrammingError is not a subclass of DatabaseError')

	def CheckNotSupportedError(self):
		self.failUnless(issubclass(PgSQL.NotSupportedError,
								   PgSQL.DatabaseError),
						'NotSupportedError is not a subclass of DatabaseError')

class PgSQLTestModuleInterface(unittest.TestCase):
	# fetchReturnsList is a variable that controls what the fetch*() methods
	# returns.	If zero, fetch*() returns a list (per the DB-API 2.0 specs).
	# If non-zero, ti returns a PgResultSet (PgSQL extension to the specs).
	# The default should be 0.
	def CheckFetchReturnsList(self):
		self.assertEqual(PgSQL.fetchReturnsList, 0,
						 'fetchReturnsList is %d, should be 0' %
						 PgSQL.fetchReturnsList)

	def CheckBooleanConstructors(self):
		try:
			iTrue = PgSQL.PgBooleanFromInteger(1)
			iFalse = PgSQL.PgBooleanFromInteger(0)
			sTrue = PgSQL.PgBooleanFromString('t')
			sFalse = PgSQL.PgBooleanFromString('f')
			sFalse2 = PgSQL.PgBooleanFromString('  faLSe	')
		except StandardError, msg:
			self.assert_(0, msg)

		self.failUnless(iTrue,
						'PgBooleanFromInteger failed to create a TRUE value.')
		self.failIf(iFalse,
					'PgBooleanFromInteger failed to create a FALSE value (1).')
		self.failIf(iFalse,
					'PgBooleanFromInteger failed to create a FALSE value (2).')
		self.failUnless(sTrue,
						'PgBooleanFromString failed to create a TRUE value.')
		self.failIf(sFalse,
					'PgBooleanFromString failed to create a FALSE value.')

	def CheckPgInt8(self):
		a = PgSQL.PgInt8(12345678)
		self.failUnless(a == 12345678, 'PgInt8 comparison to Int failed.')
		self.failUnless(a == 12345678.0,
						'PgInt8 comparison to Float failed.')
		self.failUnless(a == 12345678L, 'PgInt8 comparison to Long failed.')
		self.failUnless(a != None, 'PgInt8 comparison to None failed')
		b = float(a)
		self.failUnless(type(b) == types.FloatType,
						'PgInt8 conversion to Float failed.')
		self.failUnless(b == 12345678.0,
						'PgInt8 conversion to Float failed.')
		b = int(a)
		self.failUnless(type(b) == types.IntType,
						'PgInt8 conversion to Int failed.')
		self.failUnless(b == 12345678,
						'PgInt8 conversion to Int failed.')
		b = long(a)
		self.failUnless(type(b) == types.LongType,
						'PgInt8 conversion to Long failed.')
		self.failUnless(b == 12345678L,
						'PgInt8 conversion to Long failed.')
		b = complex(a)
		self.failUnless(type(b) == types.ComplexType,
						'PgInt8 conversion to Complex failed.')
		self.failUnless(b == (12345678+0j),
						'PgInt8 conversion to Complex failed.')
		b = (((a ** 2) / a) + 2 - 10)
		self.failUnless(b == 12345670,
						'PgInt8 match failed to produce correct result.')
		# b = PgSQL.PgInt8(3037000649L)
		# self.failUnlessRaises(OverflowError, pow, b, 2)

	def CheckPgInt2(self):
		a = PgSQL.PgInt2(181)
		self.failUnless(a == 181, 'PgInt2 comparison to Int failed.')
		self.failUnless(a == 181.0, 'PgInt2 comparison to Float failed.')
		self.failUnless(a == 181L, 'PgInt2 comparison to Long failed.')
		self.failUnless(a != None, 'PgInt2 comparison to None failed')
		b = float(a)
		self.failUnless(type(b) == types.FloatType,
						'PgInt2 conversion to Float failed.')
		self.failUnless(b == 181.0, 'PgInt2 conversion to Float failed.')
		b = int(a)
		self.failUnless(type(b) == types.IntType,
						'PgInt2 conversion to Int failed.')
		self.failUnless(b == 181, 'PgInt8 conversion to Int failed.')
		b = long(a)
		self.failUnless(type(b) == types.LongType,
						'PgInt2 conversion to Long failed.')
		self.failUnless(b == 181L, 'PgInt2 conversion to Long failed.')
		b = complex(a)
		self.failUnless(type(b) == types.ComplexType,
						'PgInt2 conversion to Complex failed.')
		self.failUnless(b == (181+0j), 'PgInt2 conversion to Complex failed.')
		b = (((a ** 2) / a) + 2 - 10)
		self.failUnless(b == 173,
						'PgInt2 match failed to produce correct result.')
		# b = PgSQL.PgInt2(182)
		# self.failUnlessRaises(OverflowError, pow, b, PgSQL.PgInt2(2))

	def CheckPgMoney(self):
		a = PgSQL.PgMoney(4634.00)
		self.failUnless(a == 4634, 'PgMoney comparison to Int failed.')
		self.failUnless(a == 4634.0, 'PgMoney comparison to Float failed.')
		self.failUnless(a == 4634L, 'PgMoney comparison to Long failed.')
		self.failUnless(a != None, 'PgMoney comparison to None failed')
		a = PgSQL.PgMoney(4634.04)
		b = float(a)
		self.failUnless(type(b) == types.FloatType,
						'PgMoney conversion to Float failed.')
		self.failUnless(b == 4634.04, 'PgMoney conversion to Float failed.')
		b = int(a)
		self.failUnless(type(b) == types.IntType,
						'PgMoney conversion to Int failed.')
		self.failUnless(b == 4634, 'PgMoney conversion to Int failed.')
		b = long(a)
		self.failUnless(type(b) == types.LongType,
						'PgMoney conversion to Long failed.')
		self.failUnless(b == 4634, 'PgMoney conversion to Long failed.')
		b = complex(a)
		self.failUnless(type(b) == types.ComplexType,
						'PgMoney conversion to Complex failed.')
		self.failUnless(b == (4634.04+0j),
						'PgMoney conversion to Complex failed.')
		b = (((a ** 2) / a) + 2.0 - 10)
		self.failUnless(isinstance(b, PgSQL.PgMoney),
						'PgMoney math failed to return PgMoney.')
		self.failUnless(b == 4626.04,
						'PgMoney match failed to produce correct result.')
		b = PgSQL.PgMoney(4634.1)
		self.failUnlessRaises(OverflowError, pow, b, 2)

		b = PgSQL.PgMoney(4634.1)
		self.failUnless(b==+b,
						'PgMoney __pos__ operation failed.')

		b = PgSQL.PgMoney(4634.1)
		self.failUnless(b * (-1) == -b,
						'PgMoney __neg__ operation failed.')

	def CheckPgNumeric(self):
		try:
			a = PgSQL.PgNumeric('1234.00')
			self.failUnless(a.getPrecision() == 6,
							'PgNumeric from string failed')
			self.failUnless(a.getScale() == 2, 'PgNumeric from string failed')
			self.failUnless(a == 1234, 'PgNumeric comparison to string failed.')
			self.failUnless(a == 1234.0,
							'PgNumeric comparison to Float failed.')
			self.failUnless(a == 1234L, 'PgNumeric comparison to Long failed.')
			self.failUnless(a != None, 'PgNumeric comparison to None failed')
		except StandardError, msg:
			self.fail(msg)

		try:
			a = PgSQL.PgNumeric(2345.00)
			self.failUnless(a.getPrecision() == 5,
							'PgNumeric from float failed')
			self.failUnless(a.getScale() == 1, 'PgNumeric from float failed')
			self.failUnless(a == 2345, 'PgNumeric from float failed.')
		except StandardError, msg:
			self.fail(msg)

		try:
			a = PgSQL.PgNumeric(345600, 6, 2)
			self.failUnless(a == 3456, 'PgNumeric from int failed.')
			self.failUnless(a.getPrecision() == 6, 'PgNumeric from int failed')
			self.failUnless(a.getScale() == 2, 'PgNumeric from int failed')
		except StandardError, msg:
			self.fail(msg)

		try:
			a = PgSQL.PgNumeric(456700L, 6, 2)
			self.failUnless(a == 4567, 'PgNumeric from long failed.')
			self.failUnless(a.getPrecision() == 6, 'PgNumeric from long failed')
			self.failUnless(a.getScale() == 2, 'PgNumeric from long failed')
		except StandardError, msg:
			self.fail(msg)

		try: # Test the casting of PgNumeric -> PgNumeric to change prec/scale
			a = PgSQL.PgNumeric('123456.78')
			b = PgSQL.PgNumeric(a, 10, 4)
			c = PgSQL.PgNumeric(a, 7, 1)
			self.failUnless(b.getPrecision() == 10, 'PgNumeric cast failed')
			self.failUnless(b.getScale() == 4, 'PgNumeric cast failed')
			self.failUnless(c.getPrecision() == 7, 'PgNumeric cast failed')
			self.failUnless(c.getScale() == 1, 'PgNumeric cast failed')
			self.failUnless(a == b, 'PgNumeric cast failed')
			self.failUnless(a != c, 'PgNumeric cast failed')
			self.failUnless(str(c) == '123456.8', 'PgNumeric cast failed.')
		except StandardError, msg:
			self.fail(msg)

		try: # Test the basic math functions.
			a = PgSQL.PgNumeric('1234.567')
			b = PgSQL.PgNumeric('12.3456789')
			c = a + b
			self.failUnless(c.getPrecision() == 11, 'PgNumeric addition failed')
			self.failUnless(c.getScale() == 7, 'PgNumeric addition failed')
			self.failUnless(str(c) == '1246.9126789',
							'PgNumeric addition failed.')
			c = a - b
			self.failUnless(c.getPrecision() == 11,
							'PgNumeric subtraction failed')
			self.failUnless(c.getScale() == 7, 'PgNumeric subtraction failed')
			self.failUnless(str(c) == '1222.2213211',
							'PgNumeric subtraction failed.')
			c = a * b
			self.failUnless(c.getPrecision() == 16,
							'PgNumeric multiplication failed')
			self.failUnless(c.getScale() == 10,
							'PgNumeric mulitplication failed')
			self.failUnless(str(c) == '15241.5677625363',
							'PgNumeric multiplication failed.')
			c = a / b
			self.failUnless(c.getPrecision() == 7, 'PgNumeric division failed')
			self.failUnless(c.getScale() == 3, 'PgNumeric divisioncast failed')
			self.failUnless(str(c) == '100.000',
							'PgNumeric divisioncast failed.')
			c = PgSQL.PgNumeric(a)
			c += b
			self.failUnless(c.getPrecision() == 11, 'PgNumeric addition failed')
			self.failUnless(c.getScale() == 7, 'PgNumeric addition failed')
			self.failUnless(str(c) == '1246.9126789',
							'PgNumeric addition failed.')
			c = PgSQL.PgNumeric(a)
			c -= b
			self.failUnless(c.getPrecision() == 11,
							'PgNumeric subtraction failed')
			self.failUnless(c.getScale() == 7, 'PgNumeric subtraction failed')
			self.failUnless(str(c) == '1222.2213211',
							'PgNumeric subtraction failed.')
			c = PgSQL.PgNumeric(a)
			c *= b
			self.failUnless(c.getPrecision() == 16,
							'PgNumeric multiplication failed')
			self.failUnless(c.getScale() == 10,
							'PgNumeric mulitplication failed')
			self.failUnless(str(c) == '15241.5677625363',
							'PgNumeric multiplication failed.')
			c = PgSQL.PgNumeric(a)
			c /= b
			self.failUnless(c.getPrecision() == 7, 'PgNumeric division failed')
			self.failUnless(c.getScale() == 3, 'PgNumeric divisioncast failed')
			self.failUnless(str(c) == '100.000',
							'PgNumeric divisioncast failed.')

			# Check for correct precision after a carry in the high-order digit
			a = PgSQL.PgNumeric('999999.99')
			c = a + b
			self.failUnless(c.getPrecision() == 14, 'PgNumeric addition failed')
			self.failUnless(c.getScale() == 7, 'PgNumeric addition failed')
			self.failUnless(str(c) == '1000012.3356789',
							'PgNumeric addition failed.')

		except StandardError, msg:
			self.fail(msg)

class PgSQLTestCases(unittest.TestCase):
	def setUp(self):
		self.cnx = PgSQL.connect(database='template1', host=Defaults.host, port=Defaults.port)
		self.cur = self.cnx.cursor()

	def tearDown(self):
		try:
			self.cnx.close()
		except AttributeError:
			pass
		except PgSQL.InterfaceError:
			pass
		
	def CheckConnectionObject(self):
		self.assert_(isinstance(self.cnx, PgSQL.Connection),
					 'PgSQL.connect did not return a Connection object')
		self.assert_(self.cnx.autocommit == 0,
					 'autocommit default is not zero (0)')

	def CheckConnectionClose(self):
		self.assert_(hasattr(self.cnx, 'close') and 
					 type(self.cnx.close) == types.MethodType,
					 'close is not a method of Connection')
		self.cnx.close()
		self.failUnlessRaises(PgSQL.InterfaceError, self.cnx.close)

	def CheckConnectionCommit(self):
		self.assert_(hasattr(self.cnx, "commit") and
					 type(self.cnx.commit) == types.MethodType,
					 'commit is not a method of Connection')
		self.cnx.close()
		self.failUnlessRaises(PgSQL.InterfaceError, self.cnx.commit)

	def CheckConnectionRollback(self):
		self.assert_(hasattr(self.cnx, "rollback") and
					 type(self.cnx.rollback) == types.MethodType,
					 'rollback is not a method of Connection')
		self.cnx.close()
		self.failUnlessRaises(PgSQL.InterfaceError, self.cnx.rollback)

	def CheckConnectionCursor(self):
		self.assert_(hasattr(self.cnx, "cursor") and
					 type(self.cnx.cursor) == types.MethodType,
					 'cursor is not a method of Connection')
		self.cnx.close()
		self.failUnlessRaises(PgSQL.InterfaceError, self.cnx.cursor)

	def CheckConnectionBinary(self):
		# Binary is a method of Connection and not a function of PgSQL
		# because of the requirements of the PostgreSQL Large Objects.
		self.assert_(hasattr(self.cnx, "binary") and
					 type(self.cnx.binary) == types.MethodType,
					 'binary is not a method of Connection')
		self.cnx.close()
		self.failUnlessRaises(PgSQL.InterfaceError, self.cnx.binary, '')

	def CheckCloseConnection(self):
		self.cnx.close()
		self.failUnlessRaises(PgSQL.InterfaceError, self.cur.close)
		
	if version >= 20100:
		# The follwoing tests are only for Python 2.1 or greater.
		def CheckWeakReference1(self):
			del self.cur
			self.assertEquals(len(self.cnx.cursors.data.keys()), 0,
						 'deleting cursor did not remove for connection cursor list')

		def CheckWeakReference2(self):
			del self.cnx
			self.failUnlessRaises(PgSQL.InterfaceError, self.cur.close)
	
	def CheckCursorObject(self):
		self.assert_(isinstance(self.cur, PgSQL.Cursor),
					 'cnx.cursor() did not return a Cursor object')

	def CheckCursorArraysize(self):
		self.assert_(self.cur.arraysize == 1,
					 'cur.arraysize is %d, it should be 1' %
					 self.cur.arraysize)

	def CheckCursorDescription(self):
		self.assert_(self.cur.description == None,
					 "cur.description should be None at this point, it isn't.")

	def CheckCursorRowcount(self):
		self.assert_(self.cur.rowcount == -1,
					 'cur.rowcount is %d, should be -1' % self.cur.rowcount)

	def CheckCursorCallproc(self):
		self.assert_(hasattr(self.cur, "callproc") and
					 type(self.cur.callproc) == types.MethodType,
					 'callproc is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError,
							  self.cur.callproc, 'SELECT version()')

	def CheckCursorClose(self):
		self.assert_(hasattr(self.cur, "close") and
					 type(self.cur.close) == types.MethodType,
					 'close is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError, self.cur.close)

	def CheckCursorExecute(self):
		self.assert_(hasattr(self.cur, "execute") and
					 type(self.cur.execute) == types.MethodType,
					 'execute is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError,
							  self.cur.execute, 'SELECT version()')

	def CheckCursorExecutemany(self):
		self.assert_(hasattr(self.cur, "executemany") and
					 type(self.cur.executemany) == types.MethodType,
					 'executemany is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError,
							  self.cur.executemany, 'SELECT version()', [1,2])

	def CheckCursorFetchone(self):
		self.assert_(hasattr(self.cur, "fetchone") and
					 type(self.cur.fetchone) == types.MethodType,
					 'fetchone is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError, self.cur.fetchone)

	def CheckCursorFetchMany(self):
		self.failUnless(hasattr(self.cur, "fetchmany") and
						type(self.cur.fetchmany) == types.MethodType,
						'fetchmany is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError,
							  self.cur.fetchmany, 10)

	def CheckCursorFetchall(self):
		self.failUnless(hasattr(self.cur, "fetchall") and
						type(self.cur.fetchall) == types.MethodType,
						'fetchall is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError,
							  self.cur.fetchall)

	def CheckCursorSetoutputsize(self):
		self.failUnless(hasattr(self.cur, "setoutputsize") and
						type(self.cur.setoutputsize) == types.MethodType,
						'setoutputsize is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError,
							  self.cur.setoutputsize, 1024)

	def CheckCursorSetinputsizes(self):
		self.failUnless(hasattr(self.cur, "setinputsizes") and
						type(self.cur.setinputsizes) == types.MethodType,
						'setinputsizes is not a method of the Cursor object')
		self.cur.close()
		self.failUnlessRaises(PgSQL.InterfaceError,
							  self.cur.setinputsizes, [1, 2, 3])

	def CheckCursorNextset(self):
		# The nextset method is operational.
		if hasattr(self.cur, "nextset"):
			self.assertEqual(type(self.cur.nextset), types.MethodType,
							 'nextset is not a method of the Cursor object')
			self.cur.close()
			self.failUnlessRaises(PgSQL.InterfaceError,
								  self.cur.callproc, 'SELECT version()')

	def CheckPgVer(self):
		try:
			self.cur.callproc("version")
			v = string.split(self.cur.fetchone()[0])[1].split("-")[0]
			self.cur.close()
			vstr = "%(major)d.%(minor)d" % self.cnx.version
			if v != vstr or self.cnx.version.level != 0:
				vstr = vstr + ".%(level)d" % self.cnx.version
			self.failUnlessEqual(v, vstr,
						 'SELECT version() says %s, cnx.version says %s' %
								 (v, vstr))
		except StandardError, msg:
			self.fail(msg)
		
		try:
			version = self.cnx.version
			a, b = coerce(version, '7.3.2')
			self.failUnless(type(a) == type(b) == type(version),
						'Coercion from string to PgVersion failed.')
			a, b = coerce(version, 80205)
			self.failUnless(type(a) == type(b) == type(version),
						'Coercion from int to PgVersion failed.')
			a, b = coerce(version, 80206L)
			self.failUnless(type(a) == type(b) == type(version),
						'Coercion from long to PgVersion failed.')
			a, b = coerce(version, 80207.0)
			self.failUnless(type(a) == type(b) == type(version),
						'Coercion from float to PgVersion failed.')
			try:
				a, b = coerce(version, '7. 1.3')
				self.fail(
			'Coercion from garbage string to PgVersion *should* have failed.'
				)
			except ValueError, msg:
				pass
		except StandardError, msg:
			self.fail(msg)
	
	def CheckExecuteWithSingleton(self):
		"""Test execute() with a singleton string as the parameter."""

		self.cur.execute("""
			select 'x' || %s
			""", "y")
		res = self.cur.fetchone()

		self.assertEqual(res[0], "xy", "The query did not produce the expected result")

		# The following test checks the length of one of the cur.description's
		# sequences.  It should be 7 per the DB-API 2.0 specification.	In the
		# PgSQL extension of the cur.description, it is 8.

		self.assertEqual(len(self.cur.description[0]), 8,
						 "Length of cur.description[0] is %d, it should be 8." %
						 len(self.cur.description[0]))

		self.cur.close()

	def CheckExecuteWithTuple(self):
		"""Test execute() with a tuple as the parameter."""
		try:
			self.cur.execute("""
				select %s, %s
			""", ("a", "b"))
		except StandardError, msg:
			self.fail(msg)

		res = self.cur.fetchone()
		self.failUnlessEqual(res[0], "a", "did not get expected result for first column")
		self.failUnlessEqual(res[1], "b", "did not get expected result for second column")

		self.cur.close()

	def CheckExecuteWithDictionary(self):
		"""Test execute() with a dictionary as the parameter."""
		self.cur.execute("""
			select %(value)s
			""", {"value": 10})
		res = self.cur.fetchone()
		self.failUnlessEqual(res[0], 10, "Result does not match query parameter")

	def CheckResultObject(self):
		try:
			self.cur.execute("""
								select * from pg_database
								where datname = 'template1'""")
			self.assertEqual(self.cur.rowcount, -1,
							 "cur.rowcount is %d, it should be -1." %
							 self.cur.rowcount)
			self.res = self.cur.fetchall()
		except StandardError, msg:
			self.fail(msg)

		self.assertEqual(self.cur.rowcount, 1,
						 'cur.rowcount is %d, it should be 1.' %
						 self.cur.rowcount)
		
		self.assertEqual(type(self.res), types.ListType,
						 'cur.fetchall() did not return a sequence.')

		self.assertEqual(len(self.res), 1,
						 'Length of the list of results is %d, it should be 1' %
						 len(self.res))

		self.failUnless(isinstance(self.res[0], PgSQL.PgResultSet),
						'cur.fetchall() did not return a list of PgResultSets.')
		
		self.failUnless(self.res[0].datname == 'template1'		and
						self.res[0][0] == 'template1',
						'The query did not return the correct result.')

	def CheckResultFetchone(self):
		# The following tests do not work when the version of PostgreSQL < 7.x
		if self.cnx.version < 70000:
			try:
				self.cur.execute("""
									select * from pg_database
									where datname = 'template1'""")
				self.res = self.cur.fetchone()
				self.assertEqual(self.cur.rowcount, 1,
								 'cur.rowcount is %d, it should be 1.' %
								 self.cur.rowcount)
				self.cur.rewind()
				self.assertEqual(self.cur.rowcount, -1,
								 "cur.rowcount is %d, it should be -1" %
								 self.cur.rowcount)
				self.res = self.cur.fetchone()
			except StandardError, msg:
				self.fail(msg)

			self.assertEqual(self.cur.rowcount, 1,
							 "cur.rowcount is %d, it should be 1." %
							 self.cur.rowcount)

			self.failUnless(isinstance(self.res, PgSQL.PgResultSet),
							"cur.fetchone() does not return a PgResultSet.")
			
			self.failUnless(self.res[0] == 'template1'	and
							self.res['encoding'] == 0	and
							self.res.datname == 'template1',
							"The query did not return the correct result.")

			try:
				self.res = self.cur.fetchone()
				self.assertEqual(self.cur.rowcount, 0,
								 "cur.rowcount is %d, it should be 0" %
								 self.cur.rowcount)
			except StandardError, msg:
				self.fail(msg)

			self.assertEqual(self.res, None,
							 "res should be None at this point, but it isn't.")

	def _disabled_CheckDoMoreResultObjectChecks(self):
		# Define the row counts for various version of PosrtgreSQL
		# Note: We only have to check for the minor version number in order
		#		to determine the needed row counts.
		rc = { '8.0':140, '7.4':137, '7.3':124, '7.2':101, '7.1':80, '7.0':65, '6.5':47 }

		v = self.vstr

		try:
			self.cur.execute("""
			select * from pg_class
			where relname like 'pg_%%'""")
			self.assertEqual(self.cur.rowcount, -1,
							 "cur.rowcount is %d, it should be -1." %
							 self.cur.rowcount)
			self.res = self.cur.fetchall()
			self.assertEqual(self.cur.rowcount, rc[v],
							 "cur.rowcount is %d, it should be %d." %
							 (self.cur.rowcount, rc[v]))
		except StandardError, msg:
			self.fail(msg)

	def CheckSelectOfNonPrintableString(self):
		try:
			# UNDONE -- issues warning on backend. WARNING:	 nonstandard use of escape in a string literal at character 44
			# HINT:	 Use the escape string syntax for escapes, e.g., E'\r\n'.
			a = '\x01\x02\x03\x04'
			self.cur.execute('select %s as a', a)
			r = self.cur.fetchone()
			self.assertEqual(len(r.a), len(a),
							 "Length of result is %d, it should be %d."	 %
							 (len(r.a), len(a)))
			self.failUnless(r.a == a,
							 "Result is '%s', it should be '%s'" % (r.a, a))
		except StandardError, msg:
			self.fail(msg)

	def CheckSelectOfLong(self):
		try:
			self.cur.execute('select %s + %s as a', (5L, 6L))
			r = self.cur.fetchone()
			self.assertEqual(r.a, 11,
							 "Sum of 5 and 6 should have been 11, was %i"
							 % r.a)
		except StandardError, msg:
			self.fail(msg)

	def CheckSelectOfOidWithZeroRows(self):
		# Check for control flow of bug #589370
		try:
			self.cur.execute('select oid from pg_type where false')
		except StandardError, msg:
			self.fail(msg)

class PgResultSetTests(unittest.TestCase):
	def setUp(self):
		self.cnx = PgSQL.connect(database='template1', host=Defaults.host, port=Defaults.port )
		self.cur = self.cnx.cursor()

	def tearDown(self):
		try:
			self.cnx.close()
		except AttributeError:
			pass
		except PgSQL.InterfaceError:
			pass

	def getResult(self):
		self.cur.execute("select 5 as id, 'Alice' as name, 29 as age")
		return self.cur.fetchone()
	
	def CheckAttributeAccess(self):
		res = self.getResult()
		if not hasattr(res, "id"):
			self.fail("Resultset doesn't have attribute 'id'")
		if not hasattr(res, "ID"):
			self.fail("Resultset doesn't have attribute 'ID'")

	def CheckAttributeValue(self):
		res = self.getResult()
		if res.id != 5:
			self.fail("id should be 5, is %i" % res.id)
		if res.ID != 5:
			self.fail("ID should be 5, is %i" % res.ID)

	def CheckKeyAccess(self):
		res = self.getResult()
		if not "id" in res:
			self.fail("Resultset doesn't have item 'id'")
		if not "ID" in res:
			self.fail("Resultset doesn't have item 'ID'")

	def CheckKeyValue(self):
		res = self.getResult()
		if res["id"] != 5:
			self.fail("id should be 5, is %i" % res.id)
		if res["ID"] != 5:
			self.fail("ID should be 5, is %i" % res.ID)

	def CheckIndexValue(self):
		res = self.getResult()
		if res[0] != 5:
			self.fail("item 0 should be 5, is %i" % res.id)

	def CheckChangeIndex(self):
		res = self.getResult()
		res[0] = 6
		if res[0] != 6:
			self.fail("item 0 should be 6, is %i" % res.id)

	def CheckChangeKey(self):
		res = self.getResult()
		res["id"] = 6
		if res["id"] != 6:
			self.fail("id should be 6, is %i" % res.id)

		res = self.getResult()
		res["ID"] = 6
		if res["ID"] != 6:
			self.fail("ID should be 6, is %i" % res.id)

	def CheckChangeAttr(self):
		res = self.getResult()
		res.id = 6
		if res.id != 6:
			self.fail("id should be 6, is %i" % res.id)

		res = self.getResult()
		res.ID = 6
		if res.ID != 6:
			self.fail("ID should be 6, is %i" % res.id)

	def Check_haskey(self):
		res = self.getResult()
		if not res.has_key("id"):
			self.fail("resultset should have key 'id'")
		if not res.has_key("ID"):
			self.fail("resultset should have key 'ID'")
		if not res.has_key("Id"):
			self.fail("resultset should have key 'Id'")

	def Check_len(self):
		l = len(self.getResult())
		if l != 3:
			self.fail("length of resultset should be 3, is %i", l)

	def Check_keys(self):
		res = self.getResult()
		if res.keys() != ["id", "name", "age"]:
			self.fail("keys() should return %s, returns %s" %
						(["id", "name", "age"], res.keys()))

	def Check_values(self):
		val = self.getResult().values()
		if val != [5, 'Alice', 29]:
			self.fail("Wrong values(): %s" % val)

	def Check_items(self):
		it = self.getResult().items()
		if it != [("id", 5), ("name", 'Alice'), ("age", 29)]:
			self.fail("Wrong items(): %s" % it)

	def Check_get(self):
		res = self.getResult()
		v = res.get("id")
		if v != 5:
			self.fail("Wrong result for get [1]")

		v = res.get("ID")
		if v != 5:
			self.fail("Wrong result for get [2]")

		v = res.get("asdf")
		if v is not None:
			self.fail("Wrong result for get [3]")

		v = res.get("asdf", 6)
		if v != 6:
			self.fail("Wrong result for get [4]")

class setting(object):
	def __init__(self, cur, setting, value):
		self.cur = cur
		self.setting = setting
		self.value = value
		self.initial = self._get(setting) 
		if self.initial == value:
			return
		self._set(setting, value)

	def __enter__(self):
		return self
	
	def __exit__(self, type, value, traceback):
		if self.initial != self.value:
			self._set(self.setting, self.initial)					 
		
	def _get(self, setting):
		self.cur.execute('show %s' % setting)
		res = self.cur.fetchone()
		return res.values()[0] == 'on'
	
	def _set(self, setting, value):
		self.cur.execute('set %s to %s' % (setting, '%s'), (value and 'on' or 'off'))

class PgQuoteTests(unittest.TestCase):
	def setUp(self):
		self.cnx = PgSQL.connect(database='template1', host=Defaults.host, port=Defaults.port)
		self.cur = self.cnx.cursor()

	def tearDown(self):
		try:
			self.cnx.close()
		except AttributeError:
			pass
		except PgSQL.InterfaceError:
			pass

	def query_encoded(self, encoded):
		self.cur.execute("select %s as a" % encoded) # not reencoding
		res = self.cur.fetchone()
		if res:
			print encoded, res['a']

	def query_rt(self, s):
		self.cur.execute("select %s as a" ,s) 
		res = self.cur.fetchone()
		if res:
			print s, res['a']
			self.assertEquals(s, res['a'])

	def query_rt_bytea(self, s):
		self.cur.execute("select %s::bytea as a" ,s) 
		res = self.cur.fetchone()
		if res:
			print s, res['a']
			self.assertEquals(s, res['a'])
			
	def Check_string(self):
		
		with setting(self.cur, 'standard_conforming_strings', False):
			self.assertEquals(self.cnx.conn.PgQuoteString('string'), "E'string'")
		with setting(self.cur, 'standard_conforming_strings', True):
			self.assertEquals(self.cnx.conn.PgQuoteString('string'), "'string'")
			
		self.assertEquals(self.cnx.conn.PgQuoteString('string',True), '"string"')
		self.query_rt('strin\g')
		self.query_rt('stri\\ng')
		self.query_rt('s\"tring')
		self.query_rt('st\ring')
		self.query_rt("str\ing")

	def Check_bytea(self):
		s = "str\x00ing"
		b = PgSQL.PgBytea(s)
		print self.cnx.conn.PgQuoteBytea('string')
		if self.cnx.conn.version >= '9.1':			  
			# standard conforming strings make a difference

			with setting(self.cur, 'standard_conforming_strings', True):
				self.assertEquals(b._quote(self.cnx), "'\\x73747200696e67'")
				self.assertEquals(b._quote(self.cnx, True), '"\\x73747200696e67"')

			with setting(self.cur, 'standard_conforming_strings', False):
				self.assertEquals(b._quote(self.cnx), "E'\\\\x73747200696e67'")
				self.assertEquals(b._quote(self.cnx, True), '"\\\\x73747200696e67"')

		elif self.cnx.conn.version >= '9.0':
			self.assertEquals(b._quote(self.cnx), "E'\\\\x73747200696e67'")
			self.assertEquals(b._quote(self.cnx, True), '"\\\\x73747200696e67"')
		else:
			self.assertEquals(b._quote(self.cnx), "E'str\\\\000ing'")
			self.assertEquals(b._quote(self.cnx, True), '"str\\\\000ing"')
			
		# this also tests basic function of unquotebytea
		self.query_rt_bytea(b)

	def Check_deprecated_quotestring(self):
		self.assertEquals(PgSQL.PgQuoteString('string'), "E'string'")
		self.assertEquals(PgSQL.PgQuoteString('string',True), '"string"')

	def Check_deprecated_quotebytea(self):
		s = "st\x00ring"
		print PgSQL.PgQuoteBytea(s)
		if self.cnx.conn.version >= '9.1':
			# still wrong -- this one is never going to be right.
			self.assertEquals(PgSQL.PgQuoteBytea(s), "E'st\\000ring'")
			self.assertEquals(PgSQL.PgQuoteBytea(s, True), '"st\\000ring"')
		else:
			self.assertEquals(PgSQL.PgQuoteBytea(s), "E'st\\\\000ring'")
			self.assertEquals(PgSQL.PgQuoteBytea(s, True), '"st\\\\000ring"')

		
		

def suite():
	dbapi_tests = unittest.makeSuite(DBAPICompliance, "Check")
	moduleinterface_tests = unittest.makeSuite(PgSQLTestModuleInterface, "Check")
	pgsql_tests = unittest.makeSuite(PgSQLTestCases, "Check")
	pgresultset_tests = unittest.makeSuite(PgResultSetTests, "Check")
	pgquote_tests = unittest.makeSuite(PgQuoteTests, "Check")

	test_suite = unittest.TestSuite((dbapi_tests, moduleinterface_tests,
										pgsql_tests, pgresultset_tests, pgquote_tests))

	#test_suite = unittest.TestSuite((pgquote_tests))
	return test_suite

def main():
	runner = unittest.TextTestRunner()
	runner.run(suite())
	
if __name__ == "__main__":
	main()
