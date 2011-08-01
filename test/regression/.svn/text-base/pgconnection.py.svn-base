#!/usr/local/bin/python
#ident "@(#) $Id: pgconnection.py,v 1.6 2002/12/01 04:59:25 ballie01 Exp $"
# vi:set sw=4 ts=8 showmode ai: 
#-----------------------------------------------------------------------+
# Name:		pgconnection.py						|
#									|
# Description:	pgconnection.py contains (white box) regression test	|
#		cases for the PgConnection object.			|
#									|
# Note:		These test cases requires that a test database named	|
#		pypgsql exists and that the person running the test has	|
#		full rights to the database.				|
#=======================================================================|
# Copyright 2001 by Billy G. Allie.					|
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
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN	|
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
# 30NOV2002 bga Updated tests for PostgreSQL 7.3.			|
# 17OCT2001 bga Added tests for lo_export().				|
# 30SEP2001 bga Fixed a problem with the PgVersion object test related	|
#		to the Python version running the test.			|
# 17SEP2001 bga Initial release by Billy G. Allie.			|
#-----------------------------------------------------------------------+
import sys
import unittest
import select
import types
import string
from pyPgSQL import libpq

version = sys.version_info
version = ((((version[0] * 100) + version[1]) * 100) + version[2])

# Get the default values for connection variables.
conndefs = {}
defs  = libpq.PQconndefaults()
for i in defs:
    conndefs[i[0]] = i[3]
del defs
del i
if conndefs['host'] == None:
    conndefs['host'] = 'localhost'
conndefs['port'] = int(conndefs['port'])

members = ["host", "port", "db", "tty", "user", "password", "backendPID",
	   "socket", "notices", "version", "status", "errorMessage", "isBusy",
	   "isnonblocking"]

memtypes =[types.StringType, types.IntType, types.StringType, types.StringType,
	   types.StringType, types.StringType, types.IntType, types.IntType,
	   types.ListType, libpq.PgVersionType, types.IntType, types.StringType,
	   types.IntType, types.IntType]

methods = ["consumeInput", "endcopy", "finish", "getResult", "getline",
	   "getlineAsync", "lo_creat", "lo_import", "lo_export", "lo_unlink",
	   "notifies", "putline", "query", "requestCancel", "reset",
	   "sendQuery", "trace", "untrace", "connectPoll", "flush",
	   "setnonblocking"]

# Get a connection and a version string to be used globally.
cnx = libpq.PQconnectdb("dbname=pypgsql")
vstr = "%(major)d.%(minor)d.%(level)d" % cnx.version
vnbr = ((((cnx.version.major * 100) + cnx.version.minor) * 100) + \
	cnx.version.level)

