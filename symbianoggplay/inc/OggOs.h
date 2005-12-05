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

#undef chmsg // Not particularily clever, but hides useless warning msg.
#if defined( __WINS__ )  && defined(__VC32__)
#define chSTR(x)           #x
#define chSTR2(x)        chSTR(x)
#define chmsg(desc) message(__FILE__ "("\
chSTR2(__LINE__) "):" #desc)

#else
#define chmsg(desc)
#endif


#define MULTI_LINGUAL_INSTALLER

// DEBUGGING
//
#define TRACE_ON          // See OggLog. Manually create "C:\Logs\OggPlay" to get disk logging.


// BUILD TARGETS
//
// Enable this line to enforce the platform target.
#define SERIES60
//#define SERIES80
//#define UIQ

// Enable this line to choose the OS OggPlay will run on.
//#define OS70S

//# Version String
#if defined(SERIES60)
 #if defined(OS70S)
  #define KVersionMajor 1
  #define KVersionMinor 64
  #define KVersionString "1.64"
 #else
  #define KVersionMajor 1
  #define KVersionMinor 9
  #define KVersionString "1.09b"
 #endif
#endif
#if defined(UIQ)
 #define KVersionMajor 1
 #define KVersionMinor 31
 #define KVersionString "1.31"
#endif
#if defined(SERIES80)
 #define KVersionMajor 1
 #define KVersionMinor 65
 #define KVersionString "1.65"
#endif

// There is a prize to the first to get rid of this. Switches '/' and '\' in path(s).
#define DOS

// Switches at least the splash bitmap
#define UNSTABLE_RELEASE

#if defined(UIQ)
// Define the type of UIQ platform
#define MOTOROLA
//#define SONYERICSSON
#endif


#ifdef OS70S

// When MMF is supported by the OS (from 7.0S), the OggPlay Plugin system 
// are using the MMF Player, a ECom Plugin. That allows, among other thing, to 
// use the phone built-in decoders.
#define PLUGIN_SYSTEM
// This flag is only for SOS 7.0S and over, should never be set for older SOS
#define MMF_AVAILABLE 

#else
// PLUGIN_SYSTEM on the SOS 6.1 is experimental at the moment. 
//#define PLUGIN_SYSTEM 
// support for legacy audio codec (-:
// this is experimental at the moment.
//#define MP3_SUPPORT

#endif /* OS70S */


// NEW CODE ENABLERS

#if defined(SERIES60)
#ifdef OS70S
// The splash screen is using S60 window server
// Much slower, but works with all phones.
#define SERIES60_SPLASH_WINDOW_SERVER
#else
// The splash screen is using direct screen access. 
// Much faster, but depends on phone refresh decision: Doesn't work with all phones.
#define SERIES60_SPLASH
#endif /*OS70S*/
#endif /*SERIES60*/

// Force filling the buffer to at least 75% of their capacity before sending them to
// the audio streaming device
// On NGage it gives a serious boost to avoid "not-ready buffer" problem.
// UIQ_?
#define FORCE_FULL_BUFFERS

// Multi threaded playback, non MMF only (experimental)
#if defined(SERIES60)
  #if !defined(OS70S)
  #define MULTI_THREAD_PLAYBACK
#endif
#endif

#if !defined(OS70S)
  // There is something wrong how Nokia audio streaming handles the
  // first buffers. They are somehow swallowed.
  // To avoid that problem, wait a short time before streaming, 
  // so that Application drawing have been done.
  #define DELAY_AUDIO_STREAMING_START
#endif

#if defined(SERIES60) || defined(SERIES80)
#define SEARCH_OGGS_FROM_ROOT
#endif

// Use MDCT coeffs to generate the freq analyser
#define MDCT_FREQ_ANALYSER 

// Some phones require direct TSY access for phone call notification
#if !defined(MOTOROLA)
#define MONITOR_TELEPHONE_LINE 
#endif


// other tags:
// #define RGAIN_SUPPORT
// #IMPROVE_MAD

// OTHERS
//
#if defined SERIES60    // Alternative to #ifdef/#endif around Series 60 code
#define IFDEF_S60(x)   x
#define IFNDEF_S60(x)  ;
#else
#define IFDEF_S60(x)   ;
#define IFNDEF_S60(x)  x
#endif


// Phone UIDs
// Used for determining the phone type (currently only used for the Sendo X)
#define EMachineUid_SendoX  0x101FA031
#define EMachineUid_NGage   0x101F8C19
#define EMachineUid_NGageQD 0x101FB2B1


// Playlist support
#define PLAYLIST_SUPPORT

#endif
