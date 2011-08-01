#!/usr/local/bin/python
#ident "$Id: demo1b.py,v 1.2 2001/10/18 03:17:08 ballie01 Exp $"
#-----------------------------------------------------------------------+
# demo1b.py - implement the sample program #1 from the PostgreSQL libpq	|
#	      doumentation using the PgSQL (DB-API 2.0 compliant) module|
#-----------------------------------------------------------------------+
import sys
from pyPgSQL import PgSQL

dbname = 'template1'

# Make a connection to the database and check to see if it succeeded.
try:
    cnx = PgSQL.connect(database=dbname)
except PgSQL.Error, msg:
    print "Connection to database '%s' failed" % dbname
    print msg,
    sys.exit()

# Create a Cursor object.  This handles the transaction block and the
# declaration of the database cursor.  

cur = cnx.cursor()

# Fetch instances of pg_database, the system catalog of databases.

try:
    cur.execute("SELECT * FROM pg_database")
except PgSQL.Error, msg:
    print "Select from pg_database failed\n%s" % msg,
    sys.exit()

try:
    res = cur.fetchall()
except StandardError, msg:
    print "Fetch of all instanaces failed\n%s" % msg,
    sys.exit()

# First, print out the attibute names.

fmt = ""

for i in cur.description:
    sys.stdout.write('%-15s' % i[0])
    fmt = fmt + '%-15s'	# build up the format string for use later on.

print '\n'

# Next, print out the instances.

for i in res:
    print fmt % tuple(i)

# Close the cursor

cur.close()

# Commit the transaction

cnx.commit()

# That's all folks!

del cnx, cur, res