class PgConnectionTestCases(unittest.TestCase):
    def setUp(self):
	global cnx, vstr, conndefs, members, memtypes, methods

	self.cnx = cnx
	self.vstr = vstr
	self.cdef = conndefs
	self.members = members
	self.memtypes = memtypes
	self.methods = methods

    def CheckMemberValues(self, cnx, expected):
	for i in range(len(self.members)):
	    exec 'v = (cnx.%s == expected[%d])' % (self.members[i], i)
	    self.assert_(v, 'PgConnection.%s is %s, it should be %s!' % \
			    (self.members[i], v, expected[i]))

    def CheckForMethods(self, cnx):
	for i in self.methods:
	    self.assert_(i in cnx.__methods__,
			 'PgConnection is missing method "%s".' % i)
	    exec 'm = cnx.%s' % i
	    self.assert_(callable(m),
			 'PgConnection method %s() is not callable.' % i)

    def CheckMemberTypes(self, cnx, expected):
	for i in range(len(self.members)):
	    exec 'v = cnx.%s' % self.members[i]
	    self.assertEquals(type(v), expected[i],
			      'PgConnection.%s is %s, it should be %s!' % \
			      (self.members[i], type(v), expected[i]))

    def CheckFinish(self):
	"""
    Test finish()
	"""
	try:
	    cnx.finish()
	    expected = [None, None, None, None, None, None, None, None, None,
			None, None, None, None, None]
	    self.CheckMemberValues(cnx, expected)
	except Exception, msg:
	    self.fail(msg)

    def CheckCopyOutAsyncProcessing(self):
	"""
    Test getlineAsync(), endcopy()
	"""
	try:
	    res = cnx.query("copy copytest to stdin")
	    self.assertEquals(res.resultStatus, libpq.COPY_OUT,
			      "Copy Out command did not return expect results.")
	    out = []
	    inp = ''
	    while inp != '\\.':
		inp = cnx.getlineAsync()
		if inp != None:
		    out.append(inp)
	    cnx.endcopy()
	    self.assertEquals(out[0], '3\tHello World\t4.5',
			      "Copy Out did not work")
	    self.assertEquals(out[1], '4\tGoodbye World\t7.11',
			      "Copy Out did not work")
	    del res
	except Exception, msg:
	    self.fail(msg)

    def CheckCopyOutProcessing(self):
	"""
    Test getline(), endcopy()
	"""
	try:
	    res = cnx.query("copy copytest to stdin")
	    self.assertEquals(res.resultStatus, libpq.COPY_OUT,
			      "Copy Out command did not return expect results.")
	    out = []
	    inp = ''
	    while inp != '\\.':
		inp = cnx.getline()
		out.append(inp)
	    cnx.endcopy()
	    self.assertEquals(out[0], '3\tHello World\t4.5',
			      "Copy Out did not work")
	    self.assertEquals(out[1], '4\tGoodbye World\t7.11',
			      "Copy Out did not work")
	    del res
	except Exception, msg:
	    self.fail(msg)

    def CheckCopyInProcessing(self):
	"""
    Test putline(), endcopy()
	"""
	try:
	    try:
		ret = cnx.query("drop table copytest")
	    except Exception, msg:
		pass
	    res = cnx.query("create table copytest (a int, b text, c float8)")
	    res = cnx.query("copy copytest from stdin")
	    self.assertEquals(res.resultStatus, libpq.COPY_IN,
			      "Copy In command did not return expect results.")
	    cnx.putline('3\tHello World\t4.5\n')
	    cnx.putline('4\tGoodbye World\t7.11\n')
	    cnx.putline('\\.\n')
	    cnx.endcopy()
	    res = cnx.query("select * from copytest")
	    self.assertEquals(res.ntuples, 2, "Copy In did not work")
	    self.assertEquals(res.getvalue(0,0), 3, "Copy In did not work")
	    self.assertEquals(res.getvalue(0,1), "Hello World",
			      "Copy In did not work")
	    self.assertEquals(res.getvalue(0,2), 4.5, "Copy In did not work")
	    self.assertEquals(res.getvalue(1,0), 4, "Copy In did not work")
	    self.assertEquals(res.getvalue(1,1), "Goodbye World",
			      "Copy In did not work")
	    self.assertEquals(res.getvalue(1,2), 7.11, "Copy In did not work")
	    del res
	except Exception, msg:
	    self.fail(msg)

    def CheckAsyncProcessing(self):
	"""
    Test sendQuery, consumeInput, and getResult.
	"""
	if self.vstr.startswith("7.3"):
	    flen = 11
	elif self.vstr.startswith("7.2"):
	    flen = 9
	elif self.vstr.startswith("7.1"):
	    flen = 7
	else:
	    flen = 4

	try:
	    cnx.sendQuery("""select * from pg_database
			     where datname='template1'""");
	    while 1:
		ready_sockets = select.select([cnx.socket], [], [], 1.0)[0]
		if len(ready_sockets) != 0:
		    cnx.consumeInput()
		    if cnx.isBusy == 0:
			break
	    res = cnx.getResult()
	    self.assertEquals(type(res), libpq.PgResultType,
			      'getResult did not return a PgResult.')
	    self.assertEquals(res.ntuples, 1,
			      'Query return wrong number of tuples.')
	    self.assertEquals(res.nfields, flen,
			      'Query return wrong number of fields.')
	    if res.getvalue(0,0) != 'template1' or \
	       res.getvalue(0,2) != 0		or \
	       res.getvalue(0,3) != 1		or \
	       res.getvalue(0,4) != 1:
		self.fail('Query returned unexpected data')
	except Exception, msg:
	    self.fail(msg)

    def CheckMethodFunctionality(self):
	"""
    Check setnonblocking(), reset()
	"""
	self.assertEquals(cnx.isnonblocking, 0,
			  "isnonblocking should be 0.  It isn't.")
	cnx.setnonblocking(1)
	self.assertEquals(cnx.isnonblocking, 1,
			  "isnonblocking should be 1.  It isn't.")
	cnx.setnonblocking(0)
	self.assertEquals(cnx.isnonblocking, 0,
			  "isnonblocking should be 0.  It isn't.")
	cnx.reset()
	expected = self.memtypes[:]
	expected[11] = types.NoneType
	self.CheckMemberTypes(cnx, expected)
	# Note: There is no way we can determine what the value of cnx.socket
	#	and cnx.backendPID are programtically.
	cdef = self.cdef
	expected = [cdef['host'], cdef['port'], 'pypgsql', cdef['tty'],
		    cdef['user'], cdef['password'], self.cnx.backendPID,
		    self.cnx.socket, [], self.vstr, 0, None, 0, 0]
	if version < 20100:
	    expected[9] = vnbr
	self.CheckMemberValues(cnx, expected)


    def CheckMethodSanity3(self):
	"""
    Check response when methods are called with erronious arguments.
	"""
	self.CheckForMethods(self.cnx)
	self.failUnlessRaises(ValueError, self.cnx.lo_creat, 's')
	self.failUnlessRaises(libpq.OperationalError,
			      self.cnx.lo_import, 'NoSuchFile')
	self.failUnlessRaises(IOError, self.cnx.lo_unlink, 0)
	# putline can not have an erronious argument if the type is correct
	self.failUnlessRaises(TypeError, self.cnx.putline, 'a\x00b')
	self.failUnlessRaises(TypeError, self.cnx.query, 0)
	self.failUnlessRaises(TypeError, self.cnx.sendQuery, 0)
	# trace can not have an erronious argument if the type is correct
	# setnonblocking can not have an erronious argument if the type is
	# correct.

    def CheckMethodSanity2(self):
	"""
    Check response when methods are called with the wrong type of argument[s]
	"""
	self.CheckForMethods(self.cnx)
	self.failUnlessRaises(TypeError, self.cnx.lo_creat, [])
	self.failUnlessRaises(TypeError, self.cnx.lo_import, [])
	self.failUnlessRaises(TypeError, self.cnx.lo_export, [], ())
	self.failUnlessRaises(TypeError, self.cnx.lo_unlink, [])
	self.failUnlessRaises(TypeError, self.cnx.putline, [])
	self.failUnlessRaises(TypeError, self.cnx.query, [])
	self.failUnlessRaises(TypeError, self.cnx.sendQuery, [])
	self.failUnlessRaises(TypeError, self.cnx.trace, [])
	self.failUnlessRaises(TypeError, self.cnx.setnonblocking, [])

    def CheckMethodSanity1(self):
	"""
    Check response when methods are called with the wrong number of arguments
	"""
	self.CheckForMethods(self.cnx)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.consumeInput, 1)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.endcopy, 1)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.finish, 1)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.getResult, 1)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.getline, 1)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.getlineAsync, 1)
	self.failUnlessRaises(TypeError, self.cnx.lo_creat)
	self.failUnlessRaises(TypeError, self.cnx.lo_creat, 1, 2)
	self.failUnlessRaises(TypeError, self.cnx.lo_import)
	self.failUnlessRaises(TypeError, self.cnx.lo_import, 1, 2)
	self.failUnlessRaises(TypeError, self.cnx.lo_export)
	self.failUnlessRaises(TypeError, self.cnx.lo_export, 1)
	self.failUnlessRaises(TypeError, self.cnx.lo_unlink)
	self.failUnlessRaises(TypeError, self.cnx.lo_unlink, 1, 2)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.notifies, 1)
	self.failUnlessRaises(TypeError, self.cnx.putline)
	self.failUnlessRaises(TypeError, self.cnx.putline, 1, 2)
	self.failUnlessRaises(TypeError, self.cnx.query)
	self.failUnlessRaises(TypeError, self.cnx.query, 1, 2)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.requestCancel, 1)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.reset, 1)
	self.failUnlessRaises(TypeError, self.cnx.sendQuery)
	self.failUnlessRaises(TypeError, self.cnx.sendQuery, 1, 2)
	self.failUnlessRaises(TypeError, self.cnx.trace)
	self.failUnlessRaises(TypeError, self.cnx.trace, 1, 2)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.untrace, 1)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.connectPoll, 1)
	self.failUnlessRaises(libpq.InterfaceError, self.cnx.flush, 1)
	self.failUnlessRaises(TypeError, self.cnx.setnonblocking)
	self.failUnlessRaises(TypeError, self.cnx.setnonblocking, 1, 2)
	# Check for existance of removed method, lo_get()
	try:
	    tattr = self.cnx.lo_get
	    self.fail("PgConnection method 'lo_get' exists!")
	except AttributeError, msg:
	    pass


    def CheckMemberSanity(self):
	"""
    Check the object for valid member types and values.
	"""
	self.CheckForMethods(self.cnx)
	expected = self.memtypes[:]
	expected[11] = types.NoneType
	self.CheckMemberTypes(cnx, expected)
	# Note: There is no way we can determine what the value of cnx.socket
	#	and cnx.backendPID are programtically.  Fill in the expected
	#	values from the connection.
	cdef = self.cdef
	expected = [cdef['host'], cdef['port'], 'pypgsql', cdef['tty'],
		    cdef['user'], cdef['password'], self.cnx.backendPID,
		    self.cnx.socket, [], self.vstr, 0, None, 0, 0]
	if version < 20100:
	    expected[9] = vnbr
	self.CheckMemberValues(cnx, expected)
	# Test the hidden attribute, toggleShowQuery.
	self.assertEquals(cnx.toggleShowQuery, 'On',
			  'Turning on ShowQuery failed.')
	self.assertEquals(cnx.toggleShowQuery, 'Off',
			  'Turning off ShowQuery failed.')
			  
if __name__ == "__main__":
    TestSuite = unittest.TestSuite()

    TestSuite.addTest(PgConnectionTestCases("CheckMemberSanity"))
    TestSuite.addTest(PgConnectionTestCases("CheckMethodSanity1"))
    TestSuite.addTest(PgConnectionTestCases("CheckMethodSanity2"))
    TestSuite.addTest(PgConnectionTestCases("CheckMethodSanity3"))
    TestSuite.addTest(PgConnectionTestCases("CheckMethodFunctionality"))
    TestSuite.addTest(PgConnectionTestCases("CheckAsyncProcessing"))
    TestSuite.addTest(PgConnectionTestCases("CheckCopyInProcessing"))
    TestSuite.addTest(PgConnectionTestCases("CheckCopyOutProcessing"))
    TestSuite.addTest(PgConnectionTestCases("CheckCopyOutAsyncProcessing"))
    TestSuite.addTest(PgConnectionTestCases("CheckFinish"))

    runner = unittest.TextTestRunner()
    runner.run(TestSuite)
