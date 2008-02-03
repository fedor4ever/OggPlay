/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.

 ********************************************************************/
#ifndef _OS_TYPES_H
#define _OS_TYPES_H

#ifdef _LOW_ACCURACY_
#  define X(n) (((((n)>>22)+1)>>1) - ((((n)>>22)+1)>>9))
#  define LOOKUP_T const unsigned char
#else
#  define X(n) (n)
#  define LOOKUP_T const ogg_int32_t
#endif

// Lib C replacements
#include <OggShared.h>


#ifndef chmsg
#define chmsg(desc)
#endif

#include <e32def.h>
typedef TInt16 ogg_int16_t;
typedef TInt32 ogg_int32_t;
typedef TUint32 ogg_uint32_t;
#if ( ( defined( __WINS__ )  && defined(__VC32__) ) || defined (__WINSCW__) )
   typedef __int64 ogg_int64_t;
#define LITTLE_ENDIAN
#define inline __inline
#define alloca _alloca
#undef HAVE_ALLOCA_H
//#pragma chmsg(compiling for wins with vc32)
   
#elif defined (__MARM__)
#pragma chmsg(compiling for marm)

 typedef long long ogg_int64_t;

#define BIG_ENDIAN
#define _ARM_ASSEM_ 1

#define alloca __builtin_alloca

#elif defined( _WIN32 )

#  ifndef __GNUC__
   #pragma chmsg(_win32 defined, __GNUC__ not)

   /* MSVC/Borland */
   typedef __int64 ogg_int64_t;
   typedef __int32 ogg_int32_t;
   typedef unsigned __int32 ogg_uint32_t;
   typedef __int16 ogg_int16_t;
#  else
   #pragma chmsg(_win32 defined, __GNUC__ as well ?)
   /* Cygwin */
   #include <_G_config.h>

   typedef _G_int64_t ogg_int64_t;
   typedef _G_int32_t ogg_int32_t;
   typedef _G_uint32_t ogg_uint32_t;
   typedef _G_int16_t ogg_int16_t;
#  endif

#elif defined(__MACOS__)

#  include <sys/types.h>
   typedef SInt16 ogg_int16_t;
   typedef SInt32 ogg_int32_t;
   typedef UInt32 ogg_uint32_t;
   typedef SInt64 ogg_int64_t;

#elif defined(__MACOSX__) /* MacOS X Framework build */

#  include <sys/types.h>
   typedef int16_t ogg_int16_t;
   typedef int32_t ogg_int32_t;
   typedef u_int32_t ogg_uint32_t;
   typedef int64_t ogg_int64_t;

#elif defined(__BEOS__)

   /* Be */
#  include <inttypes.h>

#elif defined (__EMX__)

   /* OS/2 GCC */
   typedef short ogg_int16_t;
   typedef int ogg_int32_t;
   typedef unsigned int ogg_uint32_t;
   typedef long long ogg_int64_t;

#else

#  include <sys/types.h>
#  include "config_types.h"

#endif

#endif  /* _OS_TYPES_H */
