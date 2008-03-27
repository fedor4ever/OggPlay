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

// Platform defines
#include <OggPlatform.h>

#undef chmsg // Not particularily clever, but hides useless warning msg.
#if defined( __WINS__ )  && defined(__VC32__)
#define chSTR(x)           #x
#define chSTR2(x)        chSTR(x)
#define chmsg(desc) message(__FILE__ "("\
chSTR2(__LINE__) "):" #desc)

#else
#define chmsg(desc)
#endif

// DEBUGGING
// See OggLog. Manually create "C:\Logs\OggPlay" to get disk logging.
#define TRACE_ON

// Enable codec performance profiling
// #define PROFILE_PERF

// Enable buffer debugging code
// #define BUFFER_FLOW_DEBUG


// Enable source (i.e. file or stream) reading using the streaming thread
#define MULTITHREAD_SOURCE_READS

#if defined(OS70S)
// When MMF is supported by the OS (from 7.0S), the OggPlay Plugin system 
// are using the MMF Player, a ECom Plugin. That allows, among other thing, to 
// use the phone built-in decoders.
#define PLUGIN_SYSTEM
#endif

// Include mp3 codec (we could do with a #define to make the FLAC codec optional too)
#define MP3_SUPPORT

// Force filling the buffer to at least 75% of their capacity before sending them to
// the audio streaming device. On NGage it gives a serious boost to avoid "not-ready buffer" problem.
#define FORCE_FULL_BUFFERS

#if defined(SERIES60) || defined(SERIES80) || defined(DELAY_AUDIO_STREAMING_OPEN)
// Used on S60, S80, S90
// This must be defined if DELAY_AUDIO_STREAMING_OPEN is present because
// the code for !SEARCH_OGGS_FROM_ROOT assumes synchronous open
#define SEARCH_OGGS_FROM_ROOT
#endif

// Some phones require direct TSY access for phone call notification
#if !defined(MOTOROLA)
#define MONITOR_TELEPHONE_LINE 
#endif

// Phone UIDs
// Used for determining the phone type
#define EMachineUid_SendoX  0x101FA031
#define EMachineUid_NGage   0x101F8C19
#define EMachineUid_NGageQD 0x101FB2B1
#define EMachineUid_N3650   0x101F466A

#define EMachineUid_E61  0x20001858
#define EMachineUid_E61i 0x20002D7F
#define EMachineUid_E62  0x20001859
#endif
