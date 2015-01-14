#!/usr/local/bin/python
#ident "@(#) $Id: pgresult.py,v 1.10 2002/12/01 04:59:25 ballie01 Exp $"
#-----------------------------------------------------------------------+
# Name:			pgresult.py												|
#																		|
# Description:	pgresult.py contains (white box) regression test cases	|
#				for the PgResult object.								|
#																		|
# Note:			These test cases requires that a test database named	|
#				pypgsql exists and that the person running the test has |
#				full rights to the database.							|
#=======================================================================|
# Copyright 2001 by Billy G. Allie.										|
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
# 30NOV2002 bga Updated tests for PostgreSQL 7.3.						|
# 24SEP2001 bga Updated test case to use new string quoting method.		|
# 21SEP2001 bga Added check of the member types in certain test cases.	|
# 10SEP2001 bga Added the test cases for methods.  This unit test file	|
#				is complete (for now :-).								|
# 09SEP2001 bga Modified tests to reflect changes to PgVersion object.	|
#				In particulare, a PgVersion object no longer has a		|
#				__dict__ attribute, but now acts as a mapping object.	|
# 06SEP2001 bga Completed the PgResult Member test cases, just have to	|
#				do the PgResult Method test cases.						|
# 04SEP2001 bga Expanded the test cases.  Still more to do.				|
# 02SEP2001 bga Initial release by Billy G. Allie.						|
#-----------------------------------------------------------------------+
import sys
import unittest
import types
import string
from pyPgSQL import libpq
from TestConnection import Defaults

version = sys.version_info
version = ((((version[0] * 100) + version[1]) * 100) + version[2])

# Get a connection and a version string to be used globally.
cnx = libpq.PQconnectdb("dbname=pypgsql host=%s port=%s"% (Defaults.host, Defaults.port))
vstr = "%(major)d.%(minor)d" % cnx.version

