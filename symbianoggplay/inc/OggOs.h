/*
 *  Copyright (c) 2003 L. H. Wilden.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __OggOs_h
#define __OggOs_h


#if defined( __WINS__ )  && defined(__VC32__)
#define chSTR(x)           #x
#define chSTR2(x)        chSTR(x)
#define chmsg(desc) message(__FILE__ "("\
chSTR2(__LINE__) "):" #desc)

#else
#define chmsg(desc)
#endif

// VERSION
//
#define KVersionMajor 0
#define KVersionMinor 96
#define KVersionString "0.96"

#define MULTI_LINGUAL_INSTALLER

// DEBUGGING
//
#define TRACE_ON          // See OggLog. Manually create "C:\Logs\OggPlay" to get disk logging.


// BUILD TARGETS
//
// Enable this line to enforce the platform target.
#define SERIES60

// Automatic platform target. Seen fail on windows systems.
#if defined(__AVKON_ELAF__) || defined(__AVKON_APAC__)
  #if !defined(UIQ)
    #define SERIES60
#endif
  //#pragma chmsg(Building for SERIES60)
#else
  #if !defined(SERIES60)
    #define UIQ
  #endif
  //#pragma chmsg(Building for UIQ)
#endif


// NEW CODE ENABLERS
//
#if defined(SERIES60)
#define SERIES60_SPLASH
#endif

// Force filling the buffer to at least 75% of their capacity before sending them to
// the audio streaming device
// On NGage it gives a serious boost to avoid "not-ready buffer" problem.
// UIQ_?
#define FORCE_FULL_BUFFERS

// There is something wrong how Nokia audio streaming handles the
// first buffers. They are somehow swallowed.
// To avoid that problem, wait a short time before streaming, 
// so that Application drawing have been done.
#define DELAY_AUDIO_STREAMING_START

#define SEARCH_OGGS_FROM_ROOT


// OTHERS
//
#if defined SERIES60    // Alternative to #ifdef/#endif around Series 60 code
#define IFDEF_S60(x)   x
#else
#define IFDEF_S60(x)   ;
#endif


#endif
