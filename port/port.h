#if ! defined(PYPGSQL_COMPAT_H)
#define PYPGSQL_COMPAT_H

/**********************************************************************
 * Get various constants for LONG_LONG and unsigned LONG_LONG defined *
 **********************************************************************/
#include "Python.h"

#ifdef MS_WIN32
#ifndef LLONG_MAX
#define LLONG_MAX _I64_MAX
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX _UI64_MAX
#endif
#ifndef LLONG_MIN
#define LLONG_MIN _I64_MIN
#endif
#endif /* MS_WIN32 */

/* There are compiler/platform combinations that HAVE_LONG_LONG, but don't
 * provide the respective constants by default. Examples are Linux glibc and
 * Cygwin.  Only define HAVE_LONG_LONG_SUPPORT if both conditions are
 * fulfilled.
 */
#if defined(HAVE_LONG_LONG) && defined(LLONG_MAX) && defined(ULLONG_MAX)  \
    && defined(LLONG_MIN)
#define HAVE_LONG_LONG_SUPPORT
#endif

/* Under Python 2.3, LONG_LONG has been replaced by PY_LONG_LONG. We continue
 * using LONG_LONG, even under 2.3 or later */
#ifdef PY_LONG_LONG
#define LONG_LONG PY_LONG_LONG
#endif

/*******************************************************
 * Prototypes for the string functions that we include * 
 *******************************************************/
char * pg_strtok_r(char *s, const char *delim, char **last);

char * pg_strtok(char *s, const char *delim);

#if defined(HAVE_LONG_LONG_SUPPORT)
LONG_LONG pg_strtoll(const char *nptr, char **endptr, register int base);
unsigned LONG_LONG pg_strtoull(const char *nptr, char **endptr, register int base);
#endif

#endif /* !defined(PYPGSQL_COMPAT_H) */