class PgResultMemberTestCases(unittest.TestCase):
	def setUp(self):
		global cnx
		global vstr

		self.cnx = cnx
		self.vstr = vstr
		self.members = ['binaryTuples', 'cmdStatus', 'cmdTuples', 'nfields',
						'ntuples', 'oidValue', 'resultStatus', 'resultType']
		self.methods = ['clear', 'fname', 'fnumber', 'fsize', 'ftype',
						'getisnull', 'getlength', 'getvalue']

	def CheckForMembers(self, res):
		for i in self.members:
			self.assert_(i in res.__members__,
						 'PgResult is missing member "%s".' % i)

	def CheckMemberValues(self, res, expected):
		for i in range(len(self.members)):
			exec 'v = res.%s' % self.members[i]
			self.assertEquals(v, expected[i],
							  'PgResult.%s is %s, it should be %s!' % \
							  (self.members[i], v, expected[i]))

	def CheckForMethods(self, res):
		for i in self.methods:
			self.assert_(i in res.__methods__,
						 'PgResult is missing method "%s".' % i)
			exec 'm = res.%s' % i
			self.assert_(callable(m),
						 'PgResult method %s() is not callable.' % i)

	def CheckDropPgResult(self):
		query = "DROP TABLE pgresult_test"
		res = self.cnx.query(query)
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'DROP', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'DROP TABLE'
		self.CheckMemberValues(res, expected)
		# Try to drop the table again.	This should raise an execption.
		self.failUnlessRaises(libpq.OperationalError, self.cnx.query, query)

	def CheckEndPgResult(self):
		res = self.cnx.query("END WORK")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'COMMIT', None, 0, 0, None, 1, 2 ]
		self.CheckMemberValues(res, expected)

	def CheckCloseBinaryPgResult(self):
		res = self.cnx.query("CLOSE pgres_bcursor")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'CLOSE', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'CLOSE CURSOR'
		self.CheckMemberValues(res, expected)

	def CheckNoticePgResult(self):
		# The following query will cause a notice to be generated, but not
		# an error.
		try:
			res = self.cnx.query("FETCH FROM pgres_cursor")
		except libpq.OperationalError:
			# sometime between 7.3 and 9.0 this got upgraded to an error.
			if self.cnx.version >= '9.0':
				#we're good.
				return
			
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'FETCH', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'FETCH 0'
		self.CheckMemberValues(res,	 expected)
		self.assertEquals(len(self.cnx.notices), 1,
						  "A notice should of been generated, none was!")
		n = self.cnx.notices.pop()
		e = 'NOTICE:  PerformPortalFetch: portal "pgres_cursor" not found\n'
		if self.cnx.version >= '7.3':
			e = 'WARNING:  PerformPortalFetch: portal "pgres_cursor" not found\n'
		self.assertEquals(n, e, "The notice did not match expected results");
		
	def CheckClosePgResult(self):
		res = self.cnx.query("CLOSE pgres_cursor")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'CLOSE', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'CLOSE CURSOR'
		self.CheckMemberValues(res, expected)

	def CheckFetchBinaryPgResult(self):
		res = self.cnx.query("FETCH 3 FROM pgres_bcursor")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [1, 'FETCH', None, 2, 3, None, 2, 1 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'FETCH 3'
		self.CheckMemberValues(res, expected)

	def CheckDeclareBinaryPgResult(self):
		res = self.cnx.query("""DECLARE pgres_bcursor BINARY CURSOR FOR
								SELECT * FROM pgresult_test""")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'SELECT', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'DECLARE CURSOR'
		self.CheckMemberValues(res, expected)

	def CheckMovePgResult(self):
		res = self.cnx.query("MOVE BACKWARD ALL IN pgres_cursor")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'MOVE', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'MOVE 1'
		self.CheckMemberValues(res, expected)
		res = self.cnx.query("FETCH ALL FROM pgres_cursor")
		expected = [0, 'FETCH', None, 2, 4, None, 2, 1 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'FETCH 4'
		self.CheckMemberValues(res, expected)

	def CheckFetchPgResult(self):
		res = self.cnx.query("FETCH 2 FROM pgres_cursor")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'FETCH', None, 2, 2, None, 2, 1 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'FETCH 2'
		self.CheckMemberValues(res, expected)

	def CheckDeclarePgResult(self):
		res = self.cnx.query("""DECLARE pgres_cursor CURSOR FOR
								SELECT * FROM pgresult_test""")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'SELECT', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'DECLARE CURSOR'
		self.CheckMemberValues(res, expected)

	def CheckBeginPgResult(self):
		res = self.cnx.query("BEGIN WORK")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'BEGIN', None, 0, 0, None, 1, 2 ]
		self.CheckMemberValues(res, expected)

	def CheckSelectPgResult(self):
		res = self.cnx.query("SELECT * FROM	 pgresult_test")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'SELECT', None, 2, 4, None, 2, 1 ]
		self.CheckMemberValues(res, expected)

	def CheckUpdatePgResult(self):
		res = self.cnx.query("""UPDATE pgresult_test
								SET f2 = 'This is a test (3u)'
								WHERE f1 = 3""")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		cs = 'UPDATE %s' % res.cmdTuples
		expected = [0, cs, 1, 0, 0, None, 1, 3 ]
		self.CheckMemberValues(res, expected)

	def CheckInsertPgResult(self):
		res = self.cnx.query("""INSERT INTO pgresult_test
								VALUES (1, 'This is a test (1)')""")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		# The PgResult object's members can't be tested with CheckMemberValues()
		# directly.	 Set up things so we can call CheckMember Values()
		cs = 'INSERT %s %s' % (0, res.cmdTuples)
		expected = [0, cs, 1, 0, 0, None, 1, 3 ]
		if self.cnx.version <= '9.0':
			#oids are deprecated.
			self.assert_(res.oidValue != None,
						 "PgResult.oidValue is None, it shouldn't be None.")
		self.CheckMemberValues(res, expected)
		res = self.cnx.query("""INSERT INTO pgresult_test
								VALUES (2, 'This is a test (2)')""")
		res = self.cnx.query("""INSERT INTO pgresult_test
								VALUES (3, 'This is a test (3)')""")
		res = self.cnx.query("""INSERT INTO pgresult_test
								VALUES (4, 'This is a test (4)')""")

	def CheckCreatePgResult(self):
		query = "CREATE TABLE pgresult_test(f1 INT, f2 TEXT)"
		res = self.cnx.query(query)
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'CREATE', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'CREATE TABLE'
		self.CheckMemberValues(res, expected)
		# Try to create the table again.  This should raise an execption.
		self.failUnlessRaises(libpq.OperationalError, self.cnx.query, query)

	def CheckPgResult(self):
		if self.vstr.startswith("7.3"):
			flen = 11
		elif self.vstr.startswith("7.2"):
			flen = 9
		elif self.vstr.startswith("7.1"):
			flen = 7
		else:
			flen = 4

		res = self.cnx.query("SELECT * FROM pg_database LIMIT 3")
		self.CheckForMembers(res)
		self.CheckForMethods(res)
		expected = [0, 'SELECT', None, flen, 3, None, 2, 1 ]
		self.CheckMemberValues(res, expected)

