#!/usr/local/bin/python
import sys, os, string
from pyPgSQL import PgSQL
 
def instructions():
    print """
This program is a basic example of using PgSQL and Python to access a
PostgreSQL database.

Usage:	basic.py DSN user password

Where	DSN is "{host:{port}:}database".  Note: Items between { } are optional.
	user is the user ID to use when connecting to the database.
	password is the password to use when connecting to the database.
	"""

class basic:
    """
basic - a short example of using PgSQL loosely based on the basic.java
	program supplied with the PostgreSQL jdbc driver. """

    def __init__(self, *args):
	if len(args) < 3:
	    raise Exception, "Wrong number of arguments"

	dsn, user, pswd = args

	dsnc = string.split(dsn, ":")

	ldsnc = len(dsnc)

	if ldsnc == 1:
	    dbase = dsnc[0]
	    dsn = "::%s:%s:%s" % (dsn, user, pswd)
	elif ldsnc == 3:
	    dbase = dsnc[2]
	    dsn = "%s:%s:%s" % (dsn, user, pswd)
	else:
	    raise Exception, "Invalid DSN value"

	# If the dbase variable is empty, then put the user name of the user
	# running the program into dbase.
	if dbase == None or dbase == "":
	    if os.environ.has_key("LOGNAME"):
		dbase = os.environ["LOGNAME"]
	    elif os.environ.has_key("USER"):
		dbase = os.environ["USER"]
	    elif os.environ.has_key("USERNAME"):
		dbase = os.environ["USERNAME"]
	    else:
		dbase = "*UNKNOWN*"

	# Connect to the database
	print "Connecting to database %s as user %s" % (dbase, user)
	self.db = PgSQL.connect(dsn)

	print "Connected ... Now creating a cursor"
	self.st = self.db.cursor()

	# Clean up from a previous run, if needed.
	self.cleanup()

	# Run the tests
	self.doexample()

	# Clean up the database.
	self.cleanup()

	print "\nNow closing the connection"
	self.st.close()
	self.db.close()

    def cleanup(self):
	try:
	    self.st.execute("DROP TABLE basic")
	except:
	    pass

    def doexample(self):
	print "\nRunning tests:"

	# First we need a table to store data in
	self.st.execute("CREATE TABLE basic (a int2, b int2)")

	# Now insert some data, using the cursor
	self.st.execute("INSERT INTO basic VALUES (1, 1)")
	self.st.execute("INSERT INTO basic VALUES (2, 1)")
	self.st.execute("INSERT INTO basic VALUES (3, 1)")

	# This shows how to get the OID of the just inserted row
	self.st.execute("INSERT INTO basic VALUES (4, 1)")
	insertedOID = self.st.oidValue
	print "Inserted row with oid %d" % insertedOID

	# Now change the value of b from 1 to 8
	self.st.execute("UPDATE basic SET b = 8")
	print "Updated %s rows" % (self.st.res.cmdTuples, )

	# Now delete two rows
	self.st.execute("DELETE FROM basic WHERE a < 3")
	print "Deleted %s rows" % (self.st.res.cmdTuples, )

	# Now add some rows using executemany()
	# The values (4, 2), (4, 3), and (4, 4) will be added.
	parms = []
	for i in range(2, 5):
	    parms.append((4, i))
	self.st.executemany("INSERT INTO basic VALUES (%s, %s)", parms)

	# Finally perform a query on the table, showing how to access the
	# data by column number.
	print "\nPerforming a query, referencing the data by column number"
	print """i.e. 'print "    a=%d b=%d" % (res[0], res[1])'"""
	self.st.execute("SELECT a, b FROM basic")
	res = self.st.fetchone()
	while res:
	    print "    a=%d b=%d" % (res[0], res[1])
	    res = self.st.fetchone()

	# show how to access the data by column name.
	print "\nDisplaying results, referencing the data by column name"
	print """i.e. 'print "    a=%d b=%d" % (res["a"], res["b"])'"""
	self.st.rewind()
	res = self.st.fetchone()
	while res:
	    print "    a=%d b=%d" % (res["a"], res["b"])
	    res = self.st.fetchone()

	# Finally perform a query on the table, showing how to access the
	# data by column name.
	print "\nDisplaying results, using column name as an attribue"
	print """i.e. 'print "    a=%d b=%d" % (res.a, res.b)'"""
	self.st.rewind()
	res = self.st.fetchone()
	while res:
	    print "    a=%d b=%d" % (res.a, res.b)
	    res = self.st.fetchone()

	# Redisplay the results, showing how to access the data using the
	# result as a dictionary.
	print "\nDisplaying results, using the ResultSet as a dictionary"
	print """i.e. 'print "    a=%(a)d b=%(b)d" % res'"""
	self.st.rewind()
	res = self.st.fetchone()
	while res:
	    print "    a=%(a)s b=%(b)s" % res
	    res = self.st.fetchone()

if __name__ == "__main__":
    try:
	if len(sys.argv) < 4:
	    raise Exception, "Wrong number of arguments"
	basic(sys.argv[1], sys.argv[2], sys.argv[3])
    except Exception, m:
	print m
	instructions()
    
