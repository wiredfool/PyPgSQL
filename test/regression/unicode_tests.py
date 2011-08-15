#!/usr/bin/env python
#-*- coding: utf-8 -*-
#ident "@(#) $Id: unicode_tests.py,v 1.3 2003/07/12 17:19:06 ghaering Exp $"
#-----------------------------------------------------------------------+
# Name:			unicode_tests.py										|
#																		|
# Synopsys:																|
#																		|
# Description:	Tests for the Unicode support in pyPgSQL.				|
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
# 15AUG2011 eds Added hostname, creating tables as temp. changed file   |
#               coding to utf-8, changed mix of tabs+sp to tabs@4       |
# 27OCT2002 gh	Corrected coypright statement.							|
# 27OCT2002 gh	Initial release by Gerhard Häring.						|
#-----------------------------------------------------------------------+
import types
import unittest

from pyPgSQL import PgSQL

# Note: This test suite requires
# - that a database 'pypgsql' exists with encoding set to UNICODE:
#	$ createdb pypgsql -E UNICODE
# - that this database has support for PL/PgSQL:
#	$ createlang plpgsql pypgsql

dbname = "pypgsql"
host = "192.168.10.249"

class UnicodeDatabaseTestCase(unittest.TestCase):
	def testInsertWithUtf8(self):
		conn = PgSQL.connect(database=dbname, host=host, client_encoding=("utf-8",), unicode_results=1)
		cursor = conn.cursor()
		cursor.execute("set client_encoding to unicode")

		l1_text = unicode("Österreich", "latin1")
		
		cursor = conn.cursor()
		cursor.execute("create temp table test (name varchar(20))")
		cursor.execute(u"insert into test(name) values (%s)", l1_text)
		cursor.execute("select name from test")
		name = cursor.fetchone().name
		self.failUnless(name == l1_text, "latin1 text from insert and select don't match")

	def testInsertWithLatin1(self):
		conn = PgSQL.connect(database=dbname, host=host, client_encoding=("latin1",), unicode_results=1)
		cursor = conn.cursor()
		cursor.execute("set client_encoding to latin1")

		l1_text = unicode("Österreich", "latin1")
		
		cursor = conn.cursor()
		cursor.execute("create temp table test (name varchar(20))")
		cursor.execute(u"insert into test(name) values (%s)", (l1_text,))
		cursor.execute("select name from test")
		name = cursor.fetchone().name
		self.failUnless(name == l1_text, "latin1 text from insert and select don't match")

	def testCallProcUtf8(self):
		text1 = unicode("Österreich-", "latin1")
		text2 = unicode("Ungarn", "ascii")
		
		conn = PgSQL.connect(database=dbname, host=host, client_encoding=("utf-8",), unicode_results=1)
		cursor = conn.cursor()
		cursor.execute("set client_encoding to unicode")
		cursor.execute(u"""
		CREATE FUNCTION concat_text (TEXT, TEXT) RETURNS TEXT AS '
		BEGIN
			RETURN $1 || $2;
		END;
		'LANGUAGE 'plpgsql';
		""")
		cursor.callproc(u"concat_text", text1, text2)
		result = cursor.fetchone()[0]
		self.failUnless(result == text1 + text2, "procedure didn't return the two strings concatenated")

	def testCallProcLatin1(self):
		text1 = unicode("Österreich-", "latin1")
		text2 = unicode("Ungarn", "ascii")
		
		conn = PgSQL.connect(database=dbname, host=host, client_encoding=("latin1",))
		cursor = conn.cursor()
		cursor.execute("set client_encoding to latin1")
		cursor.execute(u"""
		CREATE FUNCTION concat_text (TEXT, TEXT) RETURNS TEXT AS '
		BEGIN
			RETURN $1 || $2;
		END;
		'LANGUAGE 'plpgsql';
		""")
		cursor.callproc(u"concat_text", text1, text2)
		result = cursor.fetchone()[0]
		self.failUnless(unicode(result, "latin1") == text1 + text2, "procedure didn't return the two strings concatenated")


	def testCallProcLatin1UnicodeResults(self):
		text1 = unicode("Österreich-", "latin1")
		text2 = unicode("Ungarn", "ascii")
		
		conn = PgSQL.connect(database=dbname, host=host, client_encoding=("latin1",), unicode_results=1)
		cursor = conn.cursor()
		cursor.execute("set client_encoding to latin1")
		cursor.execute(u"""
		CREATE FUNCTION concat_text (TEXT, TEXT) RETURNS TEXT AS '
		BEGIN
			RETURN $1 || $2;
		END;
		'LANGUAGE 'plpgsql';
		""")
		cursor.callproc(u"concat_text", text1, text2)
		result = cursor.fetchone()[0]
		self.failUnless(result == text1 + text2, "procedure didn't return the two strings concatenated")


	def __testUnicodeConversion(self, item):
		text = unicode("Österreich", "latin1")

		conn = PgSQL.connect(database=dbname, host=host, client_encoding=("utf-8",), unicode_results=1)
		cursor = conn.cursor()
		cursor.execute("set client_encoding to unicode")
		cursor.execute("create temp table test (name varchar(20))")
		cursor.execute("insert into test (name) values (%s)", text)
		if type(item) is types.DictType:
			cursor.execute("select name from test where name=%(name)s", item)
		else:
			cursor.execute("select name from test where name=%s", item)
		result = cursor.fetchall()
		self.failUnless(len(result) == 1, "couldn't find the inserted string")

	def testTupleUnicodeConversion(self):
		self.__testUnicodeConversion((unicode("Österreich", "latin1"),))
		
	def testListUnicodeConversion(self):
		self.__testUnicodeConversion([unicode("Österreich", "latin1")])

	def testDictUnicodeConversion(self):
		self.__testUnicodeConversion({"name": unicode("Österreich", "latin1")})

	def testPgResultSetUnicodeConversion(self):
		text = unicode("Österreich", "latin1")

		conn = PgSQL.connect(database=dbname, host=host, client_encoding=("utf-8",), unicode_results=1)
		cursor = conn.cursor()
		cursor.execute("set client_encoding to unicode")
		cursor.execute("create temp table test (name varchar(20))")
		cursor.execute("insert into test (name) values (%s)", text)
		cursor.execute("select name from test")
		result = cursor.fetchone()

		cursor.execute("select name from test where name=%s", result[0])
		result2 = cursor.fetchall()
		self.failUnless(len(result2) == 1)

		cursor.execute("select name from test where name=%(name)s", result)
		result2 = cursor.fetchall()
		self.failUnless(len(result2) == 1, "couldn't find the inserted string")

def suite():
	unicode_tests = unittest.makeSuite(UnicodeDatabaseTestCase, "test")
	return unicode_tests

def main():
	runner = unittest.TextTestRunner()
	runner.run(suite())
  
if __name__ == "__main__":
	main()