class PgResultTestCases(unittest.TestCase):
	def setUp(self):
		global cnx
		global vstr

		self.cnx = cnx
		self.vstr = vstr
		self.members = ['binaryTuples', 'cmdStatus', 'cmdTuples', 'nfields',
						'ntuples', 'oidValue', 'resultStatus', 'resultType']
		self.memtypes= [types.IntType, types.StringType, types.IntType,
						types.IntType, types.IntType, types.IntType,
						types.IntType, types.IntType]

	def CheckMemberValues(self, res, expected):
		for i in range(len(self.members)):
			exec 'v = res.%s' % self.members[i]
			self.assertEquals(v, expected[i],
							  'PgResult.%s is %s, it should be %s!' % \
							  (self.members[i], v, expected[i]))

	def CheckMemberTypes(self, res, expected):
		for i in range(len(self.members)):
			exec 'v = res.%s' % self.members[i]
			self.assertEquals(type(v), expected[i],
							  'PgResult.%s is %s, it should be %s!' % \
							  (self.members[i], type(v), expected[i]))

	def CheckDropPgResult(self):
		res = self.cnx.query("DROP TABLE pgresult_test")
		expected = [0, 'DROP', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'DROP TABLE'
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)

	def CheckEndPgResult(self):
		res = self.cnx.query("END WORK")
		expected = [0, 'COMMIT', None, 0, 0, None, 1, 2 ]
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)

	def CheckClosePgResult(self):
		res = self.cnx.query("CLOSE pgres_cursor")
		expected = [0, 'CLOSE', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'CLOSE CURSOR'
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)

	def CheckMovePgResult(self):
		res = self.cnx.query("MOVE BACKWARD ALL IN pgres_cursor")
		expected = [0, 'MOVE', None, 0, 0, None, 1, 2 ]
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)

	def CheckFetchPgResult(self):
		res = self.cnx.query("FETCH 2 FROM pgres_cursor")
		expected = [0, 'FETCH', None, 2, 2, None, 2, 1 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'FETCH 2'
		self.CheckMemberValues(res, expected)
		self.assertEquals(res.fmod(0), -1,
						  "fmod(0) was %d, it should be -1." % res.fmod(0))
		self.assertEquals(res.fname(0), 'f1',
						  "fname(0) was '%s', it should be 'f1'." %
						  res.fname(0))
		self.assertEquals(res.fnumber('f2'), 1,
						  "fnumber('f2') was %d, it should be 1." %
						  res.fnumber('f2'))
		self.assertEquals(res.fsize(0), 4,
						  "fsize(0) is %d, it should be 4." % res.fsize(0))
		self.assertEquals(res.ftype(1), 25,
						  "ftype(1) is %d, it should be 25." % res.ftype(1))
		self.assertEquals(res.getisnull(0, 1), 0,
						  "getisnull(0, 1) returned %d, it should be 0." %
						  res.getisnull(0, 1))
		self.assertEquals(res.getlength(1, 1), 18,
						  "getlength(1, 1) is %d, it should be 19." %
						  res.getlength(1, 1))
		value = res.getvalue(1, 1)
		self.assertEquals(value, 'This is a test (2)',
						  "getvalue(1, 1) returned incorrect results.")
		self.assertEquals(res.getlength(1, 1), len(value),
						  "getlength(1, 1) does not match len(getvalue(1, 1)).")

	def CheckDeclarePgResult(self):
		res = self.cnx.query("""DECLARE pgres_cursor CURSOR FOR
								SELECT * FROM pgresult_test
								ORDER BY f1""")
		expected = [0, 'SELECT', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'DECLARE CURSOR'
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)

	def CheckBeginPgResult(self):
		res = self.cnx.query("BEGIN WORK")
		expected = [0, 'BEGIN', None, 0, 0, None, 1, 2 ]
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)

	def CheckSelectPgResult(self):
		res = self.cnx.query("SELECT * FROM pgresult_test ORDER BY f1")
		expected = [0, 'SELECT', None, 2, 4, None, 2, 1 ]
		self.CheckMemberValues(res, expected)
		self.assertEquals(res.fmod(0), -1,
						  "fmod(0) was %d, it should be -1." % res.fmod(0))
		self.assertEquals(res.fname(0), 'f1',
						  "fname(0) was '%s', it should be 'f1'." %
						  res.fname(0))
		self.assertEquals(res.fnumber('f2'), 1,
						  "fnumber('f2') was %d, it should be 1." %
						  res.fnumber('f2'))
		self.assertEquals(res.fsize(0), 4,
						  "fsize(0) is %d, it should be 4." % res.fsize(0))
		self.assertEquals(res.ftype(1), 25,
						  "ftype(1) is %d, it should be 25." % res.ftype(1))
		self.assertEquals(res.getisnull(0, 1), 0,
						  "getisnull(0, 1) returned %d, it should be 0." %
						  res.getisnull(0, 1))
		self.assertEquals(res.getisnull(3, 1), 1,
						  "getisnull(3, 1) returned %d, it should be 1." %
						  res.getisnull(0, 1))
		self.assertEquals(res.getlength(2, 1), 19,
						  "getlength(2, 1) is %d, it should be 19." %
						  res.getlength(2, 1))
		value = res.getvalue(2, 1)
		self.assertEquals(value, 'This is a test (3u)',
						  "getvalue(2, 1) returned incorrect results.")
		self.assertEquals(res.getlength(2, 1), len(value),
						  "getlength(2, 1) does not match len(getvalue(2, 1)).")

	def CheckUpdatePgResult(self):
		res = self.cnx.query("""UPDATE pgresult_test
								SET f2 = 'This is a test (3u)'
								WHERE f1 = 3""")
		cs = 'UPDATE %s' % res.cmdTuples
		expected = [0, cs, 1, 0, 0, None, 1, 3 ]
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)

	def CheckInsertPgResult(self):
		res = self.cnx.query("""INSERT INTO pgresult_test
								VALUES (1, 'This is a test (1)')""")
		expected = [types.IntType, types.StringType, types.IntType,
					types.IntType, types.IntType, types.IntType, types.IntType,
					types.IntType ]
		self.CheckMemberTypes(res, expected)
		# The PgResult object's members can't be tested with CheckMemberValues()
		# directly.	 Set up things so we can call CheckMember Values()
		cs = 'INSERT %s %s' % (res.oidValue, res.cmdTuples)
		expected = [0, cs, 1, 0, 0, res.oidValue, 1, 3 ]
		self.assert_(res.oidValue != None,
					 "PgResult.oidValue is None, it shouldn't be None.")
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)
		res = self.cnx.query("""INSERT INTO pgresult_test
								VALUES (2, 'This is a test (2)')""")
		res = self.cnx.query("""INSERT INTO pgresult_test
								VALUES (3, 'This is a test (3)')""")
		res = self.cnx.query("""INSERT INTO pgresult_test
								VALUES (4, NULL)""")

	def CheckCreatePgResult(self):
		query = "CREATE TABLE pgresult_test(f1 INT, f2 TEXT)"
		res = self.cnx.query(query)
		expected = [0, 'CREATE', None, 0, 0, None, 1, 2 ]
		if self.cnx.version >= '7.3':
			expected[1] = 'CREATE TABLE'
		self.CheckMemberValues(res, expected)
		self.failUnlessRaises(libpq.InterfaceError, res.fmod, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fname, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(libpq.InterfaceError, res.fsize, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.ftype, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getisnull, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getlength, 0, 0)
		self.failUnlessRaises(libpq.InterfaceError, res.getvalue, 0, 0)

	def CheckPgResult(self):
		if self.vstr.startswith("7.3"):
			flen = 11
		elif self.vstr.startswith("7.2"):
			flen = 9
		elif self.vstr.startswith("7.1"):
			flen = 7
		else:
			flen = 4
		
		res = self.cnx.query("SELECT * FROM pg_database LIMIT 3")
		expected = [types.IntType, types.StringType, types.NoneType,
					types.IntType, types.IntType, types.NoneType,
					types.IntType, types.IntType ]
		self.CheckMemberTypes(res, expected)
		expected = [0, 'SELECT', None, flen, 3, None, 2, 1 ]
		self.CheckMemberValues(res, expected)
		# Check that fname returns the expected results
		cols = ['datname', 'datdba', 'encoding', 'datistemplate',
				'datallowconn', 'datlastsysoid']
		for i in range(len(cols)):
			self.assertEquals(res.fname(i), cols[i],
							  "Name for field %d is %s, it should be %s." %
							  (i, res.fname(i), cols[i]))
			self.assertEquals(res.fnumber(cols[i]), i,
						  "Number for field named %s is %d, it should be %d." %
							  (cols[i], res.fnumber(cols[i]), i))
		self.failUnlessRaises(ValueError, res.fmod, -1)
		self.failUnlessRaises(ValueError, res.fmod, res.nfields)
		self.failUnlessRaises(ValueError, res.fname, -1)
		self.failUnlessRaises(ValueError, res.fname, res.nfields)
		self.failUnlessRaises(ValueError, res.fnumber, "NoSuchName")
		self.failUnlessRaises(ValueError, res.fsize, -1)
		self.failUnlessRaises(ValueError, res.fsize, res.nfields)
		self.failUnlessRaises(ValueError, res.ftype, -1)
		self.failUnlessRaises(ValueError, res.ftype, res.nfields)
		self.failUnlessRaises(ValueError, res.getisnull, -1, 0)
		self.failUnlessRaises(ValueError, res.getisnull, res.ntuples, 0)
		self.failUnlessRaises(ValueError, res.getisnull, 0, -1)
		self.failUnlessRaises(ValueError, res.getisnull, 0, res.nfields)
		self.failUnlessRaises(ValueError, res.getlength, -1, 0)
		self.failUnlessRaises(ValueError, res.getlength, res.ntuples, 0)
		self.failUnlessRaises(ValueError, res.getlength, 0, -1)
		self.failUnlessRaises(ValueError, res.getlength, 0, res.nfields)
		self.failUnlessRaises(ValueError, res.getvalue, -1, 0)
		self.failUnlessRaises(ValueError, res.getvalue, res.ntuples, 0)
		self.failUnlessRaises(ValueError, res.getvalue, 0, -1)
		self.failUnlessRaises(ValueError, res.getvalue, 0, res.nfields)
		try:
			tattr = res.errorMessage
			self.fail('PgResult attribute "errorMessage" exists!')
		except AttributeError, msg:
			pass

	def CheckSelectOfNonPrintableString(self):
		try:
			a = '\x01\x02\x03\004'
			res = self.cnx.query("select %s as a" % self.cnx.PgQuoteString(a))
			b = res.getvalue(0, res.fnumber('a'))
			del res
			self.assertEqual(len(b), len(a),
							 "Length of result is %d, it should be %d."	 %
							 (len(b), len(a)))
			self.failUnless(b == a,
							 "Result is '%s', it should be '%s'" % (b, a))
		except StandardError, msg:
			self.fail(msg)

if __name__ == "__main__":
	TestSuite = unittest.TestSuite()
	TestSuite.addTest(PgResultMemberTestCases("CheckPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckCreatePgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckInsertPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckUpdatePgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckSelectPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckBeginPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckDeclarePgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckFetchPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckMovePgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckDeclareBinaryPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckFetchBinaryPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckClosePgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckCloseBinaryPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckNoticePgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckEndPgResult"))
	TestSuite.addTest(PgResultMemberTestCases("CheckDropPgResult"))

	TestSuite.addTest(PgResultTestCases("CheckSelectOfNonPrintableString"))
	TestSuite.addTest(PgResultTestCases("CheckPgResult"))
	TestSuite.addTest(PgResultTestCases("CheckCreatePgResult"))
	TestSuite.addTest(PgResultTestCases("CheckInsertPgResult"))
	TestSuite.addTest(PgResultTestCases("CheckUpdatePgResult"))
	TestSuite.addTest(PgResultTestCases("CheckSelectPgResult"))
	TestSuite.addTest(PgResultTestCases("CheckBeginPgResult"))
	TestSuite.addTest(PgResultTestCases("CheckDeclarePgResult"))
	TestSuite.addTest(PgResultTestCases("CheckFetchPgResult"))
	TestSuite.addTest(PgResultTestCases("CheckClosePgResult"))
	TestSuite.addTest(PgResultTestCases("CheckEndPgResult"))
	TestSuite.addTest(PgResultTestCases("CheckDropPgResult"))

	runner = unittest.TextTestRunner()
	runner.run(TestSuite)
