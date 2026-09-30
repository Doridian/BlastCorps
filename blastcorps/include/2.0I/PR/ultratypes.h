#ifndef _ULTRATYPES_H_
#define _ULTRATYPES_H_


/**************************************************************************
 *                                                                        *
 *               Copyright (C) 1995, Silicon Graphics, Inc.               *
 *                                                                        *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *                                                                        *
 **************************************************************************/


/*************************************************************************
 *
 *  File: ultratypes.h
 *
 *  This file contains various types used in Ultra64 interfaces.
 *
 *  $Revision: 1.6 $
 *  $Date: 1997/12/17 04:02:06 $
 *  $Source: /disk6/Master/cvsmdev2/PR/include/ultratypes.h,v $
 *
 **************************************************************************/



/**********************************************************************
 * General data types for R4300
 */
#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)

#ifdef TARGET_PC
/* the port: 32-bit whatever the host's long is (docs/PORT.md, "The LP64
   build") */
typedef unsigned char			u8;	/* unsigned  8-bit */
typedef unsigned short			u16;	/* unsigned 16-bit */
typedef unsigned int			u32;	/* unsigned 32-bit */
typedef unsigned long long		u64;	/* unsigned 64-bit */

typedef signed char			s8;	/* signed  8-bit */
typedef short				s16;	/* signed 16-bit */
typedef int				s32;	/* signed 32-bit */
typedef long long			s64;	/* signed 64-bit */

typedef volatile unsigned char		vu8;	/* unsigned  8-bit */
typedef volatile unsigned short		vu16;	/* unsigned 16-bit */
typedef volatile unsigned int		vu32;	/* unsigned 32-bit */
typedef volatile unsigned long long	vu64;	/* unsigned 64-bit */

typedef volatile signed char		vs8;	/* signed  8-bit */
typedef volatile short			vs16;	/* signed 16-bit */
typedef volatile int			vs32;	/* signed 32-bit */
typedef volatile long long		vs64;	/* signed 64-bit */

#else
typedef unsigned char			u8;	/* unsigned  8-bit */
typedef unsigned short			u16;	/* unsigned 16-bit */
typedef unsigned long			u32;	/* unsigned 32-bit */
typedef unsigned long long		u64;	/* unsigned 64-bit */

typedef signed char			s8;	/* signed  8-bit */
typedef short				s16;	/* signed 16-bit */
typedef long				s32;	/* signed 32-bit */
typedef long long			s64;	/* signed 64-bit */

typedef volatile unsigned char		vu8;	/* unsigned  8-bit */
typedef volatile unsigned short		vu16;	/* unsigned 16-bit */
typedef volatile unsigned long		vu32;	/* unsigned 32-bit */
typedef volatile unsigned long long	vu64;	/* unsigned 64-bit */

typedef volatile signed char		vs8;	/* signed  8-bit */
typedef volatile short			vs16;	/* signed 16-bit */
typedef volatile long			vs32;	/* signed 32-bit */
typedef volatile long long		vs64;	/* signed 64-bit */

#endif

/* A pointer the N64's code or data share with the C: 32 bits in memory
 * even where the host's are 64 (the LP64 port, docs/PORT.md "The LP64
 * build"; clang's __ptr32 __uptr, a zero-extended 32-bit pointer).
 * `T *PTR32 p;` */
#if defined(TARGET_PC) && defined(PORT_LP64)
#define PTR32 __ptr32 __uptr
#else
#define PTR32
#endif

typedef float				f32;	/* single prec floating point */
typedef double				f64;	/* double prec floating point */

#if !defined(_SIZE_T) && !defined(_SIZE_T_) && !defined(_SIZE_T_DEF)
#define _SIZE_T
#define _SIZE_T_DEF			/* exeGCC size_t define label */
#if defined(TARGET_PC) && defined(__LP64__)
typedef __SIZE_TYPE__   size_t;
#elif (_MIPS_SZLONG == 32)
typedef unsigned int    size_t;
#endif
#if (_MIPS_SZLONG == 64)
typedef unsigned long   size_t;
#endif
#endif

#endif  /* _LANGUAGE_C */


/*************************************************************************
 * Common definitions
 */
#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL    0
#endif

#endif  /* _ULTRATYPES_H_ */

