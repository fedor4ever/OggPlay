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

//#define SERIES60

//#if defined( __WINS__ )  && defined(__VC32__)
#define chSTR(x)           #x
#define chSTR2(x)        chSTR(x)
#define chmsg(desc) message(__FILE__ "("\
chSTR2(__LINE__) "):" #desc)

//#else
//#define chmsg(desc)
//#endif

#if defined(__AVKON_ELAF__) || defined(__AVKON_APAC__)
//#define SERIES60
#pragma chmsg(Building for SERIES60)
#else
#define UIQ
//#pragma chmsg(Building for UIQ)
#endif

#endif
