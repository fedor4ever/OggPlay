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
 
#ifndef TREMORDECODER_H
#define TREMORDECODER_H

#include <e32base.h>

#include <stdio.h> //FIXMAD: this NULL redefinition thing. Help.
#include "Decoder.h"
#include "ivorbisfile.h"

#define INPUT_BUFFER_SIZE	(5*8192)


class CTremorDecoder: public CBase, public MDecoder
{
public:
  TInt Clear();
  TInt Open(FILE* f);
  TInt OpenInfo(FILE* f);
  TInt Read(TDes8& aBuffer,int Pos);
  TInt Close(FILE*);

  TInt Channels();
  TInt Rate();
  TInt Bitrate();
  TInt64 Position();
  void Setposition(TInt64 aPosition);
  TInt64 TimeTotal();

  void ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber);
  
private:

  void GetString(TDes& aBuf, const char* aStr);

public: 

private:
  TInt                     iCurrentSection; // used in the call to ov_read
  OggVorbis_File   iVf;
  vorbis_info *vi;

  FILE* iInputFp;

  TDes* iTitle;

};


#endif