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



// DEBUGGING
//
//#define TRACE_ON          // See OggLog


// NEW CODE ENABLERS
//
//#define SERIES60_SPLASH   // OggPlayAif.s60.rss and OggPlay.cpp


// BUILD TARGETS
//
// Enable this line to enforce the platform target.
//#define SERIES60

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


#endif