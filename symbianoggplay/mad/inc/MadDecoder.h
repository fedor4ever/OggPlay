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

#ifndef MADDECODER_H
#define MADDECODER_H

#include <e32base.h>
#include "mad.h"
#include <stdio.h> //FIXMAD: this NULL redefinition thing. Help.
#include "bstdfile.h"
#include "Decoder.h"

#define INPUT_BUFFER_SIZE	(5*8192)


class CMadDecoder: public CBase, public MDecoder
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

	signed short MadFixedToSshort(mad_fixed_t Fixed);
  int mad_outputpacket(unsigned char* ptr,const unsigned char* endptr);
  int mad_read(unsigned char* outputbuffer,int length);
  
public: //FIXME -- shouldn't be public
  struct mad_stream	iStream;
	struct mad_frame	iFrame;
	struct mad_synth	iSynth;
	mad_timer_t			iTimer;
	unsigned long		iFrameCount;

private:
	unsigned char InputBuffer[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD];
	unsigned char* iGuardPtr;
	const unsigned char	*iOutputBufferEnd;
  FILE* iInputFp;

	int					iStatus;
	int					i;
	bstdfile_t* iBstdFile;

  int iRememberPcmSamples; //mad_read


};

#endif