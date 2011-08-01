#ident "@(#) $Id: pymemstrdup.c,v 1.1 2001/08/03 23:09:26 ballie01 Exp $"
/* vi:set sw=4 ts=8 showmode ai: */
/**(H+)*****************************************************************\
| Name:		pymemstrdup.c						|
|									|
| Description:	This file implements a version of strdup() that allo-	|
|		cates it's memory from the Python's heap.		|
|=======================================================================|
| Copyright 2001 by Billy G. Allie.					|
| All rights reserved.							|
|									|
| Permission to use, copy, modify, and distribute this software and its	|
| documentation for any purpose and without fee is hereby granted, pro-	|
| vided that the above copyright notice appear in all copies and that	|
| both that copyright notice and this permission notice appear in sup-	|
| porting documentation, and that the copyright owner's name not be	|
| used in advertising or publicity pertaining to distribution of the	|
| software without specific, written prior permission.			|
|									|
| THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,	|
| INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN	|
| NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR	|
| CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS	|
| OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE	|
| OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE	|
| USE OR PERFORMANCE OF THIS SOFTWARE.					|
|=======================================================================|
| Revision History:							|
|									|
| Date      Ini Description						|
| --------- --- ------------------------------------------------------- |
| 02AUG2001 bga Initial release by Billy G. Allie.			|
\*(H-)******************************************************************/

#include <stdlib.h>
#include <string.h>
#include "Python.h"

void *PyMem_Strdup(void *src)
{
    void *new = PyMem_Malloc(strlen(src) + 1);

    if (new != (void *)NULL)
	strcpy(new, src);

    return new;
}
