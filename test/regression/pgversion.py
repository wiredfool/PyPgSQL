#!/usr/local/bin/python
#ident "@(#) $Id: pgversion.py,v 1.6 2002/12/01 04:59:25 ballie01 Exp $"
#-----------------------------------------------------------------------+
# Name:		pgversion.py						|
#									|
# Description:	pgversion.py contains (white box) regression test cases	|
#		for the PgVersion object.				|
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
# 30NOV2002 bga Added tests for alpha, beta, and relase canidate vers.	|
#	    ---	Updated tests for PostgreSQL 7.3.			|
# 12JAN2001 bga	Added test to check for correct operation of the fix	|
#		for bug #486151.					|
# 21SEP2001 bga Added check of the member types in the sanity check.	|
# 16SEP2001 bga Change tests to reflect changes in pgversion.c.		|
# 13SEP2001 bga Completed the test cases for PgVersion.			|
#	    --- The coercion rules in Python 2.0 do not allow compari-	|
#		sons of PgVersion object to String Object to work.  The	|
#		test cases will coerce strings to PgVersion objects be-	|
#		for comparing them to a PgVersion object.		|
# 10SEP2001 bga Initial release by Billy G. Allie.			|
#-----------------------------------------------------------------------+
import sys
import unittest
import types
import string
from pyPgSQL import libpq

version = sys.version_info
version = ((((version[0] * 100) + version[1]) * 100) + version[2])

# Get a connection and a version string to be used globally.
cnx = libpq.PQconnectdb("dbname=pypgsql")
vers = cnx.version
dbvstr = cnx.query('select version()').getvalue(0, 0)
vstr = dbvstr.split()[1]
cnx.finish()
del cnx

