#!/usr/local/bin/python
#ident "$Id: demo2b.py,v 1.3 2001/10/18 03:17:08 ballie01 Exp $"
#-----------------------------------------------------------------------+
# demo2b.py - Print out the results of a select (similar to pgsql).	|
#-----------------------------------------------------------------------+
import time
from pyPgSQL import PgSQL

c = PgSQL.connect()

c.autocommit = 1 # Auto commit must be on to drop the tables!

c1 = c.cursor()

# Make sure that the tables and rules don't exist.
try:
    c1.execute("DROP RULE demo2r1")
except StandardError:
    pass

try:
    c1.execute("DROP TABLE demo2a")
except StandardError:
    pass

try:
    c1.execute("DROP TABLE demo2b")
except StandardError:
    pass

# Create the tables and rule.
c1.execute("""
CREATE TABLE demo2a (
	updated timestamp default now(),
	name varchar(30)
        )""")
c1.execute("""
CREATE TABLE demo2b (
	updated timestamp default now(),
	name varchar(30)
        )""")
c1.execute("""
CREATE RULE demo2r1 AS ON INSERT TO demo2b DO
	(INSERT INTO demo2a (name) values (new.name); NOTIFY demo2a)""")

c1.close()
del c1

c.autocommit = 0

c1 = c.cursor()

# Update the table (which kicks off the rule which sends the notify
# Note: The notification does not occur until a commit() happens.  Multiple
#	updates to demo2b with a single commit() will only send one notice.

c1.execute("INSERT INTO demo2b (name) VALUES ('This is a test(1)')")
c1.execute("INSERT INTO demo2b (name) VALUES ('This is another test(2)')")
c.commit()	# Note: 2 updates, 1 notice.
time.sleep(5)
c1.execute("INSERT INTO demo2b (name) VALUES ('And another test(3)')")
c.commit()
time.sleep(10)
c1.execute("INSERT INTO demo2b (name) VALUES ('The final insert(4)')")
c.commit()

print "\nThere should of been 3 notices sent and recieved.\n"

del c1, c
