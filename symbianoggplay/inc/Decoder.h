/*
 *  Copyright (c) 2004 P. Wolff
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
 
#ifndef MDECODER_H
#define MDECODER_H

class MDecoder 
{
public:
  virtual ~MDecoder() { };
  // Initialize Decoder settings
  virtual TInt Clear()=0;
  // Open file and gets ready to decode
  virtual TInt Open(FILE* f)=0;
  // Internal cleanup
  virtual TInt Close(FILE* f)=0;
  // Open file with just enough information to start reading tag info 
  // and get basic file information like channels, bitrate etc.
  virtual TInt OpenInfo(FILE* f)=0;

  // Parse tag information and put it in the provided buffers.
  virtual void ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, 
                         TDes& aGenre, TDes& aTrackNumber)=0;

  // Tries to fill Buffer with PCM data.
  //
  //  return values:
  //   <0) error/hole in data 
  //    0) EOF
  //    n) number of bytes of PCM actually returned.  
  //
  // This *might* work on a packet basis, so the calling function might have to 
  // provide a minimum buffer size. 
  virtual TInt Read(TDes8& aBuffer,int Pos)=0;

  // Basic getter functions. Unit for time functions (Position, TimeTotal etc...)
  // is milliseconds.
  virtual TInt Channels()=0;
  virtual TInt Rate()=0;
  virtual TInt Bitrate()=0;
  virtual TInt64 Position()=0;
  virtual void Setposition(TInt64 aPosition)=0;
  virtual TInt64 TimeTotal()=0;


};

#endif
