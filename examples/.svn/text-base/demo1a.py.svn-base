#!/usr/local/bin/python
#ident "$Id: demo1a.py,v 1.2 2001/10/18 03:17:08 ballie01 Exp $"
#-----------------------------------------------------------------------+
# demo1a.py - implement the sample program #1 from the PostgreSQL libpq	|
#	      doumentation using the libpq module.			|
#-----------------------------------------------------------------------+
import sys
from pyPgSQL import libpq

dbname = 'template1'

# Make a connection to the database and check to see if it succeeded.
try:
    cnx = libpq.PQconnectdb('dbname=%s' % dbname)
except libpq.Error, msg:
    print "Connection to database '%s' failed" % dbname
    print msg,
    sys.exit()

# Start a transaction block.
try:
    res = cnx.query('BEGIN')
    # The following test could also be res.resultType != libpq.RESULT_ERROR
    if res.resultStatus != libpq.COMMAND_OK:
	raise libpq.Error, "ERROR: command failed"
except libpq.Error, msg:
    print "BEGIN command failed"
    sys.exit()

# Don't need the result anymore - get rid of it.
del res		# Note that this is not necessary.  When we assign a new result
		# to res, the previous result is cleaned up automagically.

# Fetch instances of pg_database, the system catalog of databases.

try:
    res = cnx.query("DECLARE mycursor CURSOR FOR SELECT * FROM pg_database")
    if res.resultStatus != libpq.COMMAND_OK:
	raise libpq.Error, "ERROR: command failed"
except libpq.Error, msg:
    print "DECLARE CURSOR command failed"
    sys.exit()

try:
    res = cnx.query("FETCH ALL FROM mycursor")
    if res.resultStatus != libpq.TUPLES_OK:
	raise libpq.Error, "ERROR: command failed"
except libpq.Error, msg:
    print "FETCH ALL command didn't return tuples properly"
    sys.exit()

# First, print out the attibute names.

for i in range(res.nfields):
    sys.stdout.write('%-15s' % res.fname(i))
print '\n'

# Next, print out the instances.

for i in range(res.ntuples):
    for j in range(res.nfields):
	sys.stdout.write('%-15s' % res.getvalue(i, j))
    print

# Close the cursor

res = cnx.query("CLOSE mycursor")

# Commit the transaction

res = cnx.query("COMMIT")

# That's all folks!

del cnx, res
