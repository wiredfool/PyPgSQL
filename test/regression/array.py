#!/usr/local/bin/python
# -*- coding: utf-8 -*-
#ident "@(#) $Id: array.py,v 1.4 2003/07/12 17:19:06 ghaering Exp $"
#-----------------------------------------------------------------------+
# Name:			array.py												|
#																		|
# Description:	array.py contains test cases for using the PostgreSQL	|
#				array type from within the PgSQL module.				|
#																		|
# Note:			These test cases requires that a test database named	|
#				pypgsql exists and that the person running the test has |
#				full rights to the database.							|
#=======================================================================|
# Copyright 2002 by Gerhard Häring.										|
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
# 02NOV2002 bga Modified code to use the PgArray class for PostgreSQL	|
#				arrays.													|
# 21APR2002 gh	Added some more mean testcases, which test for control	|
#				characters and other special characters inside strings, |
#				and multi-dimensional arrays.							|
# 16APR2002 gh	Initial release by Gerhard Häring.						|
#-----------------------------------------------------------------------+
import unittest, types
import sys
from pyPgSQL import PgSQL
PgArray = PgSQL.PgArray

class ArrayTestCases(unittest.TestCase):
	def setUp(self):
		self.conn = PgSQL.connect(database="pypgsql", host="192.168.10.249")
		self.conn.autocommit = 1

	def checkEqual(self, real, planned):
		self.failUnlessEqual(real.value, planned.value, \
			"Received %s, should have been %s" % (repr(real), repr(planned)))

	def CheckForIntegerInsert(self):
		ivalues = PgArray([3,4,5])
		cursor = self.conn.cursor()
		try:
			cursor.execute("drop table test")
		except:
			pass
		cursor.execute("create table test (ia int[])")
		cursor.execute("insert into test(ia) values (%s)", ivalues)
		cursor.execute("select ia from test")
		result = cursor.fetchone()
		self.failUnless(isinstance(result.ia, PgArray), \
			"the integer array isn't returned as a PgArray")
		self.checkEqual(result.ia, ivalues)

	def CheckForNumericInsert(self):
		numlist = PgArray(map(PgSQL.PgNumeric, ['4.7', '-3.2', '23.17']))

		cursor = self.conn.cursor()
		try:
			cursor.execute("drop table test")
		except:
			pass
		cursor.execute("create table test(na numeric[])")
		cursor.execute("insert into test(na) values (%s)", numlist)
		cursor.execute("select na from test")
		result = cursor.fetchone()
		self.failUnless(isinstance(result.na, PgArray), \
			"the numeric array isn't returned as a PgArray")
		self.checkEqual(result.na, numlist)

	def CheckForStringInsert(self):
		cursor = self.conn.cursor()

		stringlists = PgArray([['hey', 'you', 'there'],
							   ['fancy', ',', '{chars}', '"'],
							   ['what about', 'some \\backslashes'],
							   ['some more \\', '\\ and \\ etc'],
							   ['aa', '%s\n%s' % (chr(10), chr(29))],
							   ['what about backslashes', 'at the end\\'],
							   ['interesting', '"', '\\"', '\\"\\'],
							   ['{}\\', '"\\}}\\\'"']])

		for stringlist in stringlists:
			try:
				cursor.execute("drop table test")
			except:
				pass
			cursor.execute("create table test(va varchar[])")
			cursor.execute("insert into test(va) values (%s)", stringlist)
			cursor.execute("select va from test")
			result = cursor.fetchone()
			self.failUnless(isinstance(result.va, PgArray), \
				"the varchar array isn't returned as a PgArray")
			self.checkEqual(result.va, stringlist)

	def CheckForStringInsertBruteForce(self):
		cursor = self.conn.cursor()

		chars = "\"'{}\\\n%s%sabc" % (chr(14), chr(5))
		
		try:
			cursor.execute("drop table test")
		except:
			pass
		cursor.execute("create table test(va varchar[])")
		self.conn.commit()
		for i in chars:
			for j in chars:
				for k in chars:
					cursor.execute("insert into test(va) values (%s)", PgArray([i+j+k]))
					cursor.execute("select va from test where oid=%s", cursor.oidValue)
					result = cursor.fetchone()
					self.failUnless(isinstance(result.va, PgArray), \
						"the varchar array isn't returned as a PgArray")
					self.checkEqual(result.va, [i+j+k])

	def CheckForStringInsertMultiDim(self):
		cursor = self.conn.cursor()

		stringlists =  PgArray([[["a", "b"], ["c", "d"]],
								[[["a", "b"], ["c", "d"]],
								 [["e", None], ["g", "h"]]],
								[[["{}", "\005{}\\'"], ["'asf'", "[\\{]"]],
								 [["e", "f"], ["g", "h"]]],
								[["a", "b"], ["c", "d"]]])

		for stringlist in stringlists:
			try:
				cursor.execute("drop table test")
			except:
				pass
			cursor.execute("create table test(va varchar[])")
			cursor.execute("insert into test(va) values (%s)", stringlist)
			cursor.execute("select va from test")
			result = cursor.fetchone()
			self.failUnless(isinstance(result.va, PgArray), \
				"the varchar array isn't returned as a PgArray")

			expected = stringlist[:]
			#try:  # Behaviour change -- returning none instead of null
			#	if expected[1][0][1] is None:
			#		expected[1][0][1] = ''
			#except IndexError, reason:
			#	pass

			self.checkEqual(result.va, expected)

	def CheckForIntInsertMultiDim(self):
		cursor = self.conn.cursor()

		intlists = PgArray([[[1,2], [3,4]],
							[[-5,3,4], [2, None, 10]]])

		# Change!
		# Null returning None, not 0
		#excpected_intlists = PgArray([[[1,2], [3,4]],
		#							  [[-5,3,4], [2, 0, 10]]])
		excpected_intlists = intlists

		for intlist, expected in zip(intlists, excpected_intlists):
			try:
				cursor.execute("drop table test")
			except:
				pass
			cursor.execute("create table test(ia int[])")
			cursor.execute("insert into test(ia) values (%s)", intlist)
			cursor.execute("select ia from test")
			result = cursor.fetchone()
			self.failUnless(isinstance(result.ia, PgArray), \
				"the int array isn't returned as a PgArray")
			self.checkEqual(result.ia, expected)

if __name__ == "__main__":
	TestSuite = unittest.TestSuite()

	TestSuite.addTest(ArrayTestCases("CheckForIntegerInsert"))
	TestSuite.addTest(ArrayTestCases("CheckForNumericInsert"))
	TestSuite.addTest(ArrayTestCases("CheckForStringInsert"))
	#TestSuite.addTest(ArrayTestCases("CheckForStringInsertBruteForce"))
	TestSuite.addTest(ArrayTestCases("CheckForStringInsertMultiDim"))
	TestSuite.addTest(ArrayTestCases("CheckForIntInsertMultiDim"))

	runner = unittest.TextTestRunner()
	runner.run(TestSuite)
