#!/usr/local/bin/python
#ident "$Id: demo2a.py,v 1.4 2001/10/18 03:17:08 ballie01 Exp $"
#-----------------------------------------------------------------------+
# demo2a.py - implement the sample program #2 from the PostgreSQL libpq	|
#	      doumentation using the libpq module.			|
#=======================================================================|
# Start this program and then run psql in another window and enter:	|
#									|
#	NOTIFY demo2a;							|
#									|
# Or you can run demo2b.py, which will create a 'demo2a' and 'demo2b'	|
# tables with a rule to insert into demo2a the data that is inserted	|
# into demo2b and then do a "NOTIFY demo2a".				|
#									|
# Note: The table names in sample program #2 were change from 'tbl1'	|
#	and 'tbl2' to 'demo2b' and 'demo2a'.				|
#									|
# Note: Make sure that the dbname variable is set to a database you	|
#	have access to and that the database name is the same in the	|
#	demo2b progam.  If you use psql to send the notice, use the	|
#	same database as the one specified by dbname.			|
#-----------------------------------------------------------------------+
import sys
import select
from pyPgSQL import libpq

dbname = 'bga'

# Make a connection to the database and check to see if it succeeded.
try:
    cnx = libpq.PQconnectdb('dbname=%s' % dbname)
except libpq.Error, msg:
    print "Connection to database '%s' failed" % dbname
    print msg,
    sys.exit()

# Register an interest for notifications on 'demo2a'
try:
    res = cnx.query('LISTEN demo2a')
    # The following test could also be res.resultType == libpq.RESULT_ERROR
    if res.resultStatus != libpq.COMMAND_OK:
	raise libpq.Error, "ERROR: command failed"
except libpq.Error, msg:
    print "LISTEN command failed"
    sys.exit()

# Don't need the result anymore - get rid of it.
del res		# Note that this is not necessary.  When we assign a new result
		# to res, the previous result is cleaned up automagically.

# To make this also work on Windows, we use select.select() instead of the much
# nicer select.poll()

sys.stdout.write('Waiting.')

while 1:
    # Wait a little (using select())
    sys.stdout.flush()
    ready_sockets = select.select([cnx.socket], [], [], 1.0)[0]
    if len(ready_sockets) == 0:
	sys.stdout.write('.')
    else:
	sys.stdout.write('\n')
	cnx.consumeInput()
	note = cnx.notifies()
	while note:
	    sys.stdout.flush()
	    sys.stdout.write(
		"ASYNC NOTICE of '%s' from backend pid %d recieved\n" %
		    (note.relname, note.be_pid))
	    note = cnx.notifies()
	sys.stdout.write('Waiting.')