class PgVersionTestCases(unittest.TestCase):
    def setUp(self):
	global vers, vstr, dbvstr

	self.vers = vers
	self.vstr = vstr
	self.dbvstr = dbvstr
	self.members = ['version', 'major', 'minor', 'level', 'post70']

    def CheckForMembers(self, res):
	for i in self.members:
	    self.assert_(i in res.__members__,
			 'PgVersion is missing member "%s".' % i)

    def CheckMemberValues(self, res, expected):
	for i in range(len(self.members)):
	    exec 'v = res.%s' % self.members[i]
	    self.assertEquals(v, expected[i],
			      'PgVersion.%s is %s, it should be %s!' % \
			      (self.members[i], v, expected[i]))

    def CheckPgVersionComparison(self):
	global version

	# Build up the value to compare against the PgVersion Object
	value = (((vers.major * 100) + vers.minor) * 100) + vers.level
	self.assert_(vers == value, "Numeric equality check failed.")
	self.assert_(vers < (value + 1), "Numeric less than check failed.")
	self.assert_(vers > (value - 1), "Numeric greater than check failed.")
	if version < 20100:
	    # Python Version 2.0 coercion rules prevents comparison of a
	    # PgVersion object to a String object from working :-(
	    a, b = coerce(vers, self.vstr)
	    self.assert_(vers == b, "String equality check failed.")
	    a, b = coerce(vers, '99.99.99')
	    self.assert_(vers < b, "String less than check failed.")
	    a, b = coerce(vers, '1.0.0')
	    self.assert_(vers > b, "String greater than check failed.")
	else:
	    self.assert_(vers == self.vstr, "String equality check failed.")
	    self.assert_(vers < '99.99.99', "String less than check failed.")
	    self.assert_(vers > '1.0.0', "String greater than check failed.")

	# Now test that exceptions are raises on coversion errors.
	try:
	    if version < 20100:
		a, b = coerce(vers, '')
		vers == b
	    else:
		vers == ''
	    self.fail('Comparison to badly formatted string succeeded.')
	except ValueError, msg:
	    pass

	try:
	    if version < 20100:
		a, b = coerce(vers, '7.1. 2')
		vers == b
	    else:
		vers == '7.1. 2'
	    self.fail('Comparison to badly formatted string succeeded.')
	except ValueError, msg:
	    pass

	try:
	    if version < 20100:
		a, b = coerce(vers, '7.1Test')
		vers == b
	    else:
		vers == '7.1Test'
	    self.fail('Comparison to badly formatted string succeeded.')
	except ValueError, msg:
	    pass

	try:
	    if version < 20100:
		a, b = coerce(vers, '7.1 on')
		vers == b
	    else:
		vers == '7.1 on'
	    self.fail('Comparison to badly formatted string succeeded.')
	except ValueError, msg:
	    pass

	try:
	    if version < 20100:
		a, b = coerce(vers, '7.1devel')
		vers == b
	    else:
		vers == '7.1devel'
	except ValueError, msg:
	    self.fail('Comparison to development version failed.')

	try:
	    if version < 20100:
		a, b = coerce(vers, '7.1Devel')
		vers == b
	    else:
		vers == '7.1Devel'
	except ValueError, msg:
	    self.fail('Comparison to development version failed.')

	try:
	    if version < 20100:
		a, b = coerce(vers, '7.3rc2')
		vers == b
	    else:
		vers == '7.3rc2'
	except ValueError, msg:
	    self.fail('Comparison to release canidate version failed.')

	try:
	    if version < 20100:
		a, b = coerce(vers, '7.3b3')
		vers == b
	    else:
		vers == '7.3b3'
	except ValueError, msg:
	    self.fail('Comparison to release canidate version failed.')

	try:
	    if version < 20100:
		a, b = coerce(vers, '7.3a3')
		vers == b
	    else:
		vers == '7.3a3'
	except ValueError, msg:
	    self.fail('Comparison to release canidate version failed.')

	try:
	    vers == 4294967296L
	    self.fail('Comparison to large valued long succeeded.')
	except OverflowError, msg:
	    pass

	try:
	    vers == 4294967296.0
	    self.fail('Comparison to large valued float succeeded.')
	except OverflowError, msg:
	    pass

    def CheckPgVersionCoercion(self):
	global version

	# Note that these tests are designed to also check the calculation
	# of the post70 attribute.  For the first 2 coercions, post70 will
	# be 0.  It will be 1 for that second 2 coercions.

	a, b = coerce(vers, '6.1.5')
	self.failUnless(type(a) == type(b) == type(vers),
			'Coercion from string to PgVersion failed.')
	expected = ['PostgreSQL 6.1.5 on UNKNOWN, compiled by UNKNOWN',
		    6, 1, 5, 0]
	self.CheckMemberValues(b, expected)

	a, b = coerce(vers, 70003)
	self.failUnless(type(a) == type(b) == type(vers),
			'Coercion from int to PgVersion failed.')
	expected = ['PostgreSQL 7.0.3 on UNKNOWN, compiled by UNKNOWN',
		    7, 0, 3, 0]
	self.CheckMemberValues(b, expected)

	a, b = coerce(vers, 70100.0)
	self.failUnless(type(a) == type(b) == type(vers),
			'Coercion from float to PgVersion failed.')
	expected = ['PostgreSQL 7.1.0 on UNKNOWN, compiled by UNKNOWN',
		    7, 1, 0, 1]
	self.CheckMemberValues(b, expected)

	a, b = coerce(vers, 80002L)
	self.failUnless(type(a) == type(b) == type(vers),
			'Coercion from long to PgVersion failed.')
	expected = ['PostgreSQL 8.0.2 on UNKNOWN, compiled by UNKNOWN',
		    8, 0, 2, 1]
	self.CheckMemberValues(b, expected)

	# Now test coercions that will fail.

	try:
	    a, b = coerce(vers, '')
	    self.fail('Comparison to badly formatted string succeeded.')
	except ValueError, msg:
	    pass

	try:
	    a, b = coerce(vers, '7. 1.3')
	    self.fail('Comparison to badly formatted string succeeded.')
	except ValueError, msg:
	    pass

	try:
	    a, b = coerce(vers, 4294967296L)
	    self.fail('Comparison to large valued long succeeded.')
	except OverflowError, msg:
	    pass

	try:
	    a, b = coerce(vers, 4294967296.0)
	    self.fail('Comparison to large valued float succeeded.')
	except OverflowError, msg:
	    pass

    def CheckPgVersionSanity(self):
	self.CheckForMembers(vers)

	# Check the types of the members.
	self.assertEquals(type(vers.version), types.StringType,
			  "PgVersion.version is not a string.");
	self.assertEquals(type(vers.major), types.IntType,
			  "PgVersion.major is not an integer.");
	self.assertEquals(type(vers.minor), types.IntType,
			  "PgVersion.minor is not an integer.");
	self.assertEquals(type(vers.level), types.IntType,
			  "PgVersion.level is not an integer.");
	self.assertEquals(type(vers.post70), types.IntType,
			  "PgVersion.post70 is not an integer.");

	# Build up what the attributes of the PgVersion Object should be
	# from the result of the 'select version() query.
	a = vstr.split('.')
	major = int(a[0])
	minor = int(a[1])
	if len(a) < 3:
	    level = 0
	else:
	    level = int(a[2])
	value = (((major * 100) + minor) * 100) + level
	post70 = (value > 70100)

	# Compare the PgVersion object against the expected values.
	expected = [dbvstr, major, minor, level, post70]
	self.CheckMemberValues(vers, expected)

	# Compare the PgVersions value against the calculated value from it's
	# major, minor, and level attributes
	value = (((vers.major * 100) + vers.minor) * 100) + vers.level
	self.assertEquals(vers, value,
		 "PgVersion object's value does not match componet parts.");

if __name__ == "__main__":
    TestSuite = unittest.TestSuite()
    TestSuite.addTest(PgVersionTestCases("CheckPgVersionSanity"))
    TestSuite.addTest(PgVersionTestCases("CheckPgVersionCoercion"))
    TestSuite.addTest(PgVersionTestCases("CheckPgVersionComparison"))

    runner = unittest.TextTestRunner()
    runner.run(TestSuite)
