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
#include "id3tag.h"
#include "tag.h"
//#include "ltag.h"
#include <stdio.h> 
#include "bstdfile.h"
#include "OggPlayDecoder.h"

#define INPUT_BUFFER_SIZE	(5*8192)


class CMadDecoder: public CBase, public MDecoder
{
public:
  TInt Clear();
  TInt Open(FILE* f,const TDesC& aFilename=_L(""));
  TInt OpenInfo(FILE* f,const TDesC& aFilename=_L(""));
  TInt Read(TDes8& aBuffer,int Pos);
  TInt Close(FILE*);

  TInt Channels();
  TInt Rate();
  TInt Bitrate();
  TInt64 Position();
  void Setposition(TInt64 aPosition);
  TInt64 TimeTotal();
  TInt FileSize();

  void ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber);
  void GetFrequencyBins(TInt32* aBins,TInt NumberOfBins);
  
private:
  // Helper functions. Pardon the naming mess.

  signed short MadFixedToSshort(mad_fixed_t Fixed);
  int mad_outputpacket(unsigned char* ptr,const unsigned char* endptr);
  int mad_read(unsigned char* outputbuffer,int length);
  void getframeinfo(void);
  void gettaginfo(void);
  void jumpseek(void);
  void MadHandleError();

  // from madplay:
  struct id3_tag* get_id3(struct mad_stream *stream, id3_length_t tagsize);
  void process_id3(struct id3_tag const *tag);
  void Id3GetString(TDes& aString,char const *id);


  // private datastructures
private: 
  struct mad_stream	iStream;
  struct mad_frame	iFrame;
  struct mad_synth	iSynth;
  struct id3_tag *id3tag;
  struct id3_file *filetag;
  struct tag xltag; // that's for Xing and Lame tags.

  int vbr;
  unsigned int bitrate;
  unsigned long vbr_frames;
  unsigned long vbr_rate;

  mad_timer_t			iTimer; // current time
  mad_timer_t     iGlobalTimer; // time where we would like to jump to
  mad_timer_t     iTotalTime;
  mad_timer_t     iEstTotalTime;

  unsigned long		iFrameCount;
  int iPlaystate;
  enum playstates{
      EPLAY,
      EFASTFORWARD,
      EREVERSE
  };

  // file structures and misc. internal stuff
private:
  unsigned char InputBuffer[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD];
  unsigned char* iGuardPtr;
  const unsigned char	*iOutputBufferEnd;
  FILE* iInputFp;

  int	iStatus;
  int iFilesize;
  int	i;
  bstdfile_t* iBstdFile;

  int iRememberPcmSamples; //mad_read


};

#endif