// #ident "@(#) $Id: pg_types.h,v 1.5 2002/12/01 04:59:25 ballie01 Exp $"
#ifndef Pg_TYPES_H
#define Pg_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************\
| Copyright 2001 by Billy G. Allie					|
| All rights Reserved.							|
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
\***********************************************************************/

/* Character String */
#define PG_TEXT		25
#define PG_CHAR		18
#define PG_VARCHAR	1043

#define PG_NAME		19
#define PG_BPCHAR	1042

#define PG_BYTEA	17

/* Number */
#define PG_INT4		23
#define PG_INTEGER	PG_INT4
#define PG_INT2		21
#define PG_SMALLINT	PG_INT2
#define PG_INT8		20
#define PG_BIGINT	PG_INT8
#define PG_OID		26
#define PG_NUMERIC	1700
#define PG_FLOAT8	701
#define PG_FLOAT	PG_FLOAT8
#define PG_FLOAT4	700

#define PG_CASH		790

#define PG_TID		27
#define PG_XID		28
#define PG_CID		29

/* Temporal */
#define PG_DATE		1082
#define PG_TIME		1083
#define PG_TIMESTAMP	1114
#define PG_TIMESTAMPTZ	1184
#define PG_INTERVAL	1186

#define PG_ABSTIME	702
#define PG_RELTIME	703
#define PG_TINTERVAL	704

#define PG_TIMETZ	1266

/* Logical */
#define PG_BOOL		16

/* Geometric */
#define PG_POINT	600
#define PG_LSEG		601
#define PG_PATH		602
#define PG_BOX		603
#define PG_LINE		628
#define PG_CIRCLE	718
#define PG_POLYGON	604

/* Network */
#define PG_MACADDR	829
#define PG_INET		869
#define PG_CIDR		650

/* Bit strings */
#define PG_ZPBIT	1560
#define PG_VARBIT	1562

/* Misc. */
#define PG_REGPROC	24
#define PG_INT2VECTOR	22
#define PG_OIDVECTOR	30
#define PG_UNKNOWN	705
#define PG_ACLITEM	1033
#define PG_REFCURSOR	1790

#ifdef __cplusplus
}
#endif
#endif /* !Pg_TYPES_H */
