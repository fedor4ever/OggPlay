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

 // UIDs used for OggPlay


#ifndef __OggPlayUid_h
#define __OggPlayUid_h

#if defined(__GCC32__) || defined(__VC32__)
  #include <e32std.h>
  static const TInt KOggPlayApplicationUidValue = 0x101FF67D;
  static const TInt KOggPlayRecognizerUidValue  = 0x101FF67C;
  // Not sure this is correct mime.
  #define KOggMimeType "application/ogg"
#else
  // For .AIF file inclusion
  #define KOggPlayApplicationUidValue 0x101FF67D
  #define KOggPlayRecognizerUidValue  0x101FF67C
#endif

#endif