#!/usr/bin/env python
#ident "@(#) $Id: setup.py,v 1.28 2006/06/07 17:21:28 ghaering Exp $"
#-----------------------------------------------------------------------+
# Name:			setup.py												|
#																		|
# Synopsis:		python setup.py build	# Build the module.				|
#				python setup.py install # Install the module.			|
#																		|
#				See http://www.python.org/sigs/distutils-sig/doc/ for	|
#				more information on using distutils to install Python	|
#				programs and modules.									|
#																		|
# Description:	Setup script (using the distutils framework) for		|
#				pyPgSQL.												|
#=======================================================================|
# Copyright 2001 - 2006 by Gerhard Haering.								|
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
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS.	 IN |
# NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR	|
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS	|
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE |
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE	|
# USE OR PERFORMANCE OF THIS SOFTWARE.									|
#=======================================================================|
# People who have worked on this code.									|
#																		|
# Ini Name																|
# --- ----------------------------------------------------------------- |
#  gh Gerhard Haering <gerhard.haering@gmx.de>							|
# bga Billy G. Allie <bill.allie@mug.org>								|
# eds Eric Soroos <eric@soroos.net>                                     |
#=======================================================================|
# Revision History:														|
#																		|
# Date		Ini Description												|
# --------- --- ------------------------------------------------------- |
# 01AUG2011 eds Patch for hex bytea encoding used in pg9.0+, fixes for  |
#               mixed tabs and spaces (2.5.2)                           |
# 02JUN2006 gh  Use pg_config on all platforms.                         |
# 01JUN2006 gh  Removed RPM-specific hack. Use pg_config on win32.      |
# 26SEP2005 gh  Changed win32 build process to use MSVC.NET (needed     |
#               for Python 2.4, and add an additonal needed library     |
#               for PostgreSQL 8.                                       |
#		[Bug #1154791]					                                |
# 05MAR2004 bga Added /usr/include/postgresql to includes.		        |
#		[Bug #1154791]						                            |
# 25APR2003 gh	Added changes for registration with the Python Package	|
#				index (PyPI) (http://python.org/pypi).					|
# 01DEC2002 gh	Simplified build process and got rid of setup.config.	|
#				setup.config prevented the bdist_rpm command of dist-	|
#				utils to work and generally wasn't such a good idea as	|
#				it looked first. Pretty much the same effect is now		|
#				reached by the USE_CUSTOM variable with which users		|
#				can override the platform detection.					|
# 06OCT2002 bga Added support for UnixWare7 and OpenUNIX 8.				|
# 16SEP2002 gh	Total rework to make setup.py work out of the box for	|
#				most popular platforms, while still allowing flexibi-	|
#				lity through setup.config.								|
# 29AUG2001 gh	Reflected changed PostgreSQL win32 build process (Post- |
#				greSQL now built with mingw32)							|
# 28AUG2001 bga Add include_dirs and lib_dirs for building on cygwin.	|
#				This change should allow pyPgSQL to build 'out of the	|
#				box' on MS Windows using the cygwin environment.		|
# 20AUG2001 bga Modified to include the new source files.				|
#			--- Added code to determine any needed runtime library		|
#				search directories.										|
# 04AUG2001 gh	Modified to include the files pgversion.c,				|
#				pymemstrdup.c and strtok.c.								|
# 26JUL2001 bga Modified to use included strtoll.c and strtoull.c files.|
#			--- Added this flower box and cleaned up file.				|
# 22JUL2001 gh	Modified to build win32 version.						|
# 01JUN2001 gh	Initial version created by Gerhard Haering.				|
#-----------------------------------------------------------------------+
import os, os.path, sys

from distutils.core import setup
from distutils.extension import Extension

__version__ = "2.5.3"

# Define the runtime library path for this module.	It starts out as None.

def main():
	# Default settings, may be overriden for specific platforms
	pypgsql_rt_dirs = None
	optional_libs = ["pq"]
	modname = "pyPgSQL.libpq.libpqmodule"

	sources = ["libpqmodule.c",	 "pgboolean.c",
		"pgint2object.c", "pgint8object.c",
		"pgversion.c",	  "pglargeobject.c",
		"pgnotify.c",	  "pgconnection.c",
		"pgresult.c",	  "pymemstrdup.c",
		"port/strtoll.c", "port/strtoull.c",
		"port/strtok.c"]

	include_dirs = [os.popen("pg_config --includedir").read().strip()]
	library_dirs = [os.popen("pg_config --libdir").read().strip()]

	if sys.platform == "darwin": # Mac OS X
		optional_libs += ["ssl", "crypto"]
	elif sys.platform == "win32":
		library_dirs[0] = library_dirs[0] + "/ms"
		optional_libs = ["libpq", "wsock32", "advapi32"]
		modname="pyPgSQL.libpq.libpq"

	# patch distutils if it can't cope with the "classifiers" keyword
	if sys.version < '2.2.3':
		from distutils.dist import DistributionMetadata
		DistributionMetadata.classifiers = None
		DistributionMetadata.download_url = None

	classifiers = [
		"Development Status :: 5 - Production/Stable",
		"Environment :: Other Environment",
		"Intended Audience :: Developers",
		"License :: OSI Approved :: Python License (CNRI Python License)",
		"Natural Language :: English",
		"Operating System :: Microsoft :: Windows :: Windows 95/98/2000",
		"Operating System :: POSIX",
		"Programming Language :: C",
		"Programming Language :: Python",
		"Topic :: Database :: Front-Ends"]

	setup (
		name = "pyPgSQL",
		version = __version__,
		description = \
			"pyPgSQL - A Python DB-API 2.0 compliant interface to PostgreSQL.",
		maintainer = "pyPgSQL developers",
		maintainer_email = "pypgsql-devel@lists.sourceforge.net",
		url = "http://pypgsql.sourceforge.net/",
		license = "Python",
		packages = ["pyPgSQL", "pyPgSQL.libpq"],
		ext_modules = [Extension(
			name=modname,
			sources = sources,
			include_dirs = include_dirs,
			library_dirs = library_dirs,
			runtime_library_dirs = pypgsql_rt_dirs,
			libraries = optional_libs
			)],
		classifiers = classifiers
	)

if __name__ == "__main__":
	main()

